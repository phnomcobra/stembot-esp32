#include "crypto.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>

#ifdef ESP_PLATFORM
#include "psa/crypto.h"
#else
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#endif

namespace
{
constexpr unsigned int KDF_ITERATIONS = 200000;
constexpr char KDF_SALT_PREFIX[] = "stembot:user-key:v1:";
} // namespace

bool derive_user_key(uint8_t out[32], const std::string& passphrase, const std::string& objuuid)
{
    const std::string salt = std::string(KDF_SALT_PREFIX) + objuuid;
#ifdef ESP_PLATFORM
    psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;

    if (psa_crypto_init() != PSA_SUCCESS ||
        psa_key_derivation_setup(&operation, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256)) != PSA_SUCCESS ||
        psa_key_derivation_input_integer(&operation, PSA_KEY_DERIVATION_INPUT_COST,
                                         KDF_ITERATIONS) != PSA_SUCCESS ||
        psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_SALT,
                                       reinterpret_cast<const uint8_t*>(salt.data()),
                                       salt.size()) != PSA_SUCCESS ||
        psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
                                       reinterpret_cast<const uint8_t*>(passphrase.data()),
                                       passphrase.size()) != PSA_SUCCESS ||
        psa_key_derivation_output_bytes(&operation, out, 32) != PSA_SUCCESS)
    {
        psa_key_derivation_abort(&operation);
        memset(out, 0, 32);
        return false;
    }

    psa_key_derivation_abort(&operation);
    return true;
#else
    return mbedtls_pkcs5_pbkdf2_hmac_ext(
               MBEDTLS_MD_SHA256, reinterpret_cast<const unsigned char*>(passphrase.data()),
               passphrase.size(), reinterpret_cast<const unsigned char*>(salt.data()), salt.size(),
               KDF_ITERATIONS, 32, out) == 0;
#endif
}

double user_key_expire_time()
{
    return static_cast<double>(std::time(nullptr)) + 600.0;
}

KeyType key_type_from_string(const std::string& str)
{
    std::string normalized = str;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (normalized == "user")
        return KeyType::USER;
    if (normalized == "dist")
        return KeyType::DIST;
    return KeyType::UNKNOWN;
}

std::string key_type_to_string(KeyType key_type)
{
    switch (key_type)
    {
    case KeyType::USER:
        return "user";
    case KeyType::DIST:
        return "dist";
    default:
        return "unknown";
    }
}
