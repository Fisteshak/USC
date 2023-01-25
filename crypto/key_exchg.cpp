#include <iostream>
#include "GInt.h"
#include "prime.h"
#include "key_exchg.h"
#include <stdexcept>

// network
#include <iostream>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <fstream>

using namespace std;
using tbyte = unsigned char;



int sendall(int s, tbyte* buf, int len, int flags)
{
    int total = 0;
    int n;

    while(total < len)
    {
        n = send(s, (char*)buf+total, len-total, flags);
        if(n == -1) { break; }
        total += n;
    }

    return (n==-1 ? -1 : total);
}

int recvall(int sock, tbyte* buf, int len, int flags){
    int total = 0;
    int received;

    while (total < len){
        received = recv(sock, (char*)buf + total, len - total, 0);
        if (received == -1){
            perror("Can't receive buffer\n0");
            break;
        }
        if (received == 0){
            // disconnected
            return -1;
        }
        total += received;
    }
    return 0;
}


void gcd_ext(gint a, gint b, gint x, gint y, gint d){

    if (ggint_is_zero(b)){     // b == 0
        ggint_one(x);          // x = 1
        ggint_zero(y);         // y = 0
        ggint_set_gint(d, a);  // d = a
        return;
    }
    gint x1, y1, q, r, p, tmp;
    ggint_mod_gint2(b, a, r);   // r = a%b
    gcd_ext(b, r, x1, y1, d);

    // x = y1;
    ggint_set_gint(x, y1);

    // y = x1 - (a/b)*y1
    ggint_div_gint(b, a, q, r); // q = a/b
    ggint_mul_gint(q, y1, p);   // p = (a/b)*y1
    ggint_set_gint(tmp, x1);    // tmp = x1
    ggint_sub_gint(p, tmp);     // tmp = x1 - (a/b)*y1
    ggint_set_gint(y, tmp);     // y = x1 - (a/b)*y1

    return;
}


// res = a^(-1) (mod m);    res*a = 1 (mod m)
void pow_1(gint a, gint m, gint res){

    gint x, y, d, _1, _0;
    ggint_zero(_0);
    ggint_one(_1);

    gcd_ext(a, m, x, y, d);
    if (ggint_equal(d, _1)){
        // x = ((x % m) + m) % m;
        gint tmp1, tmp2;

        ggint_mod_gint2(m, x, tmp1);     // tmp1 = x % m

        ggint_add_gint(m, tmp1);         // tmp1 = tmp1 + m

        ggint_mod_gint2(m, tmp1, tmp2);  // tmp2 = tmp1 % m

        ggint_set_gint(res, tmp2);       // res = ((x % m) + m) % m


    } else {
        ggint_zero(res);
    }
}

void generate_RSA_keys(gint e_, gint d_, gint n_){
    gint p, q, phi, _1, p_1, q_1;
    ggint_one(_1);
    ggint_zero(p);
    ggint_zero(q);
    ggint_zero(phi);


    generate_prime_number(p, RSA_KEY_LENGTH / 2);
    generate_prime_number(q, RSA_KEY_LENGTH / 2);

    // p_1 = p - 1
    ggint_set_gint(p_1, p);
    ggint_sub_gint(_1, p_1);

    // q_1 = q - 1
    ggint_set_gint(q_1, q);
    ggint_sub_gint(_1, q_1);

    // n = p*q
    ggint_mul_gint(p, q, n_);

    // phi(n) = (p - 1)*(q - 1)
    ggint_mul_gint(p_1, q_1, phi);

    // e = 65537 = 0x010001
    ggint_zero(e_);
    e_[0] = 0x01;
    e_[1] = 0x00;
    e_[2] = 0x01;

    // d = e^(-1) mod phi(n)
    ggint_zero(d_);
    pow_1(e_, phi, d_);
}






void RSA_encrypt(gint m, gint e, gint n, gint c){
    gint tmp_e;
    ggint_set_gint(tmp_e, e);
    ggint_pow_mod(m, tmp_e, n, c);
}

