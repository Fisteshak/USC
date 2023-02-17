#include "aes.h"

using namespace std;
using tbyte = unsigned char;

/* ------------------------------ AES METHODS ------------------------------ */

/* constructor */

aes::aes(uint enc_key, uint enc_mode) : enc_key(enc_key), enc_mode(enc_mode)
{
    Nb = 4;
    Nk = 4;
    Nr = 10;
    if (enc_key == CONST_AES_192)
    {
        Nk = 6;
        Nr = 12;
    }
    if (enc_key == CONST_AES_256)
    {
        Nk = 8;
        Nr = 14;
    }
    block_size = enc_key >> 3;
    w.assign(4, vector<tbyte>(Nk * (Nr + 1), 0));
    state.assign(4, vector<tbyte>(Nb, 0));
}

/* utilities */

vector<vector<tbyte>> aes::roundKeyToVecVec(tbyte round_num)
{
    // вытащить один roundKey из длинного массива w
    // get vector<vector> res         its size: rows = 4; cols = Nk
    // from long vector<vector> w     its size: rows = 4; cols = (Nk*Nr)
    vector<vector<tbyte>> res(4);
    for (tbyte j = round_num * Nk; j < (round_num + 1) * Nk; j++)
        for (tbyte i = 0; i < 4; i++)
            res[i].push_back(w[i][j]);
    return res;
}
tbyte aes::sbox(tbyte x)
{
    return SBOX[x >> 4][x & 0x0F];
}

tbyte aes::invSbox(tbyte x)
{
    return INVSBOX[x >> 4][x & 0x0F];
}

/* transformations */

void aes::keyExpansion()
{
    for (int i = Nk; i < Nk * (Nr + 1); i++)
    {
        if (i % Nk)
        {
            vector<tbyte> x1 = {w[0][i - 4], w[1][i - 4], w[2][i - 4], w[3][i - 4]};
            vector<tbyte> x2 = {w[0][i - 1], w[1][i - 1], w[2][i - 1], w[3][i - 1]};
            vector<tbyte> res(4);
            transform(x1.begin(), x1.end(), x2.begin(), res.begin(), bit_xor<tbyte>());
            for (int j = 0; j < 4; j++)
                w[j][i] = res[j];
            continue;
        }
        // if 1st row of next round key:

        // shift bytes; 1 byte <--- left
        vector<tbyte> cur = {w[1][i - 1], w[2][i - 1], w[3][i - 1], w[0][i - 1]};

        // subBytes
        transform(begin(cur), end(cur), begin(cur), [&](tbyte x)
                  { return sbox(x); });

        // cur = w[i-4] ^ cur ^ rcon[i/4 - 1];
        cur[0] ^= RCON[i / 4 - 1];

        vector<tbyte> tmp = {w[0][i - 4], w[1][i - 4], w[2][i - 4], w[3][i - 4]};
        transform(cur.begin(), cur.end(), tmp.begin(), tmp.begin(), bit_xor<tbyte>());

        for (int j = 0; j < 4; j++)
            w[j][i] = cur[j];
    }
}

void aes::subBytes()
{
    for (auto &row : state) // transform for each row of state
        transform(row.begin(), row.end(), row.begin(), [&](tbyte x)
                  { return sbox(x); });
}

void aes::shiftRows()
{
    for (int i = 1; i < 4; i++)
    {
        rotate(state[i].begin(), state[i].begin() + i, state[i].end());
    }
}

void aes::mixColumns()
{
    vector<vector<tbyte>> temp_state(4, vector<tbyte>(4, 0));
    for (tbyte state_col = 0; state_col < 4; state_col++)
    {
        for (tbyte state_row = 0; state_row < 4; state_row++)
        {
            for (tbyte cmds_col = 0; cmds_col < 4; cmds_col++)
            {
                if (CMDS[state_row][cmds_col] == 1)
                    temp_state[state_row][state_col] ^= state[state_row][state_col];
                else
                    temp_state[state_row][state_col] ^= GF_MUL_TABLE[CMDS[state_row][cmds_col]][state[state_row][state_col]];
            }
        }
    }
    state = temp_state;
}

void aes::addRoundKey(vector<vector<tbyte>> key)
{
    for (tbyte i = 0; i < 4; ++i)
        for (tbyte j = 0; j < Nk; ++j)
            state[i][j] ^= key[i][j];
}

/* inverse transformations */

void aes::invSubBytes()
{
    for (auto &row : state) // transform for each row of state
        transform(row.begin(), row.end(), row.begin(), [&](tbyte x)
                  { return invSbox(x); });
}

