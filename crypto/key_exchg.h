#pragma once


#include "GInt.h"
#include "prime.h"

#include <string>

const int RSA_KEY_LENGTH  = 2048;



void gcd_ext(gint a, gint b, gint x, gint y, gint d_);
void pow_1(gint a, gint m, gint res);
void generate_RSA_keys(gint e_, gint d_, gint n_);

void RSA_encrypt(gint m, gint e_, gint n_, gint c);
void RSA_decrypt(gint c, gint d_, gint n_, gint m);

void get_RSA_key(const std::string &filename, int size_of_edn, gint e, gint d, gint n);

void read_RSA_keys_from_file(const std::string &filename, int size_of_edn, gint e, gint d, gint n);
void write_RSA_keys_to_file(const std::string &filename, int size_of_edn, gint e, gint d, gint n);

void client_session(int sock, tbyte* result_key, int len);
void server_session(int sock, tbyte* result_key, int len);

void start_server(int sock, tbyte* res_key, int len);
void start_client(int sock, tbyte* res_key, int len);

