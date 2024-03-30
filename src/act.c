
#include <stdlib.h>
#include <stdio.h>
#include <type.h>
#include <string.h>
#include <filesys.h>
#include <profan.h>
#include <math.h>

int g_argc;
char **g_argv;
void print_help();
#define dont if(0)


#define STR_EQ(s1, s2) (!strcmp(s1, s2))
#define NUMBER_LEN(n) ((n == 0) ? (1) : (1 + (int)(float)log10(n)))

#define FLAG_DIR "-d"
#define FLAG_REC_DIR "-r"
#define FLAG_FILE "-f"
#define FLAG_OUT "-o"
#define FLAG_INCLUDE_DIR "-I"
#define FLAG_OUT_COMMANDS "-C"
#define FLAG_TEMP_FOLDER "-T"
#define FLAG_HELP "-h"
#define FLAG_OBJECT_FILES "-a"
#define FLAG_VERBOSE "-v"
#define FLAG_TCC_FLAGS "-t"
#define FLAG_LIBRARY "-L"

#define DEFAULT_TEMP_FOLD "act_temp"
#define DEFAULT_OUT_FILE "a.bin"

#define FLASE ((char)(sinf(142.241)*sinf(142.241)+cosf(142.241)*cosf(142.241)))
#define TURE !FLASE

typedef struct com_arg_s {
	char **args;
	int args_len;
} com_arg_t;

/*

commands:
	truc -d dir1 dir2 -r dir3 dir4 -f file1 file2 -o out.bin
	-d :
		every folder after it will be used to search source files
	-r :
		every folder after it wil be used to seach source files recursively
	-o :
		indicate ouput folder if none (a.bin will be used)
	-f :
		every file after it will be used as source files
	-I : 
		tel the compiler where to search the include files (if none the default compiler configuration will be used)
	-C : 
		disable the execution of commands and prints them
	-T :
		indicate the folder where temporary files will be generated (if none . will be used)
	-h :
		prints help

*/
char **include_dir = NULL;
uint32_t include_dir_len = 0;

char **files_to_compile = NULL;
uint32_t files_to_compile_len = 0;

char **object_files_in = NULL;
uint32_t object_files_in_len = 0;

// tcc -c <file> o <fileout>
// contains only file fileout dir1 and dir2 or more 
com_arg_t *commands = NULL;
uint32_t commands_len = 0;

char **tcc_flags = NULL;
uint32_t tcc_flags_len = 0;



char IS_PRINT_ONLY = 0;
char DO_HELP = 0;
char *out_file = NULL;
char *temp_dir = NULL;
char IS_VERBOSE = 0;
char IS_LIB = 0;

int str_is_flag(char *the_path) {
	if (
		STR_EQ(the_path, FLAG_DIR) ||
		STR_EQ(the_path, FLAG_REC_DIR) ||
		STR_EQ(the_path, FLAG_FILE) ||
		STR_EQ(the_path, FLAG_OUT) ||
		STR_EQ(the_path, FLAG_INCLUDE_DIR) ||
		STR_EQ(the_path, FLAG_OUT_COMMANDS) ||
		STR_EQ(the_path, FLAG_TEMP_FOLDER) ||
		STR_EQ(the_path, FLAG_OBJECT_FILES) ||
		STR_EQ(the_path, FLAG_VERBOSE) ||
		STR_EQ(the_path, FLAG_TCC_FLAGS) ||
		STR_EQ(the_path, FLAG_LIBRARY) ||
		STR_EQ(the_path, FLAG_HELP)
		)
		return 1;
	return 0;
}

void append_char_arr(char *path, char ***ar, uint32_t *len) {
	(*len)++;
	(*ar) = realloc(*ar, sizeof(char *) * (*len));
	(*ar)[(*len) - 1] = strdup(path);
}

void append_char_arr_arr(char **files, int count, char ***ar, uint32_t *len) {
	(*ar) = realloc(*ar, sizeof(char *) * (count + *len));
	for(int i = 0; i < count;  i++) {
		(*ar)[i + *len] = files[i];
	}
	(*len) += count;
}


