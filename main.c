#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hashMap.h"
#include "list.h"
#include "helper.h"

#define DEFINE "#define"
#define UNDEF "#undef"
#define INCLUDE "#include"
#define IF "#if"
#define ELIF "#elif"
#define IFDEF "#ifdef"
#define IFNDEF "#ifndef"
#define ELSE "#else"
#define ENDIF "#endif"

FILE *fin, *fout;
int multiline_comment;
char *base_dir;

/* incearca sa deschida toate optiunile de concatenare dintre numele header */
/* si posibilele directoare in care se poate afla si returneaza FILE* ul */
/* care sa deshis in caz de succes sau NULL daca nu a fost gasit */
FILE *find_includes(char *include)
{
	FILE *fp = NULL;
	int i = 0,  size = strlen(base_dir), len = size + strlen(include) + 2;
	char *full_path = malloc(LINE_SIZE);
	
	DIE(full_path);

	if (*base_dir) {
		strcpy(full_path, base_dir);
		strcat(full_path, include);
		fp = fopen(full_path, "r");
	} else {
		strcpy(full_path, include);
		fp = fopen(full_path, "r");
	}

	while (!fp) {
		size = strlen(folders[i]);
		len = strlen(include) + size + 2;
		strcpy(full_path, folders[i]);
		strcat(full_path, "\\");
		strcat(full_path, include);
		fp = fopen(full_path, "r");
		if (!fp) {
			full_path[size] = '/';
			fp = fopen(full_path, "r");
		}
		i++;
		if (i >= folders_size)
			break;
	}
	free(full_path);
	return fp;
}

/* sparge linia in tokeni dupa caracterele "\t\n []{}<>=+-/%!&|^.,:;()\\\0" */
/* si reaza un nou string in care toti tokenii care nu sunt in interiorul */
/* sir si care se gasesc in hash_map */
char *include_define(char *line, char *new_line)
{
	char del[28] = "\t\n []{}<>=+-*/%!&|^.,:;()\\\0";
	char *word = malloc(100);
	char *mapping = NULL;
	int inside_string = 0;
	int index_word = 0;
	int index_new = 0;
	int inline_comment = 0, partial_comment = 0;
	int i;

	DIE(word);

	for (i = 0; line[i]; i++) {
		if (!inside_string && !inline_comment)
			if (!multiline_comment && line[i] == '/') {
				if (partial_comment) {
					partial_comment = 0;
					inline_comment = 1;
				} else {
					partial_comment = 1;
				}
			}
		if (!inside_string && !inline_comment && !multiline_comment)
			if (line[i] == '*' && partial_comment)
				multiline_comment = 1;

		if (multiline_comment && line[i] == '*' && line[i + 1] == '/')
			multiline_comment = 0;

		if (strchr(del, line[i])) {
			word[index_word] = 0;
			index_word = 0;
			mapping = search(word);
			if (!inside_string && !inline_comment &&
				!multiline_comment && mapping) {
				strcpy(new_line + index_new, mapping);
				index_new += strlen(mapping);
			} else {
				strcpy(new_line + index_new, word);
				index_new += strlen(word);
			}
			new_line[index_new++] = line[i];
			continue;
		}
		if (line[i] == '"') {
			if (inside_string && i >= 0 && line[i - 1] != '\\')
				inside_string = 0;
			else
				inside_string = 1;
		}

		word[index_word++] = line[i];
	}

	if (index_word) {
		word[index_word] = 0;
		strcpy(new_line + index_new, word);
		index_new += strlen(word);
	}
	free(word);
	new_line[index_new] = 0;
	return new_line;
}

/* parseaza conditia data de directiva #if si o evalueaza */
int evaluate_cond(char *cond)
{
	int is_num = 1, i = 0;
	char *mapping = NULL;

	for (; cond[i]; i++) {
		if (cond[i] == '\n') {
			cond[i] = '\0';
			break;
		}
		if (isdigit(cond[i]) == 0)
			is_num = 0;
	}
	if (is_num) {
		if (cond[0] == '0')
			return 0;
		else
			return 1;
	} else {
		mapping = search(cond);
		if (!mapping)
			return 0;
		else if (mapping[0] == '0')
			return 0;
		else
			return 1;
	}
}

int process(char *line);

