#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NEWLINE 10
#define SPACE 32
#define STAR 42
#define PLUS 43
#define MINUS 45
#define DEF_SIZE 10000
#define MAX_TYPES 26
#define SEMICOLON 59
#define ASCII_A 65
#define ASCII_Z 90

#define ERROR_INPUT 100

// BONUS start

typedef struct {
    char name;
    int n;      // number of rows
    int m;      // number of collumns
    int* values;
} mat;

mat* mat_init();
void free_mat_types(mat** mat_types);
void free_types_values(int** type_values);
bool read_mat_type(mat** mat_types, int** type_values);
void build_calculation(int** all_mats, int** all_dims, char* all_signs, mat** mat_types,
                        int* mat_cnt, int* sign_cnt, int* mult_cnt);

void free_everything(int** all_mats, int** all_dims, char* all_signs, mat** mat_types, int** type_values);
void print_formatted_matrix(int* mat, int n, int m);

// BONUS end

int* read_and_alloc(int* ret, int** all_sizes);

void print_matrix(int* mat, int n, int m) {
    printf("%d %d\n", n, m);
    for(int i=0; i<n; i++) {
        for(int j=0; j<m; j++) {
            printf("%d", *(mat+ i*m + j));
            if (j< (m-1) ) {
                printf(" ");
            } else {
                printf("\n");
            }
        }
    }
}

void free_all(int** matrix_list, int** all_sizes, char* signs) {
    int cnt = 0;
    while (*(matrix_list+cnt) != NULL) {
        free(*(matrix_list+cnt));
        cnt++;
    }

   /*  for (int i= 0; i<5; i++) {
        printf("matrix_arr= %p\n", *(matrix_list+i));
    }  */

    free(matrix_list);
    int cnt2 = 0;

    while (*(all_sizes+cnt2) != NULL) {
        free(*(all_sizes+cnt2));
        cnt2++;
    }
    free(all_sizes);

    free(signs);
}

void check_dimensions(char operation, int* dims1, int* dims2, int* ret) {
    // in case of + or - 
    if (operation==PLUS || operation==MINUS ) {
        if ( dims1[0] != dims2[0] || dims1[1] != dims2[1]) {
            //printf("incorrect dims for operation\n");
            *ret = ERROR_INPUT;
        } else {
            //printf("correct dimensions\n");
        }
    } 
    // in case of multiplication
    else {
        if (dims1[1] != dims2[0]) {
            //printf("incorrect dims for operation\n");
            *ret = ERROR_INPUT;
        } else {
            //printf("correct dimensions\n");
        }
    }
}

void add_or_subtract(int* mat1, int* mat2, int* dims1, char op, int** all_mats, int** all_sizes) {
    int* res_mat= NULL;
    int* res_dims = NULL;
    int n= dims1[0];
    int m= dims1[1];
    res_mat = (int*) malloc( n*m*sizeof(int) );
    res_dims = (int*) malloc( 2*sizeof(int) );
    int sign;

    if (op== PLUS) {
        sign = 1;
    } else {
        sign = -1;
    }

    for (int i=0; i<n; i++) {
        for (int j=0; j<m; j++) {
            *(res_mat + i*m +j ) = *(mat1 + i*m +j ) + sign* (*(mat2 + i*m +j));
        }
    }
    
    res_dims[0] = n;
    res_dims[1] = m;

    // free and replaces matrices
    free(*(all_mats) );
    free(*(all_mats + 1) );

    *(all_mats) = res_mat;
    *(all_mats+ 1) = NULL;

    // free and replace sizes
    free(*(all_sizes) );
    free(*(all_sizes + 1) );

    *(all_sizes) = res_dims;
    *(all_sizes + 1)= NULL;

}