void RSA_decrypt(gint c, gint d, gint n, gint m){
    gint tmp_d;
    ggint_set_gint(tmp_d, d);
    ggint_pow_mod(c, tmp_d, n, m);
}




/*
void read_RSA_keys_from_file(const std::string &filename, int size_of_edn, gint e, gint d, gint n){

    if (size_of_edn / 4 != GInt_Size){
        //std::cerr << "The size you want to read doesn't match to `GInt_Size`\n";
        throw std::invalid_argument("The size you want to read doesn't match to `GInt_Size`\n");
        return;
    }

    ifstream inp(filename, std::ios_base::binary);
    int inp_size;
    inp.read((char *) &inp_size, 4);
    if (inp_size != size_of_edn){
        //std::cerr << "The size you want to read doesn't match to real\n";
        throw std::invalid_argument("The size you want to read doesn't match to real\n");
        return;
    }

    inp.read((char *) e, size_of_edn);
    inp.read((char *) d, size_of_edn);
    inp.read((char *) n, size_of_edn);

    inp.close();
}*/

void write_RSA_keys_to_file(const std::string &filename, int size_of_edn, gint e, gint d, gint n){

    if (size_of_edn != GInt_Size){
        std::cerr << "The size you want to write doesn't match to `GInt_Size`\n";
        return;
    }

    ofstream out(filename, std::ios_base::binary);
    out.write((char *) &size_of_edn, 4);
    out.write((char *) e, size_of_edn);
    out.write((char *) d, size_of_edn);
    out.write((char *) n, size_of_edn);

    out.close();
}




void get_RSA_keys(const std::string &filename, int size_of_edn, gint e, gint d, gint n){

    //read_RSA_keys_from_file("RSA.bin", RSA_KEY_LENGTH, e_res, d_res, n_res);




    if (size_of_edn / 4 != GInt_Size){

        std::cout <<"The size you want to read doesn't match to `GInt_Size`\n";
        return;
    }

    int32_t inp_size = 0;
    ifstream inp(filename, std::ios::in | std::ios::binary);
    if (inp.is_open()){
        inp.read((char *) &inp_size, 4);
    } else {
        std::cout << "Can't find RSA.bin\n";
        std::cout << "Generating RSA public and private keys...\n";
        generate_RSA_keys(e, d, n);
        std::cout << "Writing keys to a file \"RSA.bin\" ...\n";
        write_RSA_keys_to_file("RSA.bin", GInt_Size, e, d, n);
        return;
    }


    if (inp_size != GInt_Size){
        std::cout << "The size you want to read doesn't match to real\n";
        return;
    }

    inp.read((char *) e, size_of_edn);
    inp.read((char *) d, size_of_edn);
    inp.read((char *) n, size_of_edn);

    inp.close();

        // std::cout << "Can't find RSA.bin\n";
        // std::cout << "Generating RSA public and private keys...\n";
        // generate_RSA_keys(e_res, d_res, n_res);
        // std::cout << "Writing keys to a file \"RSA_keys.bin\" ...\n";
        // write_RSA_keys_to_file("RSA_keys.bin", GInt_Size, e_res, d_res, n_res);


}






void start_server(int sock, tbyte* result_key, int len, gint e_main, gint d_main, gint n_main){

    // send our public key
    tbyte* buf = new tbyte[GInt_Size*2];
    int cnt = 0;
    for (int i = 0; i < GInt_Size; i++){
        buf[cnt] = e_main[i];
        cnt++;
    }
    for (int i = 0; i < GInt_Size; i++){
        buf[cnt] = n_main[i];
        cnt++;
    }
    sendall(sock, buf, GInt_Size*2, 0);

    // recv AES key
    gint enc_key, key;
    recvall(sock, enc_key, GInt_Size, 0);

    // decrypt AES key
    gint m, c;
    ggint_zero(m);
    RSA_decrypt(enc_key, d_main, n_main, key);
    for (int i = 0; i < len; i++)
        result_key[i] = key[i];

}