/* primeste o linie care incepe cu directiva #define */
/* si o preoceseaza introducad symbolul si mapping-ul in hashmap */
/* in cazul in care mappind-ul contine tokeni care se afla in hashmap */
/* acestia sunt inlocuiti, procesul fiind recursiv pana cand sirul */
/* ramane neschimabt */
void define_func(char *line)
{
	char *symbol, *mapping, *new_mapping = NULL, *old_mapping = NULL;
	char c = line[strlen(line)-2];
	char *n_line = malloc(LINE_SIZE);
	int i = 0, len = LINE_SIZE;

	DIE(n_line);
	while (c == '\\') {
		line[strlen(line)-2] = '\0';
		fgets(n_line, len, fin);
		strcat(line, n_line + 1);
		c = line[strlen(line)-2];
	}
	symbol = line + 8;
	mapping = symbol;
	while (*mapping != ' ' && *mapping != '\0')
		mapping++;
	if (*mapping == ' ') {
		*mapping = '\0';
		mapping++;
	}
	if (*mapping == '\0') {
		while (symbol[i] != '\n')
			i++;
		symbol[i] = '\0';
	} else {
		while (mapping[i] != '\n')
			i++;
		mapping[i] = '\0';
		new_mapping = malloc(LINE_SIZE);
		DIE(new_mapping);
		old_mapping = malloc(LINE_SIZE);
		DIE(old_mapping);
		strcpy(old_mapping, mapping);
		include_define(mapping, new_mapping);

		while (strcmp(old_mapping, new_mapping) != 0) {
			strcpy(old_mapping, new_mapping);
			include_define(old_mapping, new_mapping);
		}
		free(old_mapping);
	}
	insert(symbol, new_mapping);
	if (new_mapping)
		free(new_mapping);
	free(n_line);
}

/* primieste o linie care incepe cu directiva #include si incearca sa */
/* fisierul si sa proceseze continutul */
void include_func(char *line)
{
	char *include = strtok(line, " ");
	int i = 0, len = LINE_SIZE;
	FILE *fp, *aux;

	include = strtok(NULL, " ") + 1;
	for (i = 0; include[i] != '"' && include[i] != '>'; )
		i++;
	include[i] = 0;
	fp = find_includes(include);
	if (!fp)
		exit(-1);

	aux = fin;
	fin = fp;
	while (fgets(line, len, fin) != NULL)
		process(line);
	fin = aux;
	fclose(fp);
}

/* primieste o linie care incepe cu directiva #undef si scoate din hashmap */
/* respectivul simbol */
void undefine_func(char *line)
{
	char *symbol = strtok(line, " ");
	int i = 0;

	symbol = strtok(NULL, " ");
	while (symbol[i] != '\n')
		i++;
	symbol[i] = '\0';
	remove_hash_map(symbol);
}

/* primieste o linie care incepe cu directiva #ifdef sau #indef si procesaza */
/* liniile din ramura care este adevarata si ignora liniile din cealalta */
/* pana la #endif*/
void ifdef_func(char *line)
{
	char *n_line = malloc(LINE_SIZE);
	char *define = strtok(line, " ");
	int i = 0, len = LINE_SIZE, result = -1;
	int is_ifdef = strncmp(line, IFDEF, 6) == 0 ? 1 : 0;

	DIE(n_line);
	define = strtok(NULL, " ");
	for (i = 0; define[i]; i++) {
		if (define[i] == '\n')
			define[i] = 0;
	}

	if ((is_ifdef && search(define)) ||
		(!is_ifdef && !search(define))) {
		while (result < 3 && fgets(n_line, len, fin) != NULL)
			result = process(n_line);
		/* ignor liniile dupa else pana gesesc un endif */
		if (result == 3)
			while (fgets(n_line, len, fin) != NULL)
				if (strncmp(n_line, ENDIF, 6) == 0)
					break;
	} else {
		/* ignor liniile pana gasesc un else sau endif */
		while (result < 3 && fgets(n_line, len, fin) != NULL) {
			if (strncmp(n_line, ELSE, 5) == 0)
				result = 3;

			if (strncmp(n_line, ENDIF, 6) == 0)
				result = 4;
		}
		/* daca gasesc un else procesez pana gasesc un endif */
		if (result == 3) {
			while (result < 4 &&
				fgets(n_line, len, fin) != NULL)
				result = process(n_line);
		}
	}
	free(n_line);
}

/* primieste o linie care incepe cu directiva #if si procesaza */
/* liniile din ramura care este adevarata si ignora liniile din cealalta */
/* pana la #endif*/
void if_func(char *line)
{
	char *cond = malloc(LINE_SIZE);
	char *n_line = malloc(LINE_SIZE);
	int i = 0, len = LINE_SIZE, result = 1;

	DIE(n_line);
	DIE(cond);
	strcpy(cond, line + 4);
	while (!evaluate_cond(cond)) {
		while (result < 2 && fgets(n_line, len, fin) != NULL) {
			if (strncmp(n_line, ELIF, 5) == 0)
				result = 2;
			if (strncmp(n_line, ELSE, 5) == 0)
				result = 3;
			if (strncmp(n_line, ENDIF, 6) == 0)
				result = 4;
		}
		if (result == 2) {
			strcpy(cond, n_line + 6);
			result = 1;
			continue;
		} else if (result == 3) {
			result = 1;
			break;
		}
		result = 0;
		break;
	}
	if (result == 1) {
		while (result < 2 && fgets(n_line, len, fin) != NULL)
			result = process(n_line);

		if (result == 3 || result == 2)
			while (fgets(n_line, len, fin) != NULL)
				if (strncmp(n_line, ENDIF, 6) == 0)
					break;
	}
	free(cond);
	free(n_line);
}

