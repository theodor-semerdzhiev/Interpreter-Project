#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "../generics/utilities.h"

/**
 * DESCRIPTION:
 * This file contains the implementation of the open file table.
 * This keeps track of the files opened during the runtime
 *
 * - So that they can be closed after program execution
 *
 * Each FILE is mapped to a unique identifier, the table will therefore NEVER have duplicate keys
 *
 * The table uses linear probing
 *
 */

#define DEFAULT_BUCKET_COUNT 100

typedef struct FileEntry {
    FILE *file;
    char *filename;
    size_t fileId;
    bool was_occupied;
} FileEntry;

static FileEntry *table = NULL;
static size_t table_length = DEFAULT_BUCKET_COUNT;

static size_t entry_count = 0;
static size_t file_counter = 0;

/**
 * DESCRIPTION:
 * Initializes file table
*/
void init_FileTable()
{
    if(table) return;

    table = malloc(sizeof(FileEntry) * DEFAULT_BUCKET_COUNT);
    // if (!table)
    //     MallocError();

    for(int i=0; i < DEFAULT_BUCKET_COUNT; i++) {
        table[i].file = NULL;
        table[i].was_occupied = false;
        table[i].fileId = 0;
        table[i].filename = NULL;
    }
    
    entry_count = 0;
    file_counter = 0;
    table_length = DEFAULT_BUCKET_COUNT;
}

/**
 * DESCRIPTION:
 * Closes all files in table and frees all memory
 * All static fields are reset 
*/
void cleanup_FileTable()
{
    if(!table) {
        table_length = 0;
        entry_count = 0;
        file_counter = 0;
        return;
    }

    for(size_t i=0; i < table_length; i++) {
        if(table[i].file) {
            fclose(table[i].file);
        }
        if(table[i].filename) {
            free(table[i].filename);
        }
    }
    free(table);
    table = NULL;
    table_length = 0;
    entry_count = 0;
    file_counter = 0;
}

/**
 * DESCRIPTION:
 * Resizes file table, if entries in the table is equal to the size of the table
*/
static void resize_filetbl(size_t newsize) {
    FileEntry* newtable = malloc(sizeof(FileEntry) * newsize);
    // if(!newtable)
    //     MallocError();
    for(size_t i=0; i < newsize; i++) {
        newtable[i].file = NULL;
        newtable[i].fileId = 0;
        newtable[i].was_occupied = false;
    } 
    
    for(size_t i=0; i < table_length; i++) {
        if(table[i].file) {
            size_t idx = table[i].fileId % newsize;
            newtable[idx].file = table[i].file;
            newtable[idx].fileId = table[i].fileId;
            newtable[idx].filename = table[i].filename;
            newtable[idx].was_occupied = table[i].was_occupied;
        }
    }

    free(table);
    table = newtable;
    table_length = newsize;
}

/**
 * DESCRIPTION:
 * Inserts file into file table and returns its mapped key 
 * The returned key will always be unique
*/
size_t filetbl_insert(FILE *file, const char *filename) {
    assert(file);

    // resizes table if necessary 
    if(entry_count == table_length) {
        resize_filetbl(table_length * 2);
    }

    size_t hash = file_counter % table_length;
    size_t idx;

    for(size_t i=0; i < table_length; i++) {
        idx = (i + hash) % table_length;
        if(!table[idx].file) {
            table[idx].file = file;
            table[idx].filename = cpy_string(filename);
            table[idx].was_occupied = true;
            table[idx].fileId = file_counter;
            break;
        }
    }

    entry_count++;
    file_counter++;
    return table[idx].fileId;
}

/**
 * DESCRIPTION:
 * Searches file table using fileId, and returns ONLY the FILE struct, NOT the filename
*/
FILE *filetbl_search(size_t fileId) {
    size_t hash = fileId % table_length;

    for(size_t i=0; i < table_length; i++) {
        size_t idx = (i + hash) % table_length;

        if(table[idx].file && table[idx].fileId == fileId)
            return table[idx].file;
        
        if(!table[idx].was_occupied) 
            return NULL;
    }

    return NULL;
}

/**
 * DESCRIPTION:
 * Searches file table using fileId, but returns ONLY the filename associated with the fileID
*/
const char* filetbl_search_filename(size_t fileId) {
    size_t hash = fileId % table_length;

    for(size_t i=0; i < table_length; i++) {
        size_t idx = (i + hash) % table_length;

        if(table[idx].file && table[idx].fileId == fileId)
            return table[idx].filename;
        
        if(!table[idx].was_occupied) 
            return NULL;
    }

    return NULL;
}


FILE *filetbl_remove(size_t fileId) {
    size_t hash = fileId % table_length;

    for(size_t i=0; i < table_length; i++) {
        size_t idx = (i + hash) % table_length;
        if(table[idx].file && table[idx].fileId == fileId) {
            FILE *file = table[idx].file;
            free(table[idx].filename);
            table[idx].filename = NULL;
            table[idx].file = NULL;
            table[idx].fileId = 0;
            table[idx].was_occupied = true;
            entry_count--;
            if( (entry_count == table_length / 2) && 
                (table_length / 2 >= DEFAULT_BUCKET_COUNT)) {
                resize_filetbl(table_length / 2);
            }
            return file;
        }

        if(!table[idx].was_occupied) {
            return NULL;
        }
    }

    return NULL;
}

bool filetbl_close(size_t fileId) {
    size_t hash = fileId % table_length;

    for(size_t i=0; i < table_length; i++) {
        size_t idx = (i + hash) % table_length;
        if(table[idx].file && table[idx].fileId == fileId) {
            fclose(table[idx].file);
            free(table[idx].filename);
            table[idx].filename = NULL;
            table[idx].file = NULL;
            table[idx].fileId = 0;
            table[idx].was_occupied = true;
            entry_count--;
            if( (entry_count == table_length / 2) && 
                (table_length / 2 >= DEFAULT_BUCKET_COUNT)) {
                resize_filetbl(table_length / 2);
            }
            return true;
        }

        if(!table[idx].was_occupied)
            return false;
    }
    return false;
}

#if 0
int main() {
    init_FileTable();
    for(int i=0; i < 50; i++) {
        filetbl_insert(fopen("main.c","r"));
    }

    filetbl_close(10);
    filetbl_insert(fopen("main.c","r"));


    for(size_t i=0; i < table_length; i++) {
        printf("%d: file:%p     key: %zu     reserved: %d \n", 
        i, table[i].file, table[i].fileId, table[i].was_occupied);
    }

    cleanup_FileTable();
}
#endif