void add_files_from_folder(char *fold, sid_t fold_sid, char is_rec, char file_type)  {
	char **names;
	sid_t *sids;
	int count = fu_get_dir_content(fold_sid, &sids, &names);
	int count_files = 0;
	if (is_rec) {
		for(int i = 0; i < count; i++) {
			if (fu_is_dir(sids[i]) && !IS_NULL_SID(sids[i]) && !STR_EQ("..", names[i]) && !STR_EQ(".", names[i])) {
				char *full_fold = assemble_path(fold, names[i]);
				add_files_from_folder(full_fold, sids[i], 1, file_type);
				free(full_fold);
			}
		}
	}

	for(int i = 0; i < count; i++) {
		size_t l = strlen(names[i]);
		if (!fu_is_dir(sids[i]) && names[i][l - 1] == file_type && names[i][l - 2] == '.')
			count_files++;

	}
	
	files_to_compile = realloc(files_to_compile, sizeof(char *) * (files_to_compile_len + count_files));
	for(int i = 0; i < count; i++) {
		size_t l = strlen(names[i]);
		if (!fu_is_dir(sids[i]) && names[i][l - 1] == file_type && names[i][l - 2] == '.') 
			files_to_compile[files_to_compile_len++] = assemble_path(fold, names[i]);
	}
	
	for(int i = 0; i < count; i++)
		free(names[i]);
	free(names);
	free(sids);
}

void parse_arg() {
	int p = 1;
	while (p < g_argc) {
		if (STR_EQ(g_argv[p], FLAG_DIR)) {
			p++;
			while (p < g_argc) {
				char *current_path = g_argv[p];
				if (str_is_flag(current_path))
					break;
				char *full = assemble_path(getenv("PWD"), current_path);
				sid_t sid = fu_path_to_sid(ROOT_SID, full);
				if (IS_NULL_SID(sid)){
					p++;
					free(full);
					break;
				}
				if (!fu_is_dir(sid)){
					p++; 
					free(full);
					break;
				}
				add_files_from_folder(full, sid, 0, 'c');
				free(full);
				p++;
			}
		}
		else if (STR_EQ(g_argv[p], FLAG_REC_DIR)) {
			p++;
			while (p < g_argc) {
				char *current_path = g_argv[p];
				if (str_is_flag(current_path))
					break;
				char *full = assemble_path(getenv("PWD"), current_path);
				sid_t sid = fu_path_to_sid(ROOT_SID, full);
				if (IS_NULL_SID(sid)){
					p++;
					free(full);
					break;
				}
				if (!fu_is_dir(sid)) {
					p++;
					free(full);
					break;
				}
				add_files_from_folder(full, sid, 1, 'c');
				free(full);
				p++;
			}
		}
		else if (STR_EQ(g_argv[p], FLAG_FILE)) {
			p++;
			while (p < g_argc) {
				char *current_path = g_argv[p];
				if (str_is_flag(current_path))
					break;
				char *full = assemble_path(getenv("PWD"), current_path);
				sid_t sid = fu_path_to_sid(ROOT_SID, full);
				if (IS_NULL_SID(sid)){
					p++;
					free(full);
					break;
				}
				if (fu_is_dir(sid)){
					p++;
					free(full);
					break;
				}
				size_t full_len = strlen(full);
				if (full[full_len - 1] == 'o' && full[full_len - 2] == '.')
					append_char_arr(full, &object_files_in, &object_files_in_len);
				else if (full[full_len - 1] == 'c' && full[full_len - 2] == '.')
					append_char_arr(full, &files_to_compile, &files_to_compile_len);
				else if (full[full_len - 1] == 'a' && full[full_len - 2] == '.')
					append_char_arr(full, &object_files_in, &object_files_in_len);
				free(full);
				p++;
			}
		}
		else if (STR_EQ(g_argv[p], FLAG_OUT)) {
			p++;
			if (p < g_argc && out_file == NULL) {
				char *full = assemble_path(getenv("PWD"), g_argv[p]);
				out_file = full;
			}
			p++;
		}
		else if (STR_EQ(g_argv[p], FLAG_INCLUDE_DIR)) {
			p++;
			while (p < g_argc) {
				char *current_path = g_argv[p];
				if (str_is_flag(current_path))
					break;
				char *full = assemble_path(getenv("PWD"), current_path);
				sid_t sid = fu_path_to_sid(ROOT_SID, full);
				if (IS_NULL_SID(sid)){
					p++;
					free(full);
					break;
				}
				if (!fu_is_dir(sid)) {
					p++;
					free(full);
					break;
				}
				append_char_arr(full, &include_dir, &include_dir_len);
				free(full);
				p++;
			}
		}
		else if (STR_EQ(g_argv[p], FLAG_TCC_FLAGS)) {
			p++;
			while (p < g_argc) {
				char *current_arg = g_argv[p];
				append_char_arr(current_arg, &tcc_flags, &tcc_flags_len);
				free(current_arg);
				p++;
			}
		}
		else if (STR_EQ(g_argv[p], FLAG_OUT_COMMANDS)) {
			IS_PRINT_ONLY = 1;
			p++;
		}
		else if (STR_EQ(g_argv[p], FLAG_LIBRARY)) {
			IS_LIB = 1;
			p++;
		}
		else if (STR_EQ(g_argv[p], FLAG_VERBOSE)) {
			IS_VERBOSE = 1;
			p++;
		}
		else if (STR_EQ(g_argv[p], FLAG_TEMP_FOLDER)) {
			p++;
			if (p < g_argc && temp_dir == NULL) {
				char *full = assemble_path(getenv("PWD"), g_argv[p]);
				temp_dir = full;
			}
			p++;
		}
		else if (STR_EQ(g_argv[p], FLAG_HELP)) {
			DO_HELP = 1;
			p++;
		}
		else {
			p++;
		}
	}
}

