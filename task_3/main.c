#include <stdio.h>
#include <stdlib.h>

#define ASCII_A 65
#define ASCII_Z 90
#define ASCII_A_LOWER 97
#define ASCII_Z_LOWER 122
#define INT_LINE_LENGTH 10 // deafult (initital) length of line in bytes
#define LETTERS_IN_ALPHABET 26
#define LOWERCASE_CONVERSION 70
#define UPPPERCASE_CONVERSION 64

enum {
    ERROR_INPUT = 100,
    ERROR_LENGTH = 101
};

char* read_input_line(int* line_length, int* ret_);
int letter_to_number(char letter);
char number_to_letter(int number);
char rotate(char original, int offset);
void shift(char* encoded_p, char* tmp_p, int length, int offset);
int compare(char* translated_p, char* tmp_p, int length);
int find_best_offset(char* encoded_p, char* translated_p, char* tmp_p, int length);
void report_error(int error);
void free_memory(char* encoded_p, char* translated_p, char* tmp_p);

int main () {
    int ret = EXIT_SUCCESS;
    int line_length_1 = 0;
    int line_length_2 = 0;
    char* encoded = NULL;          
    char* translated = NULL;
    char* tmp = NULL;

    // test for ERROR_INPUT:
    encoded = read_input_line( &line_length_1, &ret);
    if ( ret == EXIT_SUCCESS) {
        translated = read_input_line( &line_length_2, &ret);
    } 

    // test for ERROR_LENGTH:
    if ( ret == EXIT_SUCCESS && line_length_1 != line_length_2) {
        ret = 101;
    } 

    // In case of no error, do this:
    if ( ret == EXIT_SUCCESS) {
       tmp = (char*)calloc(line_length_1, sizeof(char));
       if (tmp == NULL) {
            fprintf(stderr, "Error: Malo pameti!\n");
            exit(102);
        }

       int result = find_best_offset(encoded, translated, tmp, line_length_1 );
       shift(encoded, tmp, line_length_1, result);

       char* tmp_copy = tmp;
       for (int i = 0; i < line_length_1; ++i ) {
            printf("%c", *tmp_copy);
            ++tmp_copy;
       }
       printf("\n");
    }

    free_memory(encoded, translated, tmp);
    if ( ret != EXIT_SUCCESS ) {
        report_error(ret);
    }
    return ret;
}

//DEFINITIONS OF FUNCTIONS

// reads ONE STDIN LINE until it reads non-alpha character (expects to find \n)
// if a non-alpha character is found that is not \n , edit ret value (input is incorrect)
// stores the characters into an array 
char* read_input_line(int* line_length, int* ret_) { 
    char* line = (char* )malloc(INT_LINE_LENGTH); // this pointer will be returned
    if (line == NULL) {
        fprintf(stderr, "Error: Malo pameti!\n");
        exit(102);
    }
    char c;
    int capacity = INT_LINE_LENGTH;

    scanf("%c", &c);
    while ( ( c>=ASCII_A && c<=ASCII_Z ) || ( c>=ASCII_A_LOWER && c<=ASCII_Z_LOWER )  ) {
        if ( *line_length == capacity) {
            capacity *= 2;
            line = realloc(line, capacity);
            if (line == NULL) {
                fprintf(stderr, "Error: Malo pameti!\n");
                exit(102);
            }
        }
        line[*line_length] = c;
        ++*line_length;
        scanf("%c", &c);
    }

    if ( c != '\n') {
        *ret_ = ERROR_INPUT; 
    }
    return line;
}

// Helper function for function named rotate.
// converts ASCII char value to a number between 1 and 52
int letter_to_number(char letter) {
    int number;
    if (letter >= ASCII_A && letter <= ASCII_Z) {
        number = letter - UPPPERCASE_CONVERSION;
    } else {
        number = letter - LOWERCASE_CONVERSION;
    }
    return number;
}

// Helper function for function named rotate.
// converts a number between 1 and 52 to ASCII char value
char number_to_letter(int number) {
    char letter;
    if (number <= LETTERS_IN_ALPHABET) {
        letter = number + UPPPERCASE_CONVERSION;
    } else {
        letter = number + LOWERCASE_CONVERSION;
    }
    return letter;
}

// rotates original letter by given offset
char rotate(char original, int offset) {
    char rotated;
    int num = letter_to_number(original);
    num = num + offset;
    if (num > 2*LETTERS_IN_ALPHABET) {
        num = num - 2*LETTERS_IN_ALPHABET;
    }
    rotated = number_to_letter(num);
    return rotated;
}

// fills a temporary array with letters rotated by a given offset
void shift(char* encoded_p, char* tmp_p, int length, int offset) {
    char* encoded_p_copy = encoded_p;
    char* tmp_p_copy = tmp_p;
    for(int i = 0; i <  length; ++i) {
        *tmp_p_copy = rotate(*encoded_p_copy, offset);
        ++ tmp_p_copy;
        ++ encoded_p_copy;
    }
}

// compares two strings of the same length
// returns the number of identical characters on identical positions
int compare(char* translated_p, char* tmp_p, int length) {
    char* translated_p_copy = translated_p;
    char* tmp_p_copy = tmp_p; 
    int cnt = 0;
    for (int i = 0; i < length ; ++i) {
        if (*translated_p_copy == * tmp_p_copy) {
            cnt++;
        }
        ++translated_p_copy;
        ++tmp_p_copy;
    } 
    return cnt;
}

// checks EVERY possible cipher rotation (=offset)
// returns the best offset, i.e. when there are the most matches of characters in arrays
int find_best_offset(char* encoded_p, char* translated_p, char* tmp_p, int length) {
    int most_matches = 0;
    int matches;
    int best_offset= 0;
    for (int i = 0; i < 2 * LETTERS_IN_ALPHABET; ++i) {
        shift(encoded_p, tmp_p, length, i);
        matches = compare(translated_p, tmp_p, length);
        if ( matches > most_matches) {
            most_matches = matches;
            best_offset = i;
        }
    }
    return best_offset;
}

// prints an error message based on a return error value to stderr
void report_error(int error) {
    switch (error) {
    case ERROR_INPUT:
        fprintf(stderr, "Error: Chybny vstup!\n");
        break;
    case ERROR_LENGTH:
        fprintf(stderr, "Error: Chybna delka vstupu!\n");
        break;
    }
}

// frees heap memory that needs to be freed
void free_memory(char* encoded_p, char* translated_p, char* tmp_p) {
    free(encoded_p);
    if (translated_p != NULL) {
        free(translated_p);
    }
    if (tmp_p != NULL ) {
        free(tmp_p);
    }
}
