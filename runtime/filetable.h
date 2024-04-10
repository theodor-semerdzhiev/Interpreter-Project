#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

void init_FileTable();
void cleanup_FileTable();
size_t filetbl_insert(FILE *file, const char *filename);
FILE *filetbl_search(size_t fileId);
const char* filetbl_search_filename(size_t fileId);
FILE *filetbl_remove(size_t fileId);
bool filetbl_close(size_t fileId);