#include "helpers.h"
bool strToJson(std::string body, Json::Value &root)
{
    Json::CharReaderBuilder builder;
    Json::CharReader *reader = builder.newCharReader();

    std::string errors;

    bool parsingSuccessful = reader->parse(
        body.c_str(),
        body.c_str() + body.size(),
        &root,
        &errors);
    delete reader;

    if (!parsingSuccessful)
    {
        std::cout << "Failed to parse the JSON, errors:" << std::endl;
        std::cout << errors << std::endl;
        return false;
    }
    return true;
}

std::string bauth_get(const std::string user, const std::string pass)
{
    std::string tmp = user + ":" + pass;
    std::string prefix = "Authorization: Basic ";
    struct mg_str u = mg_str(tmp.c_str());
    char buf[1024];
    int i, n = 0;
    for (i = 0; i < (int)u.len; i++)
    {
        n = mg_base64_update(((unsigned char *)u.ptr)[i], buf, n);
    }
    n = mg_base64_final(buf, n);
    return prefix + std::string(buf) + "\r\n";
}

std::string token_generated(int length)
{
    std::string source_str = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 mt(rd());
    std::string ret_str;
    std::uniform_int_distribution<int> dis(0, source_str.length() - 1);
    for (int i = 0; i < length; i++)
    {
        ret_str += source_str.at(dis(mt));
    }
    return ret_str;
}
std::string pass_generated(const std::string user, const std::string token)
{
    mbedtls_aes_context aes_ctx;
    const unsigned char *key = (const unsigned char *)token.c_str();
    const unsigned char *plain_text = (const unsigned char *)user.c_str();
    //解密后明文的空间
    unsigned char dec_plain[1024] = {0};
    //密文空间
    unsigned char cipher[1024] = {0};

    mbedtls_aes_init(&aes_ctx);

    //设置加密密钥
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, plain_text, cipher);

    mbedtls_aes_free(&aes_ctx);
    return std::string((char *)cipher);
}

std::string pass_decode(const std::string pass, const std::string token)
{
    mbedtls_aes_context aes_ctx;
    const unsigned char *key = (const unsigned char *)token.c_str();
    const unsigned char *cipher_text = (const unsigned char *)pass.c_str();
    unsigned char dec_plain[1024] = {0};
    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, cipher_text, dec_plain);
    mbedtls_aes_free(&aes_ctx);
    return std::string((char *)dec_plain);
}