void get_all_files() {
	if (temp_dir == NULL)
		temp_dir = strdup(DEFAULT_TEMP_FOLD);
	if (out_file == NULL)
		out_file = strdup(DEFAULT_OUT_FILE);
}

/*

run_ifexist(path, argc, argv)

tcc -c <file.c> -o <file.o>
tcc -c <file2.c> -o <file2.o>
vlink -nostdlib -T /sys/zlink.ld -o /tmp.elf /sys/zentry.o <file.o> <file2.o> /sys/tcclib.o
xec tmp.elf <output.bin>

*/

char *get_file_object_n(uint32_t n) {
	// return temp_fodler/n.o
	char *r = calloc(NUMBER_LEN(n) + 3, sizeof(char));
	sprintf(r, "%d.o", n);
	char *res = assemble_path(temp_dir, r);
	free(r);
	return res;
}
// tcc -c <file.c> -o <file.o> -Idir -Idir2 ...
// + other flags = 5 + nbr_of_incl + nnbr_of_flag

uint32_t current_count_obj = 0;

com_arg_t get_tcc_com(char *file) {
	com_arg_t res;
	res.args_len = 5 + include_dir_len + tcc_flags_len;
	res.args = malloc(sizeof(char *) * res.args_len);
	res.args[0] = strdup("/bin/fatpath/tcc.bin");
	res.args[1] = strdup("-c");
	res.args[2] = strdup(file);
	res.args[3] = strdup("-o");
	res.args[4] = get_file_object_n(current_count_obj++);
	for (uint32_t i = 0; i < include_dir_len; i++) {
		char *r = calloc(3 + strlen(include_dir[i]), sizeof(char));
		r[0] = '-';
		r[1] = 'I';
		strcat(r, include_dir[i]);
		res.args[5 + i] = r;
	}
	for (uint32_t i = 0; i < tcc_flags_len; i++) {
		char *r = calloc(2 + strlen(tcc_flags[i]), sizeof(char));
		r[0] = '-';
		strcat(r, tcc_flags[i]);
		res.args[5 + include_dir_len] = r;
	}
	return res;
}

