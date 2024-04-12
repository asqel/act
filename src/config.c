

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



char **str_split_lines(char *str) {
	char **res = NULL;
	int res_len = 0;
	if (str == NULL)
		return NULL;
	int p = 0;
	int start = 0;
	int end = 0;
	while (str[p] != '\0') {
		while(str[p] == '\n')
			p++;
		start = p;
		end = start;
		while(str[end] != '\n')
			end++;
		res_len++;
		res = realloc(res, res_len * sizeof(char *));
		res[res_len - 1] = malloc(sizeof(char) * (1 + end - start));
		for(int i = start; i < end; i++)
			res[i - start] = str[p];
		res[end - start] = '\0';
	}
	res_len++;
	res = realloc(res, sizeof(char *) * res_len);
	res[res_len - 1] = NULL;
	return res;
}


int do_config(char *text) {
	char **lines = str_split_lines(text);
}