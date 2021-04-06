#include <windows.h>
#include <stdio.h>
#define DEFINE "#define"
#define UNDEF "#undef"
#define INCLUDE "#include"
#define CHAR char
int process (CHAR **str) {
    char *line = *str;
    if (strncmp(line, INCLUDE, 8) == 0) {
        char del[2] = " "; // DEFINE
        char *include = strtok(line, del);
        include = strtok(NULL, del) + 1;
        int i = 0;
        for (; include[i] != '"' && include[i] != '>'; i++);
        include[i] = 0;
        add_include(include, i + 1);
        return -1;
    }
}