/* verifica daca linia contine vreo directiva daca nu proceseaza linia */
/* pentru a inlocui simbolurile care se gasesc in hashmap */
int process(char *line)
{
	char *new_line = NULL;

	if (strncmp(line, INCLUDE, 8) == 0) {
		include_func(line);
		return -1;
	}

	if (strncmp(line, DEFINE, 7) == 0) {
		define_func(line);
		return -1;
	}

	if (strncmp(line, UNDEF, 6) == 0) {
		undefine_func(line);
		return -1;
	}

	if (strncmp(line, IFDEF, 6) == 0 || strncmp(line, IFNDEF, 7) == 0) {
		ifdef_func(line);
		return -1;
	}

	if (strncmp(line, IF, 3) == 0) {
		if_func(line);
		return -1;
	}

	if (strncmp(line, ELIF, 5) == 0)
		return 2;

	if (strncmp(line, ELSE, 5) == 0)
		return 3;

	if (strncmp(line, ENDIF, 6) == 0)
		return 4;

	new_line = malloc(LINE_SIZE);
	DIE(new_line);
	include_define(line, new_line);
	add_line_code(new_line);
	free(new_line);

	return 1;
}

void init(void)
{
	len = 0;
	size = 1024;
	folders_size = 0;
	multiline_comment = 0;
	code = NULL;

	hash_map = malloc(sizeof(HashMap *) * size);
	DIE(hash_map);
	memset(hash_map, 0, sizeof(HashMap *) * size);

	indices = malloc(sizeof(int) * size);
	DIE(indices);
	memset(indices, -1, sizeof(int) * size);
}

/* primiste numele fisierului de intrarea si intoarea o copie pana la */
/* ultimul / sau sirul vid in caz ca nu exista */
char *find_base_dir(char *path)
{
	int len = strlen(path);
	int i = len;
	char *base_dir = malloc(len + 1);

	DIE(base_dir);
	while (i && path[i] != '\\' && path[i] != '/')
		i--;
	if (i) {
		strncpy(base_dir, path, i + 1);
		base_dir[i + 1] = '\0';
	} else {
		base_dir[0] = '\0';
	}
	return base_dir;
}

/* parseaza argumentele iar daca aceste nu sunt sub forma specificata in */
/* enunt intoarce eroare*/
void parse_arg(int argc, char **argv)
{
	char *in_file = NULL;
	char *out_file = NULL;
	char *symbol;
	char *mapping;
	char folder[LINE_SIZE];
	char define[LINE_SIZE];
	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strncmp(argv[i], "-D", 2) == 0) {
				if (*(argv[i] + 2))
					strcpy(define, argv[i] + 2);
				else
					strcpy(define, argv[++i]);
				symbol = strtok(define, "=");
				mapping = strtok(NULL, "=");
				insert(symbol, mapping);
				continue;
			}

			if (strncmp(argv[i], "-o", 2) == 0) {
				out_file = malloc(LINE_SIZE);
				DIE(out_file);
				if (*(argv[i] + 2))
					strcpy(out_file, argv[i] + 2);
				else
					strcpy(out_file, argv[++i]);
			}

			if (strncmp(argv[i], "-I", 2) == 0) {
				if (*(argv[i] + 2))
					strcpy(folder, argv[i] + 2);
				else
					strcpy(folder, argv[++i]);
				add_folders(folder, strlen(folder) + 1);
				continue;
			}
			exit(-1);
		}
		if (!in_file) {
			in_file = malloc(LINE_SIZE);
			DIE(in_file);
			strcpy(in_file, argv[i]);
		} else if (!out_file) {
			out_file = malloc(LINE_SIZE);
			DIE(out_file);
			strcpy(out_file, argv[i]);
		} else {
			exit(-1);
		}
	}

	fout = out_file ? fopen(out_file, "w") : stdout;
	if (in_file) {
		fin = fopen(in_file, "r");
		base_dir = find_base_dir(in_file);
	} else {
		fin = stdin;
		base_dir = malloc(1);
		DIE(base_dir);
		base_dir[0] = '\0';
	}

	if (!fin)
		exit(-1);
	if (!fout)
		exit(-1);

	if (in_file)
		free(in_file);
	if (out_file)
		free(out_file);
}

int main(int argc, char **argv)
{
	int len = LINE_SIZE;
	char *line = malloc(LINE_SIZE);

	DIE(line);
	init();
	parse_arg(argc, argv);

	while (fgets(line, len, fin) != NULL)
		process(line);

	print_code(fout);

	free_lists();
	free(base_dir);
	free(line);
	free_hash_map();
	fclose(fin);
	fclose(fout);
	return 0;
}