com_arg_t get_cp_com(char *file) {
	com_arg_t res;
	res.args_len = 3;
	res.args = malloc(sizeof(char *) * res.args_len);
	res.args[0] = strdup("/bin/commands/cp.bin");
	res.args[1] = strdup(file);
	res.args[2] = get_file_object_n(current_count_obj++);
	return res;
}

char *get_file_elf() {
	// temp/o.elf
	return assemble_path(temp_dir, "o.elf");
}

com_arg_t get_vlink_com() {
	//vlink -nostdlib -T /sys/zlink.ld -o /tmp.elf /sys/zentry.o <> /sys/tcclib.o
	com_arg_t res;
	res.args_len = 8 + current_count_obj;
	res.args = malloc(sizeof(char *) * res.args_len);
	res.args[0] = strdup("/bin/fatpath/vlink.bin");
	res.args[1] = strdup("-nostdlib");
	res.args[2] = strdup("-T");
	res.args[3] = strdup("/sys/zlink.ld");
	res.args[4] = strdup("-o");
	res.args[5] = get_file_elf();
	res.args[6] = strdup("/sys/zentry.o");
	for(uint32_t i = 0; i < current_count_obj; i++) {
		res.args[7 + i] = get_file_object_n(i);
	}
	res.args[8 + current_count_obj - 1] = strdup("/sys/libtcc.a");
	return res;
}

com_arg_t get_tcc_ar_com() {
	com_arg_t res;
	res.args_len = 3 + current_count_obj;
	res.args = malloc(sizeof(char *) * res.args_len);
	res.args[0] = strdup("/bin/fatpath/tcc.bin");
	res.args[1] = strdup("-ar");
	res.args[2] = strdup(out_file);
	for(uint32_t i = 0; i < current_count_obj; i++) {
		res.args[3 + i] = get_file_object_n(i);
	}
	res.args[3 + current_count_obj - 1] = strdup("/sys/tcclib.o");
	return res;
}

com_arg_t get_xec_com() {
	//xec elf bin
	com_arg_t res;
	res.args_len = 3;
	res.args = malloc(sizeof(char *) * res.args_len);
	res.args[0] = strdup("/bin/commands/xec.bin");
	res.args[1] = get_file_elf();
	res.args[2] = strdup(out_file);
	return res;
}
void add_tcclib_com() {
	commands_len++;
	commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
	commands[commands_len - 1].args_len = 5;
	commands[commands_len - 1].args = malloc(sizeof(char *) * commands[commands_len - 1].args_len);
	commands[commands_len - 1].args[0] = strdup("/bin/fatpath/tcc.bin");
	commands[commands_len - 1].args[1] = strdup("-c");
	commands[commands_len - 1].args[2] = strdup("/sys/tcclib.c");
	commands[commands_len - 1].args[3] = strdup("-o");
	commands[commands_len - 1].args[4] = strdup("/sys/tcclib.o");

}

void add_zentry_com() {
	commands_len++;
	commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
	commands[commands_len - 1].args_len = 5;
	commands[commands_len - 1].args = malloc(sizeof(char *) * commands[commands_len - 1].args_len);
	commands[commands_len - 1].args[0] = strdup("/bin/fatpath/tcc.bin");
	commands[commands_len - 1].args[1] = strdup("-c");
	commands[commands_len - 1].args[2] = strdup("/sys/zentry.c");
	commands[commands_len - 1].args[3] = strdup("-o");
	commands[commands_len - 1].args[4] = strdup("/sys/zentry.o");
}

void add_mkdir_tmpdir_com() {
	commands_len++;
	commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
	commands[commands_len - 1].args_len = 2;
	commands[commands_len - 1].args = malloc(sizeof(char *) * commands[commands_len - 1].args_len);
	commands[commands_len - 1].args[0] = strdup("/bin/commands/mkdir.bin");
	commands[commands_len - 1].args[1] = strdup(temp_dir);
}