void multiplication(int* mat1, int* mat2, int* dims1, int* dims2, int**all_mats, int** all_sizes, char* all_signs, int mat1_indx) {
    int comdim= dims1[1];  // common dimension, comdim = m1 = n2
    //printf("comdim= %d\n", comdim);
    int n1= dims1[0];
    int m2= dims2[1];
    int* res_mat= NULL;
    int* res_dims= NULL;
    res_mat = (int*) malloc( n1*m2*sizeof(int) );
    res_dims = (int*) malloc( 2*sizeof(int) );

    for (int i=0; i< n1; i++) {
        for (int j=0; j< m2; j++) {
            int element = 0;
            for (int k=0; k<comdim; k++) {
                //printf("part %d of element %d,%d = %d*%d\n", k, i, j, *(mat1 + i*comdim + k), (*(mat2 + k*m2 + j)));
                element += *(mat1 + i*comdim + k) * (*(mat2 + k*m2 + j));
            }
            *(res_mat + i*m2 +j ) = element;
        }
    }

    res_dims[0] = n1;
    res_dims[1] = m2;

    // free and replaces matrices
    free(*(all_mats + mat1_indx) );
    free(*(all_mats + mat1_indx + 1) );

    *(all_mats + mat1_indx) = res_mat;
    *(all_mats + mat1_indx + 1) = NULL;

    // free and replace sizes
    free(*(all_sizes + mat1_indx) );
    free(*(all_sizes + mat1_indx + 1) );

    *(all_sizes + mat1_indx) = res_dims;
    *(all_sizes + mat1_indx + 1)= NULL;

    // replace sign
    *(all_signs + mat1_indx) = '\0';
}

void shift_sign_arr(char* sign_arr) {
    for (int i=0; i< (DEF_SIZE - 1) ; i++) {
        if ( *(sign_arr + i) == '\0' &&  *(sign_arr + i +1) != '\0') {
            *(sign_arr + i) = *(sign_arr + i +1);
            *(sign_arr + i +1) = '\0';
        }
    }
}

void shift_ptr_arr(int** ptr_arr) {
    for (int i=0; i< (DEF_SIZE - 1) ; i++) {
        if ( *(ptr_arr + i) == NULL &&  *(ptr_arr + i +1) != NULL) {
            *(ptr_arr + i) = *(ptr_arr + i +1);
            *(ptr_arr + i +1) = NULL;
        }
    }
}

int main() {
    int ret = EXIT_SUCCESS;
    int** all_dims= (int**)malloc(DEF_SIZE*sizeof(int*));
    int** all_mats = (int**)malloc(DEF_SIZE*sizeof(int*));
    char* all_signs = (char*)malloc(DEF_SIZE*sizeof(char));

    for (int i= 0; i<DEF_SIZE; i++) {
        *(all_dims+i) = NULL;
        *(all_mats+i) = NULL;
        *(all_signs+i) = '\0';
    } 

    mat** mat_types = (mat**)calloc(MAX_TYPES, sizeof(mat*));
    int** types_values = (int**)calloc(MAX_TYPES, sizeof(int*));
    
    while (read_mat_type(mat_types, types_values)) {}
    
    // Read everything
    int mat_cnt = 0;
    int sing_cnt =0;
    int mult_cnt =0;
    build_calculation(all_mats, all_dims, all_signs, mat_types, &mat_cnt, &sing_cnt, &mult_cnt);
    /*
    // this while loop reads ONE matrix (with its dimensions) AND the following sign (if present) in ONE iteration. 
    while(!feof(stdin)) {
        *(all_mats + mat_cnt)= read_and_alloc(&ret, all_dims);
        if (ret != EXIT_SUCCESS) {
            goto ERROR_SKIP;
        }
        mat_cnt++;
        
        if (feof(stdin)) {
            break;
        }

        *(all_signs + sing_cnt) = getchar();

        if ( *(all_signs + sing_cnt) == STAR ) {
            mult_cnt ++;
        }

        sing_cnt++;
        getchar();

    }
    */

    //printf("sign_cnt= %d\n", sing_cnt);
    *(all_signs + sing_cnt) = '\0';
    sing_cnt --;

    //Multiplication
    for (int i= 0; i < mult_cnt; i++) {
        // find star
        int star_indx = 0;
        while (*(all_signs+star_indx) != STAR ) {
            star_indx++;
        }
        //check dimensions 
        check_dimensions(*(all_signs+star_indx), *(all_dims+star_indx), *(all_dims+star_indx+1), &ret);
        if (ret != EXIT_SUCCESS) {
            goto ERROR_SKIP;
        }
        //multiply
        multiplication( *(all_mats + star_indx), 
                        *(all_mats + star_indx +1),
                        *(all_dims+star_indx),
                        *(all_dims+star_indx+1),
                        all_mats,
                        all_dims,
                        all_signs,
                        star_indx);
        

        shift_sign_arr(all_signs);
        shift_ptr_arr(all_mats);
        shift_ptr_arr(all_dims);
    }

    // Add and/or subtract
    while ( *(all_signs) == PLUS || *(all_signs) == MINUS) {
        //check dimensions
        check_dimensions(*(all_signs), *(all_dims), *(all_dims+1), &ret);
        if (ret != EXIT_SUCCESS) {
            goto ERROR_SKIP;
        }
        // add or subtrarct
        add_or_subtract(*(all_mats), 
                        *(all_mats+1),
                        *(all_dims),
                        *all_signs,
                        all_mats,
                        all_dims );

        // remove sign
        *(all_signs) = '\0';

        shift_sign_arr(all_signs);
        shift_ptr_arr(all_mats);
        shift_ptr_arr(all_dims);
    }
    
    //Print result
    //print_matrix(*all_mats, **(all_dims), *(*(all_dims)+1));
    print_formatted_matrix(*all_mats, **(all_dims), *(*(all_dims)+1));
    
    ERROR_SKIP:
    free_everything(all_mats, all_dims, all_signs, mat_types, types_values);
    //printf("final ret= %d\n", ret);
    if (ret != EXIT_SUCCESS) {
        fprintf(stderr, "Error: Chybny vstup!\n");
    }
    return ret;
} 
 
