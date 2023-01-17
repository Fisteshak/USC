
#include "GInt.h"
#include "prime.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>

// Miller-Robin primality test
// return false: number n is composite
// return true:  number n is very likely to be a prime
//

// если s = 1 и число внатуре простое, то должно пройти по continue
// думаю, ошибка в этой строке ggint_pow_mod(a, tmp, n, x); // x = a^d mod n


bool is_prime(gint n, size_t trials)
{
    // если четно, сразу нет
    if (ggint_is_even(n))
        return false;


    // Представить n − 1 в виде (2^s)·t, где t нечётно, можно сделать последовательным делением n - 1 на 2.    
    // просто единица
    gint _1; 
    ggint_one(_1); // _1 = 1
    
    // n_1 = n - 1
    gint n_1;
    ggint_set_gint(n_1, n);  // n_1 = n
    ggint_sub_gint(_1, n_1); // n_1 = n_1 - 1

    size_t s = 0;
    gint m;
    ggint_set_gint(m, n_1); // m = n_1 = n - 1
    while (ggint_is_even(m))
    {
        s++;
        ggint_shbr(m, 1); // m /= 2
    }
    // n - 1 = (2^s)·t

    gint d;
    {
        ggint_set_gint(m, n_1);     // m = n_1 = n - 1
        gint t;
        ggint_set_gint(t, _1);      // t = _1 = 1
        gint r;
        ggint_shbl(t, s);           // t = 2^s
        ggint_div_gint(t, m, d, r); // d = m/t; r = m%t
    }
    // n - 1 = (2^s)*d

    
    if (trials <= 0)
        trials = 3;

    size_t i;

    // цикл А
    for (i = 0; i < trials; i++)
    {
        // Выбрать случайное целое число a в отрезке [2, n − 2]
        gint a;
        {
            gint _max; ggint_set_gint(_max, n);
            ggint_sub_gint(_1, _max);
            ggint_rand_range(a, _max);
        }

        gint x;
        gint tmp;
        ggint_set_gint(tmp, d);
        ggint_pow_mod(a, tmp, n, x); // x = a^d mod n;

        // если x = 1 или x = n − 1, то перейти на следующую итерацию цикла А
        if (ggint_equal(x, _1) || ggint_equal(x, n_1)) 
            continue;
        
        
        // цикл B
        size_t r;
        for (r = 0; r < s - 1; r++)
        {
            gint x2, g;
            ggint_mul_gint(x, x, x2); // x2 = x*x
            ggint_mod_gint(n, x2, g); // g = x2 % n
            ggint_set_gint(x, g);     // x = g
            // x = x^2 % n

            // если x = 1, то вернуть составное
            if (ggint_equal(x, _1))
                return false;

            // если x = n − 1, то перейти на следующую итерацию цикла A
            if (ggint_equal(x, n_1))
                break;
        } // цикл B конец

        // если x = n − 1, то перейти на следующую итерацию цикла A
        if (ggint_equal(x, n_1) == false) 
            return false;

    } // цикл А конец
    return true;
}

void check(gint n){
    // printf("\n");
    // for (int i = 0; i < 128; i++){
    //     printf("%.2x", n[i]);
    // }
    // printf("\n");
    // fflush(stdout);
    gint x;
    ggint_set_gint(x, n);
    ggint_print_format("p", x, true);
}

void check2(gint n){
    printf("\n");
    printf("%.2x", n[1]);
    printf("%.2x", n[0]);
    //printf("\n");
    fflush(stdout);
}
/*
нужно запустить под определенным сидом
найти простое число ($ primes - в линухе)
проверить в чем именно ошибка тут
в тесте миллера-рабина или где?
*/

void generate_prime_number(uint8_t number[], int nbits){
    size_t smallPrimes[100] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541};
    size_t smallPrimesCount = 100;

    gint _1;

    ggint_one(_1);

    gint n;
    ggint_zero(n);
    size_t pmod[100];

    printf("Searching for %d-bit prime ...\n", nbits);

    clock_t start,end;
    double cpu_time_used;
    start = clock();

    size_t ncheck = 0;
    size_t i,p,r;
    while (true)
    {
        if (ggint_is_zero(n))
        {

            my_ggint_rand(n, nbits); 

            if (ggint_is_even(n)) // проверка на четность
                ggint_add_int(1, n);

            for (i = 0; i < smallPrimesCount; i++)
            {
                p = smallPrimes[i];
                ggint_mod_int(p, n, &r);
                pmod[i] = r;
            }
        }
        //check(n);
        bool prime = true;
        while (true)
        {
            for (i = 0; i < smallPrimesCount; ++i)
            {
                if (pmod[i] == 0 && smallPrimes[i] != n[0]) // костыль. надо чекать первые два байта, но пофиг, т.к. n > 2^1024
                {
                    prime = false; // если составное
                    break;
                }
            }
            
            if (prime == false) // если составное
            {
                ncheck++;                
                ggint_add_int(2, n);
                
                //check2(n);

                for (i=0; i < smallPrimesCount; i++)
                {
                    p = smallPrimes[i];
                    pmod[i] += 2;
                    if (pmod[i] >= p)
                        pmod[i] -= p;
                }
                prime = true;
                //printf("*");
                //fflush(stdout);
                continue;
            }
            else // если простое
                break; 

            // зачем это надо?    
            //if (prime == false)
            //    break;
        }

        // когда прошли проверку на маленьких простых числах,
        // проверяем Миллером-Рабином
        if (is_prime(n, 10))
        {
            printf("\nFound prime\n");
            for(i = 0; i < 128; i++){
                number[i] = n[i]; //n[127-i];
            }
            //ggint_print_format("p", n,true);
            break;
        }
        else // 
        {
            printf(".");
            fflush(stdout);
        }

        ncheck++;
        ggint_add_int(2, n);

        for (i = 0; i < smallPrimesCount; ++i)
        {
            p = smallPrimes[i];
            pmod[i] += 2;
            if (pmod[i] >= p)
                pmod[i] -= p;
        }
    }

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC * 1000;
    printf("Checked %d numbers in %d ms: %g num/sec\n", (int) ncheck, (int) cpu_time_used, 1000.0*((double)(ncheck))/cpu_time_used);

    return;
}
