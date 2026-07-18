// AES-256-EAX encryption and decryption using the PSA Crypto API.
//
// EAX mode is constructed from AES-CMAC (OMAC) and AES-CTR:
//   N = OMAC_K^0(nonce)          -- nonce authenticator
//   H = OMAC_K^1([])             -- header authenticator (empty header)
//   C = CTR_K(N, plaintext)      -- ciphertext
//   M = OMAC_K^2(C)              -- ciphertext authenticator
//   Tag = N XOR H XOR M          -- 16-byte MAC tag
//
// OMAC_K^t(X) = CMAC(K, [0x00...0x0t] || X)  where the prefix is a
// 16-byte block with the last byte set to t (0, 1, or 2).
//
// Wire format (header values, not EAX internal values):
//   Nonce header  = raw 16-byte random nonce (before OMAC processing)
//   Tag header    = 16-byte MAC tag (N XOR H XOR M)
//   Body          = AES-CTR ciphertext

#include "agent.hpp"
#include "psa/crypto.h"

#include <ArduinoJson.h>
#include <cstring>
#include <vector>

// ── Hex helpers ───────────────────────────────────────────────────────────────

std::string bytes_to_hex(const uint8_t* data, size_t len)
{
    static const char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++)
    {
        out += kHex[(data[i] >> 4) & 0xFu];
        out += kHex[data[i] & 0xFu];
    }
    return out;
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex)
{
    std::vector<uint8_t> out;
    if (hex.size() % 2 != 0)
        return out;

    auto nibble = [](char c) -> std::optional<uint8_t>
    {
        if (c >= '0' && c <= '9')
            return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f')
            return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F')
            return static_cast<uint8_t>(c - 'A' + 10);
        return std::nullopt;
    };

    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        const auto hi = nibble(hex[i]);
        const auto lo = nibble(hex[i + 1]);
        if (!hi || !lo)
        {
            out.clear();
            return out;
        }
        out.push_back(static_cast<uint8_t>((*hi << 4) | *lo));
    }
    return out;
}

// ── PSA helpers ───────────────────────────────────────────────────────────────

static bool import_aes256(const uint8_t key[32], psa_algorithm_t alg, psa_key_usage_t usage,
                          psa_key_id_t* out_id)
{
    psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attrs, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attrs, 256);
    psa_set_key_algorithm(&attrs, alg);
    psa_set_key_usage_flags(&attrs, usage);
    return psa_import_key(&attrs, key, 32, out_id) == PSA_SUCCESS;
}

// Compute OMAC_t(data) = CMAC(key_id, [0x00^15 || t] || data).
static bool omac_t(psa_key_id_t mac_key, uint8_t t, const uint8_t* data, size_t len,
                   uint8_t out[16])
{
    std::vector<uint8_t> input(16 + len);
    memset(input.data(), 0, 16);
    input[15] = t;
    if (len > 0)
    {
        memcpy(input.data() + 16, data, len);
    }
    size_t mac_len = 0;
    return psa_mac_compute(mac_key, PSA_ALG_CMAC, input.data(), input.size(), out, 16, &mac_len) ==
           PSA_SUCCESS;
}

// CTR-mode encrypt or decrypt with a specific IV (both directions are identical
// for CTR mode).
static bool ctr_crypt(const uint8_t key[32], const uint8_t iv[16], const uint8_t* input, size_t len,
                      std::vector<uint8_t>& output)
{
    psa_key_id_t ctr_key = PSA_KEY_ID_NULL;
    if (!import_aes256(key, PSA_ALG_CTR, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT, &ctr_key))
        return false;

    size_t buf_size = len + PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES);
    output.resize(buf_size);

    psa_cipher_operation_t op = PSA_CIPHER_OPERATION_INIT;
    bool ok = false;

    if (psa_cipher_encrypt_setup(&op, ctr_key, PSA_ALG_CTR) != PSA_SUCCESS)
        goto done;
    if (psa_cipher_set_iv(&op, iv, 16) != PSA_SUCCESS)
        goto abort;

    {
        size_t out1 = 0;
        size_t out2 = 0;
        if (psa_cipher_update(&op, input, len, output.data(), buf_size, &out1) != PSA_SUCCESS)
            goto abort;
        if (psa_cipher_finish(&op, output.data() + out1, buf_size - out1, &out2) != PSA_SUCCESS)
            goto abort;
        output.resize(out1 + out2);
        ok = true;
        goto done;
    }

