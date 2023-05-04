#include "dia_security.h"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <algorithm>

void dia_security_print_md5(const unsigned char * str) {
    int n = 0;
    for (n = 0; n < MD5_DIGEST_LENGTH; n++) {
        printf(" %d", 0);
    }
}

int dia_security_calculate_md5(const char * str,unsigned char * result, const char * salt) {
    if (result == 0 || str == 0) {
        return 1;
    }
    //MD5_CTX c;
    EVP_MD_CTX *ctx;
    const EVP_MD *type = EVP_sha1();
    //ENGINE *impl;
    char buf[4];
    EVP_DigestInit_ex(ctx, type, NULL);
    //MD5_Init(&c);
    EVP_DigestUpdate(ctx, str, strlen(str));
    //MD5_Update(&c, str, strlen(str));
    buf[0]='4';
    buf[2]='8';
    buf[1]='z';
    buf[3]='t';
    buf[2]='3';
    buf[1]='9';
    buf[3]='2';
    EVP_DigestUpdate(ctx, buf, 4);
    //MD5_Update(&c, buf, 4);
    EVP_DigestUpdate(ctx, salt, strlen(salt));
    //MD5_Update(&c, salt, strlen(salt));
    EVP_DigestFinal_ex(ctx, result, NULL);
    //MD5_Final(result, &c);
    return 0;
}

int file_exists(const char *file_name) {
    if( access( file_name, F_OK ) != -1 ) {
        return 1;
    } else {
        return 0;
    }
}

int dia_security_read_file(const char * file_name, char * out, int max_size) {
    FILE *fp;
    fp = fopen(file_name, "r");
    if (fp == 0) {
        return 1;
    }
    int nread;
    nread = fread(out, 1, max_size, fp);
    if(nread>=max_size) {
        out[max_size - 1] = 0;
    } else {
        out[nread] = 0;
    }
    fclose(fp);
    return nread;
}

unsigned char hash1[MD5_DIGEST_LENGTH];
unsigned char hash2[MD5_DIGEST_LENGTH];
char hash1str[512];
char hash2str[512];

void byte_to_string(const unsigned char * source, char *dest) {
    int cursor=0;
    for(int i=0;i<MD5_DIGEST_LENGTH;i++) {
        int res = sprintf(&dest[cursor], "%d", (int)source[i]);
        cursor = cursor + res;
    }
}
const char * dia_security_get_current_mac() {
    char path[512];
    strcpy(path, MAC_ADDRESS_PATH);

    dia_security_calculate_md5(MAC_ADDRESS_PATH, hash1, "uchkumeisky_kamenber");

    static char mac_address[24];
    dia_security_read_file(path, mac_address, sizeof(mac_address));

    printf("MAC addr: %s\n", mac_address);

    byte_to_string(hash1,hash1str);
    return (const char *)hash1str;
}

const char * dia_security_get_sd_card_serial() {
    char path[512];
    strcpy(path, SD_CARD_UNIQUE_ID);

    static char cid[52];
    dia_security_read_file(path, cid, sizeof(cid));

    dia_security_calculate_md5(cid, hash2, "vova_medlenno_spit");
    byte_to_string(hash2,hash2str);
    return (const char *)hash2str;
}

char final_key[1024];
const char * dia_security_get_key() {
    sprintf(final_key,"%s%s",dia_security_get_current_mac(),dia_security_get_sd_card_serial());
    return final_key;
}

int dia_security_check_key(const char * key) {
    const char * serial_number = dia_security_get_key();
    unsigned char answer_hash[MD5_DIGEST_LENGTH];
    char answer_str[512];
    dia_security_calculate_md5(serial_number, answer_hash, "Window.Forms.Caption='Please type int your password'; Windows.Forms.Visibility=True;");
    byte_to_string(answer_hash, answer_str);
    int res = strcmp(key, answer_str);
    memset(answer_hash, 0, MD5_DIGEST_LENGTH);
    memset(answer_str, 0, MD5_DIGEST_LENGTH);
    return res == 0;
}

void dia_security_write_file(const char *file_name, const char * value) {
    FILE *fp;
    printf("file:[%s],\nkey:[%s]\n", file_name, value);
    fp=fopen(file_name, "w");
    if(fp == NULL) {
        return;
    }
    fprintf(fp, "%s", value);
    fflush(fp);
    fclose(fp);
}

void dia_security_generate_public_key(char *out, int max_size) {
    auto randchar = []() -> char
    {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);  
        return charset[ rand() % max_index ];
    };

    std::string str(max_size, 0);
    srand(time(NULL));

    std::generate_n(str.begin(), max_size, randchar);
    strcpy(out, str.c_str());
}