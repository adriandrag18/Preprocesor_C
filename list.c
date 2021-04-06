#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"

char *folders[100];
int folders_size;
LIST *code;

void add_folders(char *folder, int size)
{
	folders[folders_size] = malloc(size);
	DIE(folders[folders_size]);
	strcpy(folders[folders_size], folder);
	folders_size++;
}

void add_line_code(char *line)
{
	LIST *new_element = malloc(sizeof(LIST));
	LIST *p = code;

	if (strcmp(line, "\n") == 0) {
		free(new_element);
		return;
	}
	DIE(new_element);
	new_element->line = malloc(strlen(line) + 1);
	DIE(new_element->line);
	strcpy(new_element->line, line);
	new_element->next = NULL;
	if (!code) {
		code = new_element;
	} else {
		while (p->next)
			p = p->next;
		p->next = new_element;
	}
}

void print_code(FILE *fp)
{
	LIST *p;

	if (code) {
		for (p = code; p; p = p->next)
			fprintf(fp, "%s", p->line);
	}
}

void free_lists(void)
{
	LIST *p, *aux;
	int i;

	if (code) {
		p = code;
		while (p) {
			aux = p;
			p = p->next;
			free(aux->line);
			free(aux);
		}
	}

	for (i = 0; i < folders_size; i++)
		free(folders[i]);
}
