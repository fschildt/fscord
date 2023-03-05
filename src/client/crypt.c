// @Quality: only use 1 bio instance for everything?

internal EVP_PKEY *
pem_to_rsa(const char *pem, u32 pem_len, bool is_public)
{
    EVP_PKEY *key = 0;

    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        printf("BIO_new failed\n");
        goto end;
    }

    if (BIO_write(bio, pem, pem_len) <= 0)
    {
        printf("BIO_write failed\n");
        goto end;
    }


    if (is_public)
    {
        key = PEM_read_bio_PUBKEY(bio, 0, 0, 0);
        if (!key)
        {
            printf("PEM_read_bio_PUBKEY failed\n");
            goto end;
        }
    }
    else
    {
        key = PEM_read_bio_PrivateKey(bio, 0, 0, 0);
        if (!key)
        {
            printf("PEM_read_bio_PrivateKey failed\n");
            goto end;
        }
    }

end:
    BIO_free(bio);
    return key;
}

#if 0
internal char *
rsa_to_pem(EVP_PKEY *key, bool is_public)
{
    assert(is_public);

    char *pem = 0;
    const char *pem_in_bio = 0;

    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        printf("BIO_new failed\n");
        goto end;
    }

    if (!PEM_write_bio_PUBKEY(bio, key))
    {
        printf("PEM_write_bio_PUBKEY failed\n");
        goto end;
    }

    long data_size = BIO_get_mem_data(bio, &pem_in_bio);
    pem = malloc(data_size); // @LEAK
    if (!pem)
    {
        printf("malloc failed\n");
        goto end;
    }
    memcpy(pem, pem_in_bio, data_size);

end:
    BIO_free(bio);
    return pem;
}
#endif


// rsa ciphertext length is the same as the key size
internal bool
rsa_encrypt(EVP_PKEY *key, void *plaintext, void *ciphertext, size_t size)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(key, 0);
    if (!ctx)
    {
        printf("EVP_PKEY_CTX_new failed\n");
        return false;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0)
    {
        printf("EVP_PKEY_encrypt_init failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
    {
        printf("EVP_PKEY_CTX_set_rsa_padding failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    size_t ciphertext_len;
    if (EVP_PKEY_encrypt(ctx, 0, &ciphertext_len, plaintext, size) <= 0)
    {
        printf("EVP_PKEY_encrypt failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    if (EVP_PKEY_encrypt(ctx, ciphertext, &ciphertext_len, plaintext, size) <= 0)
    {
        printf("EVP_PKEY_encrypt failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    printf("rsa encrypted %zu plaintext-bytes to %zu ciphertext-bytes\n", size, ciphertext_len);
    EVP_PKEY_CTX_free(ctx);
    return true;
}

#if 0
internal EVP_PKEY *
generate_rsa(int keysize_in_bits)
{
    EVP_PKEY *key = 0;

    EVP_PKEY_CTX *context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, 0);
    if (!context)
    {
        printf("EVP_PKEY_CTX_new_id failed\n");
        goto end;
    }

    if (EVP_PKEY_keygen_init(context) <= 0)
    {
        printf("EVP_PKEY_keygen_init failed\n");
        goto end;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(context, keysize_in_bits) <= 0)
    {
        printf("EVP_PKEY_CTX_set_rsa_keygen_bits failed\n");
        goto end;
    }

    if (EVP_PKEY_keygen(context, &key) <= 0)
    {
        printf("EVP_PKEY_keygen failed\n");
        goto end;
    }

end:
    EVP_PKEY_CTX_free(context);
    return key;
}
#endif



// About aes 256bit data
// - key should be 256bit (32 byte)
// - iv should be 128bit (16 byte)
// - iv should be unique per message per key (nonce - number once)
// - gcm is a combination of CTR and MAC
// - ciphertext and plaintext have the same length

internal bool
aes_gcm_decrypt(u8 *ciphertext, s32 ciphertext_len,
                u8 *plaintext, // plaintext_len = ciphertext_len
                u8 *key, s32 key_size,
                u8 *iv, s32 iv_size,
                u8 *tag, s32 tag_size)
{
    printf("aes_gcm_decrypt:\n"
           "\t key = [%d, %d, ...]\n"
           "\t  iv = [%d, %d, ...]\n"
           "\t tag = [%d, %d, ...]\n"
           "\tsize = %d\n"
           "\tfrom = [%d, %d, ...]\n",
           key[0], key[1],
           iv[0], iv[1],
           tag[0], tag[1],
           ciphertext_len,
           ciphertext[0], ciphertext[1]);

    EVP_CIPHER_CTX *ctx;
    int plaintext_len;
    int tmp_len;

    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), 0, key, iv) != 1)
    {
        printf("EVP_DecryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &tmp_len, ciphertext, ciphertext_len) != 1)
    {
        printf("EVP_DecryptUpdate() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len = tmp_len;
    printf("\t  to = [%d, %d, ...]\n",
           ((u8*)plaintext)[0], ((u8*)plaintext)[1]);

    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_size, tag) != 1)
    {
        printf("EVP_CIPHER_CTX_ctrl (GCM_SET_TAG) failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len, &tmp_len) != 1)
    {
        printf("EVP_DecryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += tmp_len;

    if (plaintext_len != ciphertext_len)
    {
        printf("aes_gcm_decrypt failed: plaintext_len != ciphertext_len\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    EVP_CIPHER_CTX_free(ctx);

    return true;
}

internal bool
aes_gcm_encrypt(void *plaintext, int plaintext_len, void *ciphertext, u8 *key, s32 key_size, u8 *out_iv, int out_iv_len, u8 *out_tag, int out_tag_len)
{
    EVP_CIPHER_CTX *ctx;
    int ciphertext_len;
    int tmp_len;

    if (RAND_bytes(out_iv, out_iv_len) <= 0)
    {
        printf("aes_gcm_encrypt failed: cannot generate iv_nonce\n");
        return false;
    }

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        printf("EVP_CIPHER_CTX_new() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), 0, key, out_iv) != 1)
    {
        printf("EVP_EncryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptUpdate(ctx, ciphertext, &tmp_len, plaintext, plaintext_len) != 1)
    {
        printf("EVP_EncryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = tmp_len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &tmp_len) != 1)
    {
        printf("EVP_EncryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += tmp_len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, out_tag_len, out_tag) != 1)
    {
        printf("EVP_CIPHER_CTX_ctrl failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }


    EVP_CIPHER_CTX_free(ctx);
    return true;
}

