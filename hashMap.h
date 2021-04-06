#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct hashMap {
	char *symbol;
	char *mapping;
	struct hashMap *next;
} HashMap;

extern HashMap **hash_map;
extern unsigned long  size;
extern unsigned long  *indices;
extern unsigned long  len;

extern unsigned long  hash(unsigned char *str);
extern void insert(char *symbol, char *mapping);
extern char *search(char *symbol);
extern void remove_hash_map(char *symbol);
extern void print_hash_map(void);
extern void free_hash_map(void);

#endif
