#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include "complex_math.h"

// image creation
unsigned char* create_blank(int w, int h) {
    unsigned char* img = (unsigned char*)calloc(RGB* w * h, sizeof(unsigned char));
    if(img == NULL) {
        exit(EXIT_FAILURE);
    }
    return img;
}

// calculate t (=k/n) for each pixel
double* create_t_array(int n, complex* c, int w, int h, complex* bound1, complex* bound2) {
    double dx = (bound2->a - bound1->a)/((double)w);   // should be 0.01
    double dy = (-1)*( (bound2->b - bound1->b)/((double)h) );   // should be -0.00916

    //create array:
    double* t_array = (double*) malloc(w*h*sizeof(double));
    if (t_array == NULL) {
        fprintf(stderr, "Error: Memory Error!\n");
        exit(EXIT_FAILURE);
    }
    //calculate t values and put them into the array:
    for (int i=0; i< h; ++i) {
        //printf("i=%d", i);
        for (int j=0; j<w; ++j) {
            complex* z = (complex*)malloc(sizeof(complex));
            if(z == NULL) {
                fprintf(stderr, "Error: Memory Error!\n");
                exit(EXIT_FAILURE);
            }
            z->a = bound1->a + j*dx;
            z->b = bound2->b + i*dy;

            int k = find_k(z, c, n);
            double t= ((double)k)/((double)n);
            *(t_array + i*w + j) = t;

            free(z);
        }
    }

    return t_array;
}

//create an image array from array full of t-values
unsigned char* create_image(int w, int h, double* t_arr) {
    unsigned char* image = malloc(3*w*h*sizeof(unsigned char));
    if (image == NULL) {
        fprintf(stderr, "Error: Memory Error!\n");
        exit(EXIT_FAILURE);
    }
    for (int i=0; i< w*h; ++i) {
        //printf("t_arr[%d] = %f \n", i, t_arr[i]);
        unsigned char r = (unsigned char) round(R( t_arr[i] ));
        unsigned char g = (unsigned char) round(G( t_arr[i] ));
        unsigned char b = (unsigned char) round(B( t_arr[i] ));
        //printf("%f,%f,%f\n", R( t_arr[i] ), G( t_arr[i] ),  B( t_arr[i] ));
        *(image + 3*i ) = r;
        *(image + 3*i +1 ) = g;
        *(image + 3*i +2 ) = b;
        //printf("%d,%d,%d\n\n", r, g, b );
    }
    return image;
}


//list of complex numbers
void add_to_compl_arr(complex**compl_arr, int* cap, complex* new_cn) {
    int occupied = 0;
    complex* element = compl_arr[0];
    while (element != NULL) {
        occupied++;
        element = *(compl_arr + occupied);
    }
    // realloc if needed:
    /* if ( (occupied +1) == *cap) {
        *(cap) *= 2;
        printf("new cap=%d\n", *cap);
        compl_arr = (complex**) realloc(compl_arr, *(cap)*sizeof(complex) );
        if (compl_arr == NULL) {
            fprintf(stderr, "Error: Realloc memory Error!\n");
            exit(EXIT_FAILURE);
        }
        int start = occupied;
        for (int i = start; i<*cap; ++i) {
            *(compl_arr + i) = NULL;
        }
        printf("print out compl_arr from realloc\n");
        for (int i=0; i<*cap; ++i ) {
            printf("on idx=%d is %p\n", i, *(compl_arr+i) );
        }
        printf("print out compl_arr\n");
        for (int i=0; i<*cap; ++i ) {
            printf("on idx=%d is %p\n", i, *(compl_arr+i) );
        }
        
    } */

    //add number to array:
    *(compl_arr + occupied) = new_cn;
   
}

//create a complex number
complex* compl_init(complex** all_compl_nbrs, int* cap) {
    complex* cn = (complex*)malloc(sizeof(complex));
    if(cn == NULL) {
        exit(EXIT_FAILURE);
    }
    add_to_compl_arr(all_compl_nbrs, cap, cn);
    return cn;
}

//free all complex numbers
void free_complex_array(complex** all_compl_nbrs) {
    int cnt = 0;
    while (*(all_compl_nbrs + cnt)!=NULL) {
        free( *(all_compl_nbrs+cnt) );
        cnt++;
    }
    free(all_compl_nbrs);
}

//MATH
//abs value
double abs_val(complex* cn) {
    return sqrt(cn->a*cn->a + cn->b*cn->b);
}

//complex square
complex* compl_square(complex* cn, complex** all_compl_nbrs, int* cap) {    
    complex* ret = compl_init(all_compl_nbrs, cap);
    ret->a = cn->a*cn->a - cn->b*cn->b;      //a^2 - b^2
    ret->b = 2*cn->a*cn->b;                  //2abi
    return ret;
}
//complex addition
complex* compl_add(complex* cn1, complex* cn2, complex** all_compl_nbrs, int* cap) {
    complex* ret = compl_init(all_compl_nbrs, cap);
    ret->a = cn1->a + cn2->a;
    ret->b = cn1->b + cn2->b;
    return ret;
}

//
int find_k(complex* cn_init, complex* c, int n) {
    int k = 0;
    int cap = 2*n + 10;
    complex** numbers = (complex**) calloc(cap, sizeof(complex));
    if (numbers == NULL) {
        exit(EXIT_FAILURE);
    }

    complex* z = compl_init(numbers, &cap);
    z->a = cn_init->a;
    z->b = cn_init->b;

    while ( (abs_val(z) < CONV_LIMIT) && (k < n) ){
        complex* new_z = compl_square(z, numbers, &cap);
        z = compl_add(new_z , c, numbers, &cap);
        //printf("k=%d, abs_val= %f\n",k , abs_val(z) );
        k++;
    }

    free_complex_array(numbers);

    return k;
}