void aes::invShiftRows()
{
    for (int i = 1; i < 4; i++)
    {
        rotate(state[i].begin(), state[i].begin() + (4 - i), state[i].end());
    }
}

void aes::invMixColumns()
{
    vector<vector<tbyte>> temp_state(4, vector<tbyte>(4, 0));
    for (tbyte state_col = 0; state_col < 4; state_col++)
    {
        for (tbyte state_row = 0; state_row < 4; state_row++)
        {
            for (tbyte cmds_col = 0; cmds_col < 4; cmds_col++)
            {
                temp_state[state_row][state_col] ^= GF_MUL_TABLE[INV_CMDS[state_row][cmds_col]][state[state_row][state_col]];
            }
        }
    }
    state = temp_state;
}

/* general functions encrypt and decrypt */

void aes::encryptBlock()
{
    // initial round
    addRoundKey(roundKeyToVecVec(0));

    // main rounds
    for (tbyte i = 1; i <= Nr - 1; ++i)
    {
        subBytes();
        shiftRows();
        mixColumns();
        addRoundKey(roundKeyToVecVec(i));
    }

    // final round
    subBytes();
    shiftRows();
    addRoundKey(roundKeyToVecVec(Nr));
}

void aes::decryptBlock()
{
    // initial round
    addRoundKey(roundKeyToVecVec(Nr));

    // main rounds
    for (tbyte i = Nr - 1; i >= 1; i--)
    {
        invSubBytes();
        invShiftRows();
        invMixColumns();
        addRoundKey(roundKeyToVecVec(i));
    }

    // final round
    invSubBytes();
    invShiftRows();
    addRoundKey(roundKeyToVecVec(0));
}

// it disposes input buf and returns the new one
tbyte *aes::addPadding(uint64_t &len, tbyte *buf)
{

    // how much bytes should be added
    tbyte bytes_to_add = block_size - (len % block_size);

    // result buffer
    tbyte *result = new tbyte[len + bytes_to_add];

    // it means: new_buf = old_buf
    memcpy(result, buf, len);

    // it means: new_buf = new_buf + additional_block
    for (tbyte i = len; i < len + bytes_to_add; i++)
    {
        result[i] = bytes_to_add % block_size;
    }

    // update input parameters
    // delete []buf; // free(buf);
    // buf = nullptr;
    len += bytes_to_add;

    return result;
}


void dbg_printf2(tbyte *ptr, int len){
    for (int i = 0 ; i < len; i++){
        printf("%.2x", *(ptr + i));
    }
    printf("\n");
    return;
}


tbyte *aes::removePadding(uint64_t &len, tbyte *&buf)
{
    // последние несколько байт не равны (некорректный паддинг) =>  bytes_to_remove>len => len - bytes_to_remove < 0 => bad_alloc
    //cout << "removePadding dbg unciphered\n";
    //dbg_printf2(buf, len);
    tbyte bytes_to_remove = buf[len - 1] == 0 ? enc_key >> 3 : buf[len - 1];
    tbyte *result = new tbyte[len - bytes_to_remove];
    memcpy(result, buf, len - bytes_to_remove);
    delete []buf; // free(buf);
    buf = nullptr;
    len -= bytes_to_remove;
    return result;
}

// returns number of added bytes
int aes::addPadding(const string &filename)
{

    ofstream file(filename, ios::app | ios::binary);
    int size = filesystem::file_size((filesystem::path)filename);
    if (size < 0)
        return -1;

    tbyte num_bytes_to_add = block_size - (size % block_size);

    // add padding
    for (int i = 0; i < num_bytes_to_add; i++){
        tbyte data = num_bytes_to_add % block_size;
        file.write((const char*)&data,sizeof(data));
    }

    file.close();
    return num_bytes_to_add;
}

// void dbg_printf(tbyte *ptr, int len){
//     for (int i = 0 ; i < len; i++){
//         printf("%.2x", *(ptr + i));
//     }
//     printf("\n");
// }

