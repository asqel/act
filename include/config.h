#ifndef CONFIG_H
#define CONFIG_H

void parse_config_section(char *text);
int act_config();
char *trim_end(char *str);
char **str_split_lines(char *str);
char *read_file(char *path);
extern char *conf_path;
extern char **sections;

#endif