int* read_and_alloc(int* ret, int** all_sizes) {
    // n = number of rows
    // m = number of collumns
    int n, m;
    int* mat= NULL;
    if ( scanf("%d%d", &n, &m) == 2 && n > 0 && m>0) {
    } else {
        *ret = ERROR_INPUT;
        goto SKIP;
    }

    char ch;
    if (*ret == EXIT_SUCCESS) {
        mat = (int*) malloc(n*m*sizeof(int));
        if (mat==NULL) {
            fprintf(stderr, "ERROR: Memory malloc error.\n");
            exit(EXIT_FAILURE);
        }
        for (int row= 0; row < n; row++) {
            for (int col=0; col<m; col++) {
                scanf("%d", (mat+row*m+col));
                ch = getchar();
                if (col != m-1 && ch!=SPACE) {
                    //printf("wrong number of elements m\n");
                    *ret = ERROR_INPUT;
                    goto SKIP;
                }
            }

        }

    }
    SKIP:
    if ( *ret!=EXIT_SUCCESS && mat!= NULL ) {
        free(mat);
        mat = NULL;
    }
    
    // write the size dimension
    int ptr_cnt = 0;
    while (*(all_sizes + ptr_cnt) != NULL)
    {
        ptr_cnt++;
    }
    *(all_sizes + ptr_cnt) = (int*)malloc(2*sizeof(int));
    **(all_sizes + ptr_cnt) = n;
    *(*(all_sizes + ptr_cnt)+1) = m;
    return mat;
}


// BONUS start


mat* mat_init() {
    mat* matrix = (mat*) malloc (sizeof(mat));
    if (matrix == NULL) {
        fprintf(stderr, "ERROR: Memory malloc error.\n");
        exit(EXIT_FAILURE);
    }
    return matrix;
}

void free_mat_types(mat** mat_types) {
    for(int i= 0; i < MAX_TYPES; ++i) {
        if ( *(mat_types + i)!= NULL ) {
            free( ((*(mat_types + i))->values) );
            free(*(mat_types + i));
        }
    }
    free(mat_types);
}

void free_types_values(int** type_values) {
    for(int i= 0; i < MAX_TYPES; ++i) {
        if ( *(type_values + i)!= NULL ) {
            free(*(type_values+ i));
        }
    }
    free(type_values);
}

void free_everything(int** all_mats, int** all_dims, char* all_signs, mat** mat_types, int** type_values) {
    //type values
    for(int i= 0; i < MAX_TYPES; ++i) {
        if ( *(type_values + i)!= NULL ) {
            //printf("freeing type_values %d\n", i);
            free(*(type_values+ i));
        }
    }
    free(type_values);
    //mat types
    for(int i= 0; i < MAX_TYPES; ++i) {
        if ( *(mat_types + i)!= NULL ) {
            //free( ((*(mat_types + i))->values) );
            //printf("freeing mat_types %d\n", i);
            free(*(mat_types + i));
        }
    }
    free(mat_types);
    
    //original arrays:
    for(int i= 0; i < DEF_SIZE; ++i) {
        if ( *(all_mats + i)!= NULL ) {
            //printf("freeing all_mats %d\n", i);
            free(*(all_mats + i));
        }
    }
    free(all_mats);

    for(int i= 0; i < DEF_SIZE; ++i) {
        if ( *(all_dims + i)!= NULL ) {
            //printf("freeing all_dims %d\n", i);
            free(*(all_dims + i));
        }
    }
    free(all_dims);

    free(all_signs);
}