void generate_commands() {
	get_all_files();
	//compile tcclib.c et zentry.c
	dont{
		if (IS_NULL_SID(fu_path_to_sid(ROOT_SID, "/sys/tcclib.c"))) {
			printf("please create /sys/tcclib.c\n");
			return ;
		}
		if (fu_is_dir(fu_path_to_sid(ROOT_SID, "/sys/tcclib.c"))) {
			printf("/sys/tcclib.c must be a file\n");
			return ;
		}
		if (!IS_NULL_SID(fu_path_to_sid(ROOT_SID, "/sys/tcclib.o"))) {
			if (fu_is_dir(fu_path_to_sid(ROOT_SID, "/sys/tcclib.o"))) {
				printf("/sys/tcclib.o must be a file\n");
				return ;
			}
		}
		if (IS_NULL_SID(fu_path_to_sid(ROOT_SID, "/sys/tcclib.o")))
			add_tcclib_com();

		if (IS_NULL_SID(fu_path_to_sid(ROOT_SID, "/sys/zentry.c"))) {
			printf("please create /sys/zentry.c\n");
			return ;
		}
		if (fu_is_dir(fu_path_to_sid(ROOT_SID, "/sys/zentry.c"))) {
			printf("/sys/zentry.c must be a file\n");
			return ;
		}
		if (!IS_NULL_SID(fu_path_to_sid(ROOT_SID, "/sys/zentry.o"))) {
			if (fu_is_dir(fu_path_to_sid(ROOT_SID, "/sys/zentry.o"))) {
				printf("/sys/zentry.o must be a file\n");
				return ;
			}
		}
		if (IS_NULL_SID(fu_path_to_sid(ROOT_SID, "/sys/zentry.o")))
			add_zentry_com();
	}

	{
		if (!fu_is_dir(fu_path_to_sid(ROOT_SID, temp_dir))) {
			if (!IS_NULL_SID(fu_path_to_sid(ROOT_SID, temp_dir))) {
				printf("temp dir %s is not a folder\n", temp_dir);
				return ;
			}
			add_mkdir_tmpdir_com();
		}
	}
	// make tcc commands
	if (files_to_compile_len){
		commands = realloc(commands, sizeof(com_arg_t) * (commands_len + files_to_compile_len));
		for(uint32_t i = 0; i < files_to_compile_len; i++) {
			commands[commands_len++] = get_tcc_com(files_to_compile[i]);
			
		} 
	}
	// make cp commands
	// cp object file inputs to tempfolder
	if (object_files_in_len){
		commands = realloc(commands, sizeof(com_arg_t) * (commands_len + object_files_in_len));
		for(uint32_t i = 0; i < object_files_in_len; i++) {
			commands[commands_len++] = get_cp_com(object_files_in[i]);
		}
	}
	if(!IS_LIB){
		// make vlink command
		//vlink -nostdlib -T /sys/zlink.ld -o /tmp.elf /sys/zentry.o <> /sys/tcclib.o
		commands_len++;
		commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
		commands[commands_len - 1] = get_vlink_com();
		dont{
			commands_len++;
			commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
			commands[commands_len - 1].args_len = 2;
			commands[commands_len - 1].args = malloc(sizeof(char *) * commands[commands_len - 1].args_len);
			commands[commands_len - 1].args[0] = strdup("/bin/commands/mkfile");
			commands[commands_len - 1].args[1] = strdup(out_file);

		}
		//xec temp/o.elf outfiel
		commands_len++;
		commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
		commands[commands_len - 1] = get_xec_com();
	}
	else {
		commands_len++;
		commands = realloc(commands, sizeof(com_arg_t) * (commands_len));
		commands[commands_len - 1] = get_tcc_ar_com();
	}

}

