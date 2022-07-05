#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ERROR_INPUT 100
#define MIN_INPUT 1
#define MAX_PRIME 1000000   // one million

int read_input(long *n, int *ret);
int compute_Eratosthenes( int array_size, bool natural_numbers[array_size] );
void create_primes_array(int primes_size, int primes[primes_size], int num_size, bool natural_numbers[num_size]);
void calculate_decomposition(long *num, int arr_size, int tmp_arr[arr_size], int primes[arr_size] );
void print_decomposition(long *num, int arr_size, int tmp_arr[arr_size], int primes[arr_size] );

int main() {
    int ret = EXIT_SUCCESS;
    long n;
    bool cond = true;

    bool natural_numbers[MAX_PRIME];
    int npn = compute_Eratosthenes(MAX_PRIME, natural_numbers);
    int primes[npn];
    create_primes_array(npn, primes, MAX_PRIME, natural_numbers);

    while (cond == true) {
        cond = read_input(&n, &ret);
        if (cond == true) {
            int primes_in_decomposition[npn];
            calculate_decomposition(&n, npn, primes_in_decomposition, primes);
            print_decomposition(&n, npn, primes_in_decomposition, primes);
        }
    }
    return ret;
}

// reads input, returns true if reading should continue and false otherwise.
// also changes value of ret
int read_input(long *n, int *ret) {
    int cond = true;
    int r = scanf("%ld", n);
    if ( (r == 1) && ( *n >= MIN_INPUT ) ) {
        cond= true;
    } else if ( (r == 1) && ( *n == 0) ) {
        cond = false;
    } else {
        *ret = 100;
        fprintf(stderr, "Error: Chybny vstup!\n"); 
        cond = false;
    }
    return cond; 
}

// edits natural_numbers to only store true on indexes that are prime
// AND returns the number of primes up to MAX_PRIME
int compute_Eratosthenes(int array_size, bool natural_numbers[array_size] ) {
    int npn = 0;
    natural_numbers[0] = false;
    natural_numbers[1] = false;
    for (int i = 2; i < array_size; ++i) {
        natural_numbers[i] = true;
    }
    for ( int i = 2; i * i < array_size; ++i) {
        if (natural_numbers[i] == true) {
            for ( int j = i * i; j < array_size; j+=i) {
                natural_numbers[j] = false;
            }
        }
    }
    for (int i = 0; i < array_size; ++i) {
        if (natural_numbers[i] == true) {
            npn++;
        }
    }
    return npn;
}

// fills primes array of length npn with primes up to MAX_PRIME
void create_primes_array(int primes_size, int primes[primes_size], int num_size, bool natural_numbers[num_size]) {
    int counter = 0;
    for (int i = 0; i < num_size; ++i) {
        if ( natural_numbers[i] == true ) {
            primes[counter] = i;
            counter++;
        }
    }
}

// calculates prime decomposition of given number num
void calculate_decomposition(long *num, int arr_size, int tmp_arr[arr_size], int primes[arr_size] ) {
   // clear tmp_arr:
    for (int i = 0; i < arr_size; ++i) {
       tmp_arr[i] = 0;
    }
   // find the prime decomposition of num, mark it in tmp_arr> 
    long result = *num;
    int ind = 0;
    int p = primes[ind];
    while (true) {
        while ( (result % p) == 0 ) {
            tmp_arr[ind]++;
            result = result / p;
        }
        ind++;
        p = primes [ind];
        if (result == 1) {
            break;
        } 
    }
}

// print nonzero values from tmp_arr (prints the prime decomposition of num)
void print_decomposition(long *num, int arr_size, int tmp_arr[arr_size], int primes[arr_size] ) {
    printf("Prvociselny rozklad cisla %ld je:\n", *num);
    int counter = 0;
    int ind = 0;
    if (*num == 1) {
        printf("1");
    } else { 
        while (ind < arr_size) {
            if (tmp_arr[ind] == 1) {
                counter++;
                if (counter > 1) {
                    printf(" x ");
                }
                printf("%d", primes[ind]);
            } 
            else if (tmp_arr[ind] > 1) {
                counter++;
                if (counter > 1) {
                    printf(" x ");
                }
                printf("%d^%d", primes[ind], tmp_arr[ind]);
            }
            ind++;
        }
    }
    printf("\n");
}
