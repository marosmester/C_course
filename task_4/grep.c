#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 100  // default size
#define QSTN_MARK 63
#define ASTERISK 42
#define PLUS 43

unsigned long myStrLen(const char* str);
bool myStrCmp(char* str1, char* str2);
char* readFileLine(FILE* fp);
int readStdinLine2(char* line);
bool isSubstringInString(char* substr, char* str);

bool searchForStrings(char* substr, char* str, bool regex);
void removeGivenChar(char* pattern, char* str, int length, char symbol, bool include);
bool plusOrAsterixSearch(char* substr, char* str, char symbol);

int findSubstring(char* substr, char* str);
void coloredPrint(char* substr, char* str);
void coloredPrintAll(char* substr, char* str);

int main(int argc, char *argv[]) {
    int ret= EXIT_SUCCESS;
    FILE* fp;
    char* pattern;
    const char* fname = NULL;
    bool regex = false;
    bool clrs = false; 

    //Decide what to do based on input arguments
    if (myStrCmp(argv[1], "-E") ) {
        regex = true;
        pattern = argv[2];
        fname = argc > 3 ? argv[3] : NULL;
        if (fname != NULL) {
            fp = fopen(fname, "r");           // fp = file pointer                     
            if (fp == NULL) {
                fprintf(stderr, "Error: cannoot open file %s\n", fname);
            }
        }
    } else if (myStrCmp(argv[1], "--color=always")) {
        clrs = true;
        pattern = argv[2];
        fname = argc > 3 ? argv[3] : NULL;
        if (fname != NULL) {
            fp = fopen(fname, "r");           // fp = file pointer                     
            if (fp == NULL) {
                fprintf(stderr, "Error: cannoot open file %s\n", fname);
            }
        } 
    } else {
        pattern = argv[1];
        fname = argc > 2 ? argv[2] : NULL;
        if (fname != NULL) {
            fp = fopen(fname, "r");           // fp = file pointer                     
            if (fp == NULL) {
                fprintf(stderr, "Error: cannoot open file %s\n", fname);
            }
        }
    }

    bool found = false;
    char* line;
    if (fname!=NULL) {
        while (!feof(fp)) {
            line = readFileLine(fp);
            if ( searchForStrings(pattern, line, regex) ) {
                found = true;
                if (clrs) {
                    coloredPrintAll(pattern, line);
                } else {
                    printf("%s", line);
                }
            }
            free(line);
            // offset for not reading the entire line??
            int charac = fgetc(fp);
            if (charac!=-1) {
                fseek(fp, -1, SEEK_CUR);
            }
            //
        }
        fclose(fp);
    
    } else {
        while(1) {
            char* line = (char*)malloc(SIZE*sizeof(char));
            if (line == NULL) {
                fprintf(stderr, "Error: Malo pameti!\n");
                exit(101);
            }
            int res = readStdinLine2(line);
            bool terminal = res == -1 ? true : false;
            if (!terminal && isSubstringInString(pattern, line) ) {
                    found = true;
                    printf("%s", line);
                }
            free(line);
            if (terminal) {
                break;
            }
        }
    }
    
    if (!found) {
        ret = 1;
    }
    
    return ret;
}

// Calculates the length of string ('\0' is not included in the length)
unsigned long myStrLen(const char* str) {      
    unsigned long ret = 0;
    while (str && *(str++) != '\0' ) {
        ret += 1;
    }
    return ret;
}

// compares 2 strings 
// returns true if they are exactly the same, false otherwise
bool myStrCmp(char* str1, char* str2) {
    bool ret;
    unsigned long len1 = myStrLen(str1);
    unsigned long len2 = myStrLen(str2);
    int cnt = 0;
    if (len1 == len2) {
        for(int i=0; i< len1; i++) {
            if (str1[i] == str2[i]) cnt++;
        }
    } 

    if (cnt == len1 && cnt == len2) {
        ret = true;
    } else {
        ret = false;
    }
    return ret;
}

// Reads the whole ONE line in text file
// fills the char* pointer so that it points to the string representation of the line
char* readFileLine(FILE* fp) {
    char* line = (char*)malloc(SIZE*sizeof(char));
    if (line == NULL) {
        fprintf(stderr, "Error: Malo pameti!\n");
        exit(101);
    }
    unsigned long capacity = SIZE;

    char* return_value= fgets(line, capacity, fp);
    if (return_value==NULL) {
        return NULL;
    }
    unsigned long length =  myStrLen(line);

    if ( length == (capacity-1)  && line[length-1]!='\n' ) {
        while (line[length-1]!='\n') {
            capacity *= 2;
            line = (char*)realloc(line, capacity);
            if (line == NULL) {
                fprintf(stderr, "Error: Malo pameti!\n");
                exit(101);
            }
            fseek(fp, -length, SEEK_CUR);
            fgets(line, capacity, fp);
            length = myStrLen(line);
        }
    }
    return line;
}

// Reads the whole ONE line in stdin
// fills the char* pointer so that it points to the string representation of the line
int readStdinLine2(char* line) {
    unsigned long capacity = SIZE;

    char c = getchar();
    if (c == EOF) {
        return -1;
    }
    int cnt = 0;
    while (c != '\n') {
        line[cnt] = c;
        cnt++;
        c = getchar();
        if (cnt == capacity) {
            capacity *= 2;
            line = (char*)realloc(line, capacity);
            if (line == NULL) {
                fprintf(stderr, "Error: Malo pameti!\n");
                exit(101);
            }

        }
    }
    line[cnt] = c;
    line[cnt+1] = '\0';
    /*
    for (int i=0; i<= cnt+1; i++) {
        printf("%d ", line[i]);
    }
    printf("\n");
    */
    return 0;
}