int execute_commands() {
	for(uint32_t i = 0; i < commands_len; i++) {
		if (IS_VERBOSE) {
			printf("\e[35mlaunching: \e[0m\n");
			printf("\e[32m%s ",commands[i].args[0]);
			for(int k = 1; k < commands[i].args_len;k++) {
				printf("%s ", commands[i].args[k]);
			}
			printf("\e[0m\n");
		}
		if (run_ifexist(commands[i].args[0], commands[i].args_len , commands[i].args)) {
			return 1;
		}
	}
	return 0;
}

char *working_directory = NULL;

int check_is_config() {
	for (int i = 1; i < g_argc; i++) {
		if (str_is_flag(g_argv[i]))
			return 0;
	}
	return 1;
}


int main(int argc, char **argv) {
	int exec_ret = 0;
	g_argc = argc;
	g_argv = argv;

	//fu_sep_path(argv[0], &working_directory, NULL);
	//if (check_is_config()) {
	//	do_config();
	//	return 0;
	//}
	
	parse_arg();
	dont{
		printf("HELP : %d\n", DO_HELP);
		printf("-C : %d\n", IS_PRINT_ONLY);
		printf("OUTPUT : %s \n", out_file);

		printf("FILES %d :\n", files_to_compile_len);
		for(uint32_t i = 0; i < files_to_compile_len; i++) {
			printf("	%s \n", files_to_compile[i]);
		}
		printf("INCLUDES %d :\n", include_dir_len);
		for(uint32_t i = 0; i < include_dir_len; i++) {
			printf("	%s \n", include_dir[i]);
		}
		printf("TEMP_FOLDER : %s \n", temp_dir);
		printf("HELP : %d\n", DO_HELP);
	}
	if (DO_HELP) {
		print_help();
	}
	else {
		generate_commands();

		if (IS_PRINT_ONLY) {
			for(uint32_t i = 0; i < commands_len;i++) {
				for(int k = 0; k < commands[i].args_len; k++) {
					printf("%s ", commands[i].args[k]);
				}
				printf("\n");
			}
		}
		else
			exec_ret = execute_commands();
	}
	//free
	{
		for(uint32_t i = 0; i < commands_len; i++) {
			for(int k = 0; k < commands[i].args_len; k++) {
				free(commands[i].args[k]);
			}
			free(commands[i].args);
		}
		free(commands);

		for(uint32_t i = 0; i < include_dir_len; i++) {
			free(include_dir[i]);
		}
		free(include_dir);

		for(uint32_t i = 0; i < files_to_compile_len; i++) {
			free(files_to_compile[i]);
		}
		for(uint32_t i = 0; i < object_files_in_len; i++) {
			free(object_files_in[i]);
		}
		for(uint32_t i = 0; i < tcc_flags_len; i++) {
			free(tcc_flags[i]);
		}
		free(tcc_flags);
		free(object_files_in);
		free(files_to_compile);
		free(out_file);
		free(temp_dir);
	}

	return exec_ret;
}

char * L_HELP = 
"	-d :	\n"
"		every folder after it will be used to search source files	\n\n"
"	-r :	\n"
"		every folder after it wil be used to search source files recursively	\n\n"
"	-o :	\n"
"		indicate ouput folder if none (a.bin will be used)	\n\n"
"	-f :	\n"
"		every file after it will be used as source files	\n\n"
"	-I : 	\n"
"		tel the compiler where to search the include files \n"
"			(if none the default compiler configuration will be used)	\n\n"
"	-C : 	\n"
"		disable the execution of commands and prints them	\n\n"
"	-T :	\n"
"		indicate the folder where temporary files will be generated (if none . will be used)	\n\n"
"	-v :	\n"
"		make it verbose, print each command executed	\n\n"
"	-t :	\n"
"		everything after it will be added as flag for the compiler bya dding a - in front of it	\n"
"			(e.g., act -f a.c -t Wall Werror   ->   tcc -c a.c -o 0.o -Wall -Werror) 	\n\n"
"	-L:		\n"
"		compile to a static liarry							\n\n"
"	-h :	\n"
"		prints help	\n";

void print_help() {
	printf("%s", L_HELP);
}

