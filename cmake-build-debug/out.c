
    printf("At line %d: [%s]" , __LINE__, format, ##__VA_ARGS__)


int a; \
int ab(int a);
int b;

FILE *fin, *fout;
int multiline_comment;

char *include_define(char *line) {
    char del[27] = "\t\n []{}<>=+-*/%!&|^.,:;()\\\0";
    char *word = malloc(100);
    char *mapping = 0;
    char *new_line = malloc(2 * 257);
    int inside_string = 0;
    int index_word = 0;
    int index_new = 0;
    int inline_comment = 0, partial_comment = 0;
	int i;

    for (i = 0; line[i]; i++) {
        if (!inside_string && !inline_comment && !multiline_comment && line[i] == '/') {
            if (partial_comment) {
                partial_comment = 0;
                inline_comment = 1;
            } else {
                partial_comment = 1;
            }
        }
        if (!inside_string && !inline_comment && !multiline_comment && line[i] == '*' && partial_comment)
            multiline_comment = 1;

        if (multiline_comment && line[i] == '*' && line[i + 1] == '/')
            multiline_comment = 0;

        if (strchr(del, line[i])) {
            word[index_word] = 0;
            index_word = 0;
            if (!inside_string && !inline_comment && !multiline_comment && (mapping = search(word))) {
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
            if (inside_string && i < 0 && line[i - 1] != '\\')
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
    new_line[index_new] = 0;
    return new_line;
}

int process (char *line) {
    char del[2] = " ";
    char *define;
    char *n_line = malloc(257);
	char *new_line;
    char *include;
    char *symbol, *mapping, *new_mapping = 0;
	int result = -1;
	int i;
	int is_ifdef;
    size_t len = 257;
    
	if (strncmp(line, "#include", 8) == 0) {
        include = strtok(line, del);
        include = strtok(0, del) + 1;
        for (i = 0;include[i] != '"' && include[i] != '>'; i++);
        include[i] = 0;
        add_include(include, i + 1);
        // TODO inserarea inculde urilor
        return -1;
    }
    if (strncmp(line, "#define", 7) == 0) {
        mapping = strtok(line, del);
        symbol = strtok(0, del);
        mapping = strtok(0, del);
        i = 0;
        if (!mapping) {
            for (; symbol[i] != '\n'; i++);
            symbol[i] = 0;
        } else {
            for (; mapping[i] != '\n'; i++);
            mapping[i] = 0;
            new_mapping = include_define(mapping);
        }
        insert(symbol, new_mapping);
        return -1;
    }
    if (strncmp(line, "#undef", 6) == 0) {
        symbol = strtok(line, del);
        symbol = strtok(0, del);
        i = 0;
        for (;symbol[i] != '\n'; i++);
        symbol[i] = 0;
        remove_hash_map(symbol);
        return -1;
    }

    if (strncmp(line, "#elif", 5) == 0)
        return 2;

    if (strncmp(line, "#else", 5) == 0)
        return 3;

    if (strncmp(line, "#endif", 6) == 0)
        return 4;

    if (strncmp(line, "#ifdef", 6) == 0 || strncmp(line, "#ifndef", 7) == 0) {
		is_ifdef = strncmp(line, "#ifdef", 6) == 0 ? 1 : 0;
		define = strtok(line, del);
	    define = strtok(0, del);
        for (i = 0; define[i]; i++) {
            if (define[i] == '\n')
                define[i] = 0;
        }

        if (is_ifdef && search(define)) {
            // procesez liniile pana gasesc un else sau endif
            while (fgets(n_line, len, fin) != 0 && result < 3) {
                result = process(n_line);
            }
            // ignor liniile dupa else pana gesesc un endif
            if (result == 3) // else
                while (fgets(n_line, len, fin) != 0 && strncmp(n_line, "#endif", 6) != 0);
        } else {
            // ignor liniile pana gasesc un else sau endif
            while (fgets(n_line, len, fin) != 0  && result < 3) {
                if (strncmp(n_line, "#else", 5) == 0)
                    result = 3;

                if (strncmp(n_line, "#endif", 6) == 0)
                    result = 4;
            }
            // daca gasesc un else procesez liniile pana gasesc un endif
            if (result == 3) { // else
                while (fgets(n_line, len, fin) != 0 && result < 3)
                    result = process(n_line);
            }
        }
        free((void *) n_line);
        return -1;
    }

    if (strncmp(line, "#if", 3) == 0) {
        return -1;
        // TODO
    }

    new_line = include_define(line);
    add_line_code(new_line);
    free((void *) new_line);

    return 1;
}

void init() {
    len = 0;
    size = 1024 * 1024 * 16;
    includes_size = 0;
    folders_size = 0;
    multiline_comment = 0;

    hash_map = malloc(sizeof(HashMap*) * size);
    memset(hash_map, 0, sizeof(HashMap*) * size);

    indices = malloc(sizeof(int) * size);
    memset(indices, -1, sizeof(int) * size);

    deleted_item = malloc(sizeof(HashMap));
    deleted_item->symbol = malloc(1);
    deleted_item->symbol[0] = 0;
}

void parse_arg(int argc, char** argv) {
    char *in_file = 0;
    char *out_file = 0;
	char *symbol;
	char *mapping;
	char folder[257];
	int i;

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-D", 2) == 0) {
            char define[257];
            if (*(argv[i] + 2))
                strcpy(define, argv[i] + 2);
            else
                strcpy(define, argv[++i]);
            symbol = strtok(define, "=");
            mapping = strtok(0, "=");
            insert(symbol, mapping);
            continue;
        }

        if (strncmp(argv[i], "-o", 2) == 0) {
            out_file = malloc(257);
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
            add_folders(folder, strlen(folder));
            continue;
        }
        in_file = malloc(257);
        strcpy(in_file, argv[i] + 2);
    }

    fin = in_file ? fopen(in_file, "r") : stdin;
    fout = out_file ? fopen(out_file, "w") : stdout;

    if (in_file) free(in_file);
    if (out_file) free(out_file);
}

int main(int argc, char** argv) {
    char *line = malloc(257);
    size_t len = 257;
    init();
    parse_arg(argc, argv);

    while (fgets(line, len, fin) != 0)
        process(line);

    print_hash_map();
    print_includes();
    print_folders();

    free((void *)line);
    free_hash_map();
    fclose(fin);
    fclose(fout);
    return 0;
}