// Checks whether substring is in a larger string
// returns true or false
bool isSubstringInString(char* substr, char* str) {
    //printf("beginning\n");
    //printf("substr = %s, str=%s\n", substr, str);
    int len1 = myStrLen(substr);      // length of substring
    int len2 = myStrLen(str);         // length of the line
    //printf("len1=%d, len2=%d \n", len1, len2);
    

    if ( (len2 - len1) >= 0 ) {
        for (int i = 0; i <= (len2 - len1); i++) {
            int cursor1 = 0;                // to index substring
            int cursor2 = i;                // to index string
            while ( str[cursor2] == substr[cursor1] ) {
                cursor1++;
                cursor2++;
                if (cursor1 == len1) {
                    break;
                }
            }
            if (cursor1 == len1) {
                //printf("returned true\n");
                //printf("end\n");
                return true;
            }
        }
    }
    //printf("returned false\n");
    //printf("end\n");
    return false;
}

void removeGivenChar(char* pattern, char* str, int length, char symbol, bool include) {
    int indx = 0;
    if (include) {
        for (int i=0; i < length; i++) {
            if ( pattern[i] != symbol) {
                str[indx] = pattern[i];
                indx++;                
            }
        }
    } else {
        int indx = 0;
        for (int i=0; i < length; i++) {
            if ( pattern[i+1] != symbol && pattern[i] != symbol) {
                str[indx] = pattern[i];
                indx++;
            }
        }
        str[length-2] = '\0';
    }
}

bool plusOrAsterixSearch(char* substr, char* str, char symbol) {
    //printf("beginning\n");
    //printf("substr = %s, str=%s\n", substr, str);
    int len1 = myStrLen(substr);      // length of substring
    int len2 = myStrLen(str);         // length of the line
    //printf("len1=%d, len2=%d \n", len1, len2);

    if ( (len2 - len1) >= 0 ) {
        for (int i = 0; i < (len2); i++) {
            //printf("i= %d\n", i);
            int cursor1 = 0;                // to index substring
            int cursor2 = i;                // to index string
            int precursor = 1;            // to search for special characters
            while ( str[cursor2] == substr[cursor1] && substr[cursor1]!='\0' ) {
                //printf("zhoda na i=%d\n", i);
                if (substr[precursor]==symbol) {
                    //printf("PRECURSOR ON +\n");
                    char tmp = str[cursor2];
                    while(str[cursor2]== tmp) {
                        //printf("char = %c\n", str[cursor2]);
                        cursor2++;
                    }
                    cursor1 = cursor1 + 2;
                    precursor = precursor + 2;
                } else{
                    cursor1++;
                    cursor2++;
                    precursor++;
                }
            }
            if (substr[cursor1]=='\0') {
                //printf("RETURNED TRUE\n");
                return true;
            }
        }
    }
    //printf("RETURNED FALSE\n");
    return false;
}

// IF regex == false then it calls simple isSubstringInString(substr, str)
// IF regex == true then searches str based on regex rules
bool searchForStrings(char* substr, char* str, bool regex) {
    bool searchResult;
    
    if (regex) {
        if ( isSubstringInString("?", substr) ) {
            int patternLength = myStrLen(substr);
            char tmp1[patternLength];
            char tmp2[patternLength + 1];
            removeGivenChar(substr, tmp1, patternLength, QSTN_MARK, false);
            removeGivenChar(substr, tmp2, patternLength+1, QSTN_MARK, true);
            searchResult = isSubstringInString(tmp1, str) || isSubstringInString(tmp2, str);
        } else if (isSubstringInString("+", substr)) {
            searchResult = plusOrAsterixSearch(substr, str, PLUS);
        } else {
            int patternLength = myStrLen(substr);
            char tmp1[patternLength];
            removeGivenChar(substr, tmp1, patternLength, ASTERISK, false);
            searchResult = isSubstringInString(tmp1, str) || plusOrAsterixSearch(substr, str, ASTERISK);
        }
        
    } else {
        searchResult = isSubstringInString(substr, str);
    }

    return searchResult;
}

int findSubstring(char* substr, char* str) {
    int indx;
    int len1 = myStrLen(substr);      // length of substring
    int len2 = myStrLen(str);         // length of the line
    //printf("len1=%d, len2=%d \n", len1, len2);
    

    if ( (len2 - len1) >= 0 ) {
        for (int i = 0; i <= (len2 - len1); i++) {
            int cursor1 = 0;                // to index substring
            int cursor2 = i;                // to index string
            while ( str[cursor2] == substr[cursor1] ) {
                indx = i;
                cursor1++;
                cursor2++;
                if (cursor1 == len1) {
                    break;
                }
            }
            if (cursor1 == len1) {
                // Success
                return indx;
            }
        }
    }
    // failed to find Substr in str entirely
    return -1;
}

void coloredPrintAll(char* substr, char* str) {
    int startIndx;
    int len1 = myStrLen(substr);      // length of substring
    int len2 = myStrLen(str);         // length of the line
    //printf("len1=%d, len2=%d \n", len1, len2);

    if ( (len2 - len1) >= 0 ) {
        for (int i = 0; i < len2 ; i++) {
            int cursor1 = 0;                // to index substring
            int cursor2 = i;                // to index string
            while ( str[cursor2] == substr[cursor1] ) {
                startIndx = i;
                cursor1++;
                cursor2++;
                if (cursor1 == len1) {
                    break;
                }
            }
            if (cursor1 == len1) {
                printf("\x1b[01;31m\x1b[K");
                int patternLength = myStrLen(substr);
                for (int j= 0; j<patternLength; j++) {
                    printf("%c", substr[j]);
                }
                printf("\x1b[m\x1b[K");
                i = i + patternLength -1;
            } else {
                printf("%c", str[i]);
            }
        }
    }
}