// sock - socket, result_key - pointer, len - size of AES key in byte
void start_client(int sock, tbyte* result_key, int len){
    gint e, n;
    tbyte* buf = new tbyte[GInt_Size*2];

    // recv servers' public key
    recvall(sock, buf, GInt_Size*2, 0);
    int cnt = 0;
    for (int i = 0; i < GInt_Size; i++){
        e[i] = buf[cnt];
        cnt++;
    }
    for (int i = 0; i < GInt_Size; i++){
        n[i] = buf[cnt];
        cnt++;
    }

    // generate AES key
    for (int i = 0; i < len; i++)
        result_key[i] = rand() & 0xFF;

    // encrypt AES key
    gint m, c;
    ggint_zero(m);
    for (int i = 0; i < len; i++)
        m[i] = result_key[i];
    RSA_encrypt(m, e, n, c);

    // send encrypted AES key
    sendall(sock, c, GInt_Size, 0);
}










/*
// sock - socket, result_key - pointer, len - size of AES key
void alice_session(int sock, tbyte* result_key, int len){
    // it's client

    gint my_e, my_d, my_n, e, n;
    generate_RSA_keys(my_e, my_d, my_n);

    write_RSA_keys_to_file("alice_keys.bin", GInt_Size, my_e, my_d, my_n);

    // send our public key
    tbyte* buf = new tbyte[GInt_Size*2];
    int cnt = 0;
    for (int i = 0; i < GInt_Size; i++){
        buf[cnt] = my_e[i];
        cnt++;
    }
    for (int i = 0; i < GInt_Size; i++){
        buf[cnt] = my_n[i];
        cnt++;
    }
    sendall(sock, buf, GInt_Size*2, 0);


    // recv Bob's public key
    recvall(sock, buf, GInt_Size*2, 0);
    cnt = 0;
    for (int i = 0; i < GInt_Size; i++){
        e[i] = buf[cnt];
        cnt++;
    }
    for (int i = 0; i < GInt_Size; i++){
        n[i] = buf[cnt];
        cnt++;
    }

    // generate AES key
    for (int i = 0; i < len; i++)
        result_key[i] = rand() & 0xFF;


    // encrypt AES key
    gint m, c;
    ggint_zero(m);
    for (int i = 0; i < len; i++)
        m[i] = result_key[i];
    RSA_encrypt(m, e, n, c);

    // send encrypted AES key
    sendall(sock, c, GInt_Size, 0);
}



// sock - socket, result_key - pointer, len - size of AES key
void bob_session(int sock, tbyte* result_key, int len){
    // it's server

    gint my_e, my_d, my_n, e, n;
    generate_RSA_keys(my_e, my_d, my_n);

    //write_RSA_keys_to_file("bob_keys.bin", GInt_Size, my_e, my_d, my_n);

    // send our public key
    tbyte* buf = new tbyte[GInt_Size*2];
    int cnt = 0;
    for (int i = 0; i < GInt_Size; i++){
        buf[cnt] = my_e[i];
        cnt++;
    }
    for (int i = 0; i < GInt_Size; i++){
        buf[cnt] = my_n[i];
        cnt++;
    }
    sendall(sock, buf, GInt_Size*2, 0);


    // recv Alice's public key
    recvall(sock, buf, GInt_Size*2, 0);
    cnt = 0;
    for (int i = 0; i < GInt_Size; i++){
        e[i] = buf[cnt];
        cnt++;
    }
    for (int i = 0; i < GInt_Size; i++){
        n[i] = buf[cnt];
        cnt++;
    }

    // recv AES key
    gint enc_key, key;
    recvall(sock, enc_key, GInt_Size, 0);

    // decrypt AES key
    gint m, c;
    ggint_zero(m);
    RSA_decrypt(enc_key, my_d, my_n, key);
    for (int i = 0; i < len; i++)
        result_key[i] = key[i];

}
*/
