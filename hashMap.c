#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashMap.h"
#include "helper.h"

unsigned long size;
unsigned long *indices;
unsigned long len;
HashMap **hash_map;

unsigned long hash(unsigned char *str)
{
	unsigned long hash = 381;

	while (*str)
		hash = hash * 13 + *str++;
	return hash;
}

void free_element(HashMap **p)
{
	free((*p)->symbol);
	free((*p)->mapping);
	free(*p);
	(*p) = NULL;
}


void insert(char *symbol, char *mapping)
{
	unsigned long  key = hash((unsigned char *) symbol) % size;
	HashMap *q, *p = malloc(sizeof(HashMap));

	DIE(p);
	remove_hash_map(symbol);
	p->symbol = malloc(strlen(symbol) + 1);
	DIE(p->symbol);
	strcpy(p->symbol, symbol);
	if (mapping) {
		p->mapping = malloc(strlen(mapping) + 1);
		DIE(p->mapping);
		strcpy(p->mapping, mapping);
	} else {
		p->mapping = malloc(1);
		DIE(p->mapping);
		strcpy(p->mapping, "");
	}
	p->next = NULL;

	if (!hash_map[key]) {
		hash_map[key] = p;
	} else  {
		q = hash_map[key];
		while (!q->next)
			q = q->next;
		q->next = p;
	}
	indices[len] = key;
	len++;
}

char *search(char *symbol)
{
	unsigned long key = hash((unsigned char *) symbol) % size;
	HashMap *p;

	if (!symbol)
		return NULL;
	if (hash_map[key]) {
		p = hash_map[key];
		if (strcmp(p->symbol, symbol) == 0)
			return p->mapping;
		p = p->next;
	}
	return NULL;
}

void remove_hash_map(char *symbol)
{
	unsigned long  i, key = hash((unsigned char *) symbol) % size;
	HashMap *aux, *p;

	if (hash_map[key] != NULL) {
		if (strcmp(hash_map[key]->symbol, symbol) == 0) {
			aux = hash_map[key]->next;
			free_element(&hash_map[key]);
			hash_map[key] = aux;
		} else {
			p = hash_map[key];
			while (p) {
				if (strcmp(p->next->symbol, symbol) == 0) {
					aux = p->next->next;
					free_element(&p->next);
					p->next = aux;
					break;
				}
				p = p->next;
			}
		}
		for (i = len - 1; indices[i] != key; )
			i--;
		indices[i] = indices[--len];
	}
}

void free_hash_map(void)
{
	unsigned long i;
	HashMap *aux, *p;

	for (i = 0; i < len; i++) {
		hash_map[indices[i]];
		if (!hash_map[indices[i]]) {
			continue;
		} else {
			aux = hash_map[indices[i]]->next;
			free_element(&hash_map[indices[i]]);
		}
	}
	free(hash_map);
	free(indices);
}