bool read_mat_type(mat** mat_types, int** type_values) {
    bool ret = true;
    char c;
    c = getc(stdin);
    if (c == NEWLINE) { // if the first character on a line is NEWLINE, than it`s not a matrix
        ret = false;
        goto NOT_A_MATRIX; 
    }

    mat* mat_type = mat_init();      
    mat_type->name= c;      // put the matrix name into structure
    
    int capacity = DEF_SIZE;
    int* values = (int*)malloc(DEF_SIZE*sizeof(int));
    if (values == NULL) {
        fprintf(stderr, "ERROR: Memory malloc error.\n");
        exit(EXIT_FAILURE);
    }
    
    int occupied = 0;
    int row_cnt = 1;
    while (c!= NEWLINE) {
        if (c > 44 && c < 58) { // case: number or a minus sign
            //push the number into type_values array:
            ungetc(c, stdin);

            int temp;
            scanf("%d", &temp);
            *(values + occupied) = temp;
            occupied++;
            //check for capacity limit:
            if (occupied == capacity) {
                capacity *= 2;
                values = (int*)realloc(values, capacity*sizeof(int));
                if (values == NULL) {
                    fprintf(stderr, "ERROR: Memory malloc error.\n");
                    exit(EXIT_FAILURE);
                }
            }
        } else if (c == SEMICOLON ) {
            row_cnt++;
        }
        c = getc(stdin);
    }

    mat_type->values = values;
    mat_type->n = row_cnt;
    mat_type->m = (int) (occupied/row_cnt);
    
    // write down the matrix type:
    int cnt = 0;
    while (*(mat_types + cnt) != NULL ) {
        cnt++;
    }
    *(mat_types + cnt) = mat_type;
    
    // write down the values of the matrix type:
    cnt = 0;
    while (*(type_values + cnt) != NULL ) {
        cnt++;
    }
    *(type_values + cnt) = values;

    NOT_A_MATRIX:
    return ret;
}


void build_calculation(int** all_mats, int** all_dims, char* all_signs, mat** mat_types,
                        int* mat_cnt, int* sign_cnt, int* mult_cnt) {
    //read the last line:
    char c;
    while(!feof(stdin)) {
        c= getc(stdin);
        switch (c) {
        case PLUS:
            *(all_signs + *sign_cnt) = PLUS;
            (*sign_cnt)++;
            break;
        case MINUS:
            *(all_signs + *sign_cnt) = MINUS;
            (*sign_cnt)++;
            break;
        case STAR:
            *(all_signs + *sign_cnt) = STAR;
            (*mult_cnt)++;
            (*sign_cnt)++;
            break;
        default:
            if (c >= ASCII_A && c<= ASCII_Z) {
                // FIND OUT the index of matrix with the name c
                int matrix_idx = 0;
                while ( (mat_types[matrix_idx])->name != c ) {
                    matrix_idx++;
                }
                //put values of the selected matrix type into all_mats array
                /*
                *(all_mats + (*mat_cnt) ) = (*(mat_types + matrix_idx))->values;
                (*mat_cnt)++;
                */
                int amount_of_values= ((mat_types[matrix_idx])->n) * ((mat_types[matrix_idx])->m);
                int* values_copy = (int*) malloc(amount_of_values*sizeof(int));
                for(int i=0; i < amount_of_values; ++i) {
                    *(values_copy + i) = (mat_types[matrix_idx]->values)[i];
                }
                *(all_mats + (*mat_cnt) ) = values_copy;
                //put dimensions of the selected matrix type into all_dims array 
                *(all_dims + (*mat_cnt) ) = (int*)malloc(2*sizeof(int));
                **(all_dims + (*mat_cnt)) = (mat_types[matrix_idx])->n;
                *(*(all_dims+ (*mat_cnt))+1) = (mat_types[matrix_idx])->m;
                (*mat_cnt)++;
            }
            break;
        }
    }
}

void print_formatted_matrix(int* mat, int n, int m) {
    printf("[");
    for(int i = 0; i < n; ++i) {
        for (int j=0; j < m; ++j ) {
            printf("%d", *(mat + m*i + j ));
            if (j != (m-1)) {
                printf(" ");
            } else if( j==(m-1) && i!=(n-1)) {
                printf("; ");
            }
        }
    }
    printf("]\n");
}

// BONUS END
