#include "cipher.h"

struct cipher *alloc_cipher(enum method m, const char *password) {
    if (m == AEAD_CHACHA20_POLY1305) {
        struct cipher *c = static_cast<struct cipher *>(calloc(1, sizeof(struct cipher)));
        assert(c);

        c->key_size = 32;
        c->salt_size = 32;
        c->nonce_size = 12;
        c->tag_size = 16;

        if (password) {
            derive_key(password, c->key, c->key_size);
        }

        return c;
    }

    return NULL;
}

void free_cipher(struct cipher *c) { free(c); }

struct cipher_context *alloc_cipher_context(enum method m, const char *password) {
    struct cipher_context *ctx = static_cast<struct cipher_context *>(
            calloc(1, sizeof(struct cipher_context))
    );
    assert(ctx);

    ctx->de_cipher = alloc_cipher(m, password);
    ctx->en_cipher = alloc_cipher(m, password);
    assert(ctx->de_cipher && ctx->en_cipher);

    return ctx;
}

void free_cipher_context(void *ctx) {
    struct cipher_context *c = (struct cipher_context *) ctx;
    free_cipher(c->de_cipher);
    free_cipher(c->en_cipher);
    free(c);
}

void derive_key(const char *password, unsigned char *key, size_t key_len) {
    size_t datal;
    datal = strlen((const char *) password);

    const mbedtls_md_info_t *md = mbedtls_md_info_from_string("MD5");
    if (md == NULL) {
        LOG_EXIT("MD5 Digest not found in crypto library");
    }

    mbedtls_md_context_t c;
    unsigned char md_buf[MBEDTLS_MD_MAX_SIZE];
    int addmd;
    unsigned int i, j, mds;

    mds = mbedtls_md_get_size(md);
    memset(&c, 0, sizeof(mbedtls_md_context_t));
    mbedtls_md_setup(&c, md, 0);

    for (j = 0, addmd = 0; j < key_len; addmd++) {
        mbedtls_md_starts(&c);
        if (addmd) {
            mbedtls_md_update(&c, md_buf, mds);
        }
        mbedtls_md_update(&c, (uint8_t *) password, datal);
        mbedtls_md_finish(&c, &(md_buf[0]));

        for (i = 0; i < mds; i++, j++) {
            if (j >= key_len)
                break;
            key[j] = md_buf[i];
        }
    }

    mbedtls_md_free(&c);
}
