#ifndef LIST_H
#define LIST_H

#include <stdio.h>

typedef struct List {
	char *line;
	struct List *next;
} LIST;

extern char *folders[100];
extern int folders_size;
extern LIST *code;

extern void add_folders(char *folder, int size);
extern void add_line_code(char *line);
extern void print_code(FILE *fp);
extern void free_lists(void);

#endif
