
#include "../include/act.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <profan/filesys.h>
#include <profan/syscall.h>
#include <profan.h>

char *read_file(char *path) {
	FILE *f = fopen(path, "r");
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *res = malloc(sizeof(char) * (len + 1));
	fread(res, 1, len, f);
	res[len] = '\0';
	fclose(f);
	return res;
}

char **str_split_lines(char *input) {
	//char **res = NULL;
	//int res_len = 0;
	//if (str == NULL)
	//	return NULL;
	//int p = 0;
	//int start = 0;
	//int end = 0;
	//while (str[p] != '\0') {
	//	while(str[p] == '\n')
	//		p++;
	//	start = p;
	//	end = start;
	//	while(str[end] != '\n')
	//		end++;
	//	res_len++;
	//	res = realloc(res, res_len * sizeof(char *));
	//	res[res_len - 1] = malloc(sizeof(char) * (1 + end - start));
	//	for(int i = start; i < end; i++)
	//		res[res_len - 1][i - start] = str[p];
	//	res[res_len - 1][end - start] = '\0';
	//}
	//res_len++;
	//res = realloc(res, sizeof(char *) * res_len);
	//res[res_len - 1] = NULL;
	//return res;
	    // Compter le nombre de lignes dans l'entrée

    char** lines = NULL;

    char* line = strtok(input, "\n");
    int i = 0;
    while (line != NULL) {
		//printf("@@@line %d\n%s\n###",i , line);
		lines = realloc(lines, sizeof(char *) * (i + 1));
        lines[i++] = strdup(line);
        line = strtok(NULL, "\n");
    }
    
    // Insérer un pointeur NULL à la fin du tableau
	lines = realloc(lines, sizeof(char *) * (i + 1));
    lines[i] = NULL;

    return lines;
}
char is_space(char c) {
	switch (c) {
		case '\t':
		case '\n':
		case '\r':
		case ' ':
			return 1;
		default:
			return 0;
	}
}

char **trim_lines(char **lines) {
	char **res = NULL;
	int end = 0;
	int p = 0;
	while (lines[p] != NULL) {
		int current_end = strlen(lines[p]);
		while (is_space(lines[p][current_end - 1]))
			current_end--;
		if (current_end <= 1) {
			p++;
			continue;
		}
		res = realloc(res, sizeof(char *) * (end + 1));
		res[end] = calloc(current_end, sizeof(char));
		strncpy(res[end], lines[p], current_end);
		end++;
		p++;
	}
	p = 0;
	while (lines[p] != NULL)
		free(lines[p++]);
	free(lines);
	res = realloc(res, sizeof(char *) * (end + 1));
	res[end] = NULL;
	return res;
}

char *conf_path = NULL;

char **sections = NULL;

int act_config() {
	conf_path = assemble_path(getenv("PWD"), "config.act");
	if (fu_is_dir(fu_path_to_sid(ROOT_SID, conf_path)) || IS_NULL_SID(fu_path_to_sid(ROOT_SID, conf_path))) {
		printf("ERROR act : config.act not found\n");
		exit(1);
	}
	char *conf_text = read_file(conf_path);
	parse_config_section(conf_text);
	free(conf_text);
	return 0;
}


void parse_config_section(char *text) {
	char **lines = str_split_lines(text);
	lines = trim_lines(lines);
	for(int i = 0; lines[i] != NULL; i++) {
		printf("%d : %s |\n", i, lines[i]);
	}


}