abort:
    psa_cipher_abort(&op);
done:
    psa_destroy_key(ctr_key);
    return ok;
}

// ── Public API ────────────────────────────────────────────────────────────────

AesEaxResult aes_eax_encrypt(const uint8_t key[32], const uint8_t* data, size_t len)
{
    AesEaxResult result{};
    psa_crypto_init();

    // Random nonce (16 bytes)
    if (psa_generate_random(result.nonce, 16) != PSA_SUCCESS)
        return result;

    // CMAC key (sign only)
    psa_key_id_t mac_key = PSA_KEY_ID_NULL;
    if (!import_aes256(key, PSA_ALG_CMAC, PSA_KEY_USAGE_SIGN_MESSAGE, &mac_key))
        return result;

    // N = OMAC_0(nonce),  H = OMAC_1([])
    uint8_t N[16], H[16], M[16];
    if (!omac_t(mac_key, 0, result.nonce, 16, N) || !omac_t(mac_key, 1, nullptr, 0, H))
    {
        psa_destroy_key(mac_key);
        return result;
    }

    // C = CTR_N(plaintext)
    if (!ctr_crypt(key, N, data, len, result.ciphertext))
    {
        psa_destroy_key(mac_key);
        return result;
    }

    // M = OMAC_2(C)
    if (!omac_t(mac_key, 2, result.ciphertext.data(), result.ciphertext.size(), M))
    {
        psa_destroy_key(mac_key);
        result.ciphertext.clear();
        return result;
    }
    psa_destroy_key(mac_key);

    // Tag = N XOR H XOR M
    for (int i = 0; i < 16; i++)
        result.tag[i] = N[i] ^ H[i] ^ M[i];

    return result;
}

std::optional<std::vector<uint8_t>> aes_eax_decrypt(const uint8_t key[32], const uint8_t nonce[16],
                                                    const uint8_t tag[16],
                                                    const uint8_t* ciphertext, size_t ct_len)
{
    psa_crypto_init();

    psa_key_id_t mac_key = PSA_KEY_ID_NULL;
    if (!import_aes256(key, PSA_ALG_CMAC, PSA_KEY_USAGE_SIGN_MESSAGE, &mac_key))
        return std::nullopt;

    // N = OMAC_0(nonce),  H = OMAC_1([])
    uint8_t N[16], H[16], M[16];
    if (!omac_t(mac_key, 0, nonce, 16, N) || !omac_t(mac_key, 1, nullptr, 0, H))
    {
        psa_destroy_key(mac_key);
        return std::nullopt;
    }

    // Plaintext = CTR_N(ciphertext)
    std::vector<uint8_t> plaintext;
    if (!ctr_crypt(key, N, ciphertext, ct_len, plaintext))
    {
        psa_destroy_key(mac_key);
        return std::nullopt;
    }

    // M = OMAC_2(ciphertext)
    if (!omac_t(mac_key, 2, ciphertext, ct_len, M))
    {
        psa_destroy_key(mac_key);
        return std::nullopt;
    }
    psa_destroy_key(mac_key);

    // Verify tag (constant-time)
    uint8_t diff = 0;
    for (int i = 0; i < 16; i++)
        diff |= static_cast<uint8_t>((N[i] ^ H[i] ^ M[i]) ^ tag[i]);

    if (diff != 0)
        return std::nullopt; // authentication failed

    return plaintext;
}

// ── set_isrc_json ─────────────────────────────────────────────────────────────

std::string set_isrc_json(const std::string& msg_json, const std::string& isrc)
{
    JsonDocument doc;
    deserializeJson(doc, msg_json);
    doc["isrc"] = isrc;
    std::string out;
    serializeJson(doc, out);
    return out;
}