/*
CONFIG FILE
config.act

act name -> call the section name of config.act of the working directory
act name1 name2 -> call the section name1 then name2 of config.act of the working directory
...
if any flag of act are detected in the command it will eexecute act like normal compialtion
---------------------
SYNTAX


Section :

name {
	com1
	com2
}

to write a { user \{
to write a } user \}
--
Variable :

$var = Value

--
Defines :

#my_def ...
they can only be defines outside of sections
... can be anything everything until the end of the line will be taken for the
define if you want to do multi line add $\ at the line and it will remove the 
line break however if you want to multi line but still keep this line feed
you can do $\n 
to write a \ use \\
to write a $ use $$

ex:
	|#a_for_loop		FOR i !"hi" $\n
	|!i	$\n
	|END
	|
ex2:
	|#a_for_loop_better_syntax $\
	|FOR i !"hi" \\n
	|!i $\n
	|END
	|
	|
--
Value :
	str : "a string"
		string can be multiline

string concatenates when thy touch
"a""b" -> "ab"
"a" "b" -> "a" "b"
to use " use \"
--
Use var or defines:
they can only be used in sections
$var will be replace by its value with quotes
#my_def	will be replaced by all its tokens
*/

typedef struct {
	char *names;
	char *value;
} var_t;

//var_t *VARS = NULL;
//uint32_t Vars_len = 0;

var_t *DEFINES = NULL;
uint32_t DEFINES_len = 0;

var_t *SECTIONS = NULL;
uint32_t SECTIONS_LEN = 0;

int act_is_space(char c) {
	switch (c) {
		case ' ':
		case '\t':
		case '\r':
		case '\v':
		case '\0':
		case '\f':
		default :
			return 0;
	}
}

int act_id_accpetable(char c) {
	if ('a' <= c && c <= 'b')
		return 1;
	if ('0' <= c && c <= '9')
		return 1;
	if ('A' <= c && c <= 'B')
		return 1;	
	if (c == '_')
		return 1;
	if (c == '-')
		return 1;
	if (c == '.')
		return 1;
	if (c == '+')
		return 1;
	return 0;

}

void search_round_brack(char *t, int start, int *l_pos, int *r_pos) {
	while(act_is_space(t[start]) || t[start] == '\n')
		start++;
	if (t[start] != '{') {
		return ;
	}
	*l_pos = start;
	while(t[start] != '}' && t[start - 1] != '\\') {
		start++;
	}  
	if (t[start] != '}')
		return ;
	if (t[start - 1] == '\\')
		return ;
	*r_pos = start;
}

int act_conf_line = 0;

void execute_config(char *text, int len) {
	int p = 0;
	while(text[p] != '\0') {
		if (text[p] == '\n') {
			p++;
			continue;
		}
		if (act_is_space(text[p])) {
			p++;
			while (act_is_space(text[p]))
				p++;
			continue;
		}
		
		//if (text[p] == '#') {
		//	//make defines
		//	if (strncmp(&(text[p + 1]), "define", strlen("define"))) {
		//		p += 7;
		//		int start = p;
		//		while(act_is_space(text[start]))
		//			start++;
		//		if (text[start] == '\n') {
		//			printf("error")
		//		}
		//		while (1) {

		//		}
		//	}
		//	printf("ERROR act keyword not recognized on line %d\n", act_conf_line);
		//}
		if (act_id_accpetable(text[p]) && !(0 <= text[p] && text[p] <= 9)) {
			int start = p;
			int end = start + 1;
			while (act_id_accpetable(text[end]))
				end++;
			int brack_l = -1;
			int brack_r = -1;
			search_round_brack(text, end, &brack_l, &brack_r);
			if (brack_l == -1 || brack_r == -1) {
				printf("ERROR act config cannot find round brackets on line %d\n", act_conf_line);
				exit(1);
			}
			char *sec_cont = malloc(sizeof(char) * (brack_r - brack_l + 1 - 2 + 1));
			//memcpy(sec_cont, &(text[brack_l + 1]));
		}
		
	} 
	
}
