#ifndef __COMPL_MATH_H__
#define __COMPL_MATH_H__

#define RGB 3
#define SLEEP_TIME 10
#define CONV_LIMIT 2    // convergence limit
#define INIT_CAP 100
#define R(t) 9*(1-t)*t*t*t *255
#define G(t) 15*(1-t)*(1-t)*t*t*255
#define B(t) 8.5*(1-t)*(1-t)*(1-t)*t*255

typedef struct {
    double a;
    double b;
} complex;

unsigned char* create_blank(int w, int h) ;
void add_to_compl_arr(complex**compl_arr, int* cap, complex* new_cn);
complex* compl_init(complex** all_compl_nbrs, int* cap);
void free_complex_array(complex** all_compl_nbrs);
double abs_val(complex* cn);
complex* compl_square(complex* cn, complex** all_compl_nbrs, int* cap);
complex* compl_add(complex* cn1, complex* cn2, complex** all_compl_nbrs, int* cap);
int find_k(complex* cn_init, complex* c, int n);
double* create_t_array(int n, complex* c, int w, int h, complex* bound1, complex* bound2);
unsigned char* create_image(int w, int h, double* t_arr);

#endif
