#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

/* File contains useful set functions that can be used in conjunction with the Generic data structures */


/***** Set of useful functions for using hashmap/hashset with primitive types ******/

/* Useful Hash function for hashing strings */
unsigned int djb2_string_hash(const char *str) {
    assert(str);
    unsigned int hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

/* Hash function for integers */
unsigned int hash_int(const int *integer) {
    assert(integer);
    const unsigned int A = 2654435769;  // A is a constant fraction close to (sqrt(5) - 1) / 2
    return (unsigned int)(A * (*integer));
}

/* Compares if two strings are equal */
bool strings_equal(const char* str1, const char* str2) {
    if(!str1 && !str2) return true;
    if(!str1 || !str2) return false;

    int i=0;
    for(; str1[i] != '\0' && str2[i] != '\0'; i++) {
        if(str1[i] != str2[i]) {
            return false;
        }
    }

    if(str1[i] == '\0' && str2[i] == '\0') {
        return true;
    } else {
        return false;
    }
}

/* Gets the length of the argument list (ends with a NULL pointer)*/
int get_pointer_list_length(void **args)
{
    int length = 0;
    for (int i = 0; args[i] != NULL; i++)
        length++;
    return length;
}

/* Compares if two integers are equal */
bool integers_equal(const int *integer1, const int *integer2) {
    if (integer1 == NULL || integer2 == NULL) return false;
    return (*integer1) == (*integer2);
}

/* Returns a function that will check if int is larger/equal to the 'cutoff' */

typedef bool (*IntFilter)(const int*);
static int _compare_val = 0;
static bool _integer_filter(const int *integer) { return (*integer) >= _compare_val;}
IntFilter integer_bge_than(int cutoff) {
    _compare_val=cutoff;
    return _integer_filter;
}