// overload to encrypt buffer of bytes
tbyte* aes::encryptCBC(uint64_t &len, tbyte *buf, tbyte *key)
{

    uint64_t cnt = 0;
    uint64_t res_cnt = Nb * 4; // result buffer counter
    tbyte *res;

    // Note: first 4*Nb bytes of result is init_vector
    buf = addPadding(len, buf);
    res = new tbyte[Nb * 4 + len];

    // generating initialization vector (IV)
    srand(time(0));
    vector<vector<tbyte>> init_vec(4, vector<tbyte>(Nb, 0));
    for (tbyte j = 0; j < Nb; ++j)
        for (tbyte i = 0; i < 4; ++i)
        {
            init_vec[i][j] = rand() % 0xFF;
            res[j * 4 + i] = init_vec[i][j];
        }

    // convert bytes to 4xNk matrix w
    int ind = 0;
    for (tbyte j = 0; j < Nk; ++j)
    {
        for (tbyte i = 0; i < 4; ++i)
        {
            w[i][j] = key[ind];
            ind++;
        }
    }
    keyExpansion();

    auto prev_state = init_vec;
    while (cnt < len)
    {

        // convert bytes to block
        vector<vector<tbyte>> temp_state(4, vector<tbyte>(Nb, 0));
        for (tbyte j = 0; j < Nb; ++j)
        {
            for (tbyte i = 0; i < 4; ++i)
            {
                temp_state[i][j] = buf[cnt];
                cnt++;
            }
        }

        // current plain text XOR previous cipher text
        for (tbyte i = 0; i < 4; i++)
            transform(temp_state[i].begin(), temp_state[i].end(), prev_state[i].begin(), state[i].begin(), bit_xor<tbyte>());

        // encryption
        encryptBlock();

        // save result
        for (tbyte j = 0; j < Nb; ++j)
            for (tbyte i = 0; i < 4; ++i)
            {
                res[res_cnt] = state[i][j];
                res_cnt++;
            }

        // update variables
        prev_state = state;
    }

    //delete []buf; // free(buf);
    //buf = res;
    len = res_cnt;

    //dbg_printf(res, len);

    return res;
}

// overload for encrypting file
int aes::encryptCBC(const string &file_name, tbyte *key)
{
    int tmp = addPadding(file_name);
    if (tmp <= 0)
    {
        cerr << "Error! Can't get your file's size :(";
        return ERR_FILE_SIZE;
    }
    uint64_t len = filesystem::file_size((filesystem::path)file_name);

    // rename file with plain text
    if (rename(file_name.c_str(), (file_name + ".tmp").c_str()))
    {
        cerr << "Error! Can't open/rename your file :(\n";
        return ERR_RENAME_FILE;
    }

    // for cipher
    ofstream out(file_name, ios::out | ios::binary);
    // for reading plain text
    ifstream in(file_name + ".tmp", ios::in | ios::binary);

    uint64_t cnt = 0;
    uint64_t res_cnt = Nb * 4; // result buffer counter

    // generating initialization vector (IV)
    srand(time(0));
    vector<vector<tbyte>> init_vec(4, vector<tbyte>(Nb, 0));
    for (tbyte j = 0; j < Nb; ++j)
        for (tbyte i = 0; i < 4; ++i)
        {
            init_vec[i][j] = rand() % 0xFF;
            out.write((const char *) &init_vec[i][j], sizeof(init_vec[i][j]));
        }

    // convert bytes to 4xNk matrix w
    int ind = 0;
    for (tbyte j = 0; j < Nk; ++j)
    {
        for (tbyte i = 0; i < 4; ++i)
        {
            w[i][j] = key[ind];
            ind++;
        }
    }
    keyExpansion();

    auto prev_state = init_vec;
    while (cnt < len)
    {

        // convert bytes to block
        vector<vector<tbyte>> temp_state(4, vector<tbyte>(Nb, 0));
        for (tbyte j = 0; j < Nb; ++j)
        {
            for (tbyte i = 0; i < 4; ++i)
            {
                in.read((char *) &temp_state[i][j], sizeof(temp_state[i][j]));
                cnt++;
            }
        }

        // current plain text XOR previous cipher text
        for (tbyte i = 0; i < 4; i++)
            transform(temp_state[i].begin(), temp_state[i].end(), prev_state[i].begin(), state[i].begin(), bit_xor<tbyte>());

        // encryption
        encryptBlock();

        // save result
        for (tbyte j = 0; j < Nb; ++j)
            for (tbyte i = 0; i < 4; ++i)
                out.write((const char *) &state[i][j], sizeof(state[i][j]));

        // update variables
        prev_state = state;
    }

    // close files
    out.close();
    in.close();

    // remove input file
    remove((file_name + ".tmp").c_str());

    return 0;
}

