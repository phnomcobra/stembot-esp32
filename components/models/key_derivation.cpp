#include "key_derivation.hpp"

#include <cstring>

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
