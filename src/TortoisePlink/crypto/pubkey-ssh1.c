/*
 * Convenience functions to encrypt and decrypt the standard format
 * for SSH-1 private key files. This uses triple-DES in SSH-1 style
 * (three separate CBC layers), but the same key is used for the first
 * and third layers.CBC mode.
 */

#include "ssh.h"

static ssh_cipher *des3_pubkey_cipher(const void *vkey)
{
    ssh_cipher *c = ssh_cipher_new(&ssh_3des_ssh1);
    uint8_t keys3[24], iv[8];

    memcpy(keys3, vkey, 16);
    memcpy(keys3 + 16, vkey, 8);
    ssh_cipher_setkey(c, keys3);
    smemclr(keys3, sizeof(keys3));

    memset(iv, 0, 8);
    ssh_cipher_setiv(c, iv);

    return c;
}

void des3_decrypt_pubkey(const void *vkey, void *vblk, int len)
{
    ssh_cipher *c = des3_pubkey_cipher(vkey);
    ssh_cipher_decrypt(c, vblk, len);
    ssh_cipher_free(c);
}

void des3_encrypt_pubkey(const void *vkey, void *vblk, int len)
{
    ssh_cipher *c = des3_pubkey_cipher(vkey);
    ssh_cipher_encrypt(c, vblk, len);
    ssh_cipher_free(c);
}