// overload for decrypting file
int aes::decryptCBC(const string &file_name, tbyte *key)
{

    int64_t len = filesystem::file_size((filesystem::path)file_name);
    uint64_t cnt = 0;

    // rename file with cipher text
    if (rename(file_name.c_str(), (file_name + ".tmp").c_str()))
    {
        cerr << "Error! Can't open/rename your file :(\n";
        return ERR_RENAME_FILE;
    }

    // for plain
    ofstream out(file_name, ios::out | ios::binary);
    // for reading cipher text
    ifstream in(file_name + ".tmp", ios::in | ios::binary);

    vector<vector<tbyte>> init_vec(4, vector<tbyte>(Nb, 0));

    // first 4xNb bytes in encrypted data is init vector
    // so convert it to init_vec 4xNb table
    for (tbyte j = 0; j < Nb; ++j)
        for (tbyte i = 0; i < 4; ++i)
        {
            in.read((char *) &init_vec[i][j], sizeof(init_vec[i][j]));
            cnt++;
        }
    auto prev_cipher = init_vec;

    // convert bytes to 4xNk matrix w
    int ind = 0;
    for (tbyte j = 0; j < Nk; ++j)
    {
        for (tbyte i = 0; i < 4; ++i)
        {
            w[i][j] = key[ind];
            ind++;
        }
    }
    keyExpansion();

    while (cnt < len)
    {

        // convert bytes to block
        vector<vector<tbyte>> cur_cipher(4, vector<tbyte>(Nb, 0));
        for (tbyte j = 0; j < Nb; ++j)
        {
            for (tbyte i = 0; i < 4; ++i)
            {
                in.read((char *) &cur_cipher[i][j], sizeof(cur_cipher[i][j]));
                cnt++;
            }
        }

        state = cur_cipher;
        decryptBlock();

        for (tbyte i = 0; i < 4; i++)
            transform(state[i].begin(), state[i].end(), prev_cipher[i].begin(), state[i].begin(), bit_xor<tbyte>());

        // save result
        for (tbyte j = 0; j < Nb; ++j)
            for (tbyte i = 0; i < 4; ++i)
                out.write((char *) &state[i][j], sizeof(state[i][j]));

        // update variables
        prev_cipher = cur_cipher;
    }

    // close files
    out.close();
    in.close();

    // remove input file
    remove((file_name + ".tmp").c_str());

    // remove padding
    // read last byte
    tbyte x;
    in.open(file_name, ios::in);
    in.seekg(-1, ios::end);
    in.read((char *) &x, sizeof(x));
    // delete padding
    x = (x == 0) ? block_size : x;
    filesystem::resize_file(file_name, filesystem::file_size((filesystem::path)file_name) - x);

    return 0;
}

// overload for decrypting buffer of bytes
tbyte* aes::decryptCBC(uint64_t &len, tbyte *buf, tbyte *key)
{

    vector<vector<tbyte>> init_vec(4, vector<tbyte>(Nb, 0));
    tbyte *res = new tbyte[len - 4 * Nb];
    uint64_t cnt = 0;
    uint64_t res_cnt = 0;

    // first 4xNb bytes in encrypted data is init vector
    // so convert it to init_vec 4xNb table
    for (tbyte j = 0; j < Nb; ++j)
        for (tbyte i = 0; i < 4; ++i)
        {
            init_vec[i][j] = buf[cnt];
            cnt++;
        }


    // Я ВСТАВИЛ
    // convert bytes to 4xNk matrix w
    int ind = 0;
    for (tbyte j = 0; j < Nk; ++j)
    {
        for (tbyte i = 0; i < 4; ++i)
        {
            w[i][j] = key[ind];
            ind++;
        }
    }
    keyExpansion();

    auto prev_cipher = init_vec;
    while (cnt < len)
    {

        // convert bytes to block
        vector<vector<tbyte>> cur_cipher(4, vector<tbyte>(Nb, 0));
        for (tbyte j = 0; j < Nb; ++j)
        {
            for (tbyte i = 0; i < 4; ++i)
            {
                cur_cipher[i][j] = buf[cnt];
                cnt++;
            }
        }

        state = cur_cipher;
        decryptBlock();

        for (tbyte i = 0; i < 4; i++)
            transform(state[i].begin(), state[i].end(), prev_cipher[i].begin(), state[i].begin(), bit_xor<tbyte>());

        // save result
        for (tbyte j = 0; j < Nb; ++j)
            for (tbyte i = 0; i < 4; ++i)
            {
                res[res_cnt] = state[i][j];
                res_cnt++;
            }

        // update variables
        prev_cipher = cur_cipher;
    }

    res = removePadding(res_cnt, res);
    len = res_cnt;
    //buf = res;
    return res;
}

// just print vector
void checkVec(vector<vector<tbyte>> v)
{
    for (auto row : v)
    {
        for (auto el : row)
            cout << hex << (int)el << " ";
        cout << "\n";
    }
}