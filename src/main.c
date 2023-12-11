#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>

#include "../libargparse/argparse.c"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PROGRAM_NAME "p2"
#define PATH_MAX 4096

#define ERR "[ERROR] "

static const char *const usages[] = {
    PROGRAM_NAME" [options] [command] [args]\n\n"
    "    Commands:\n"
    "        list\tList passwords",
    NULL,
};

struct cmd_struct {
    const char *cmd;
    int (*fn) (int, const char **);
};

void
PrintError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


char *
GetConfigPath(void)
{
    char *config_path = malloc(sizeof(char)*PATH_MAX);
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strcat(config_path, homedir);
    strcat(config_path, "/.config/" PROGRAM_NAME);
    return config_path;
}

int
CheckConfigExists(DIR *config_dir, const char *config_path)
{
    config_dir = opendir(config_path);
    if (config_dir == NULL) {
        PrintError(ERR "Could not open directory '%s', creating new one\n", config_path);
        if (mkdir(config_path, 0700) != 0) {
            PrintError(ERR "Could not create directory '%s'\n", config_path);
            return 0;
        } else {
            config_dir = opendir(config_path);
        }
    }
    return 1;
}

int
CmdList(int argc, char **argv)
{
    char *config_path = GetConfigPath();
    DIR *config_dir;
    if (!CheckConfigExists(config_dir, config_path)) {
        return 1;
    }

    struct dirent *entity;
    printf("Contents of %s:\n", config_path);
    while ((entity = readdir(config_dir)) != NULL) {
        if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            printf("  %s\n", entity->d_name);
        }
    }
    closedir(config_dir);
    free(config_path);
    return 0;
}

static struct cmd_struct commands[] = {
    { "list", CmdList },
};

int
main(int argc, char **argv)
{
    struct argparse argparse;
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_END(),
    };
    argparse_init(&argparse, options, usages, ARGPARSE_STOP_AT_NON_OPTION);
    argc = argparse_parse(&argparse, argc, argv);
    if (argc < 1) {
        argparse_usage(&argparse);
        return 1;
    }

    struct cmd_struct *cmd = NULL;
    for (int i = 0; i < ARRAY_SIZE(commands); i++) {
        if (!strcmp(commands[i].cmd, argv[0])) {
            cmd = &commands[i];
        }
    }
    if (cmd) {
        return cmd->fn(argc, argv);
    }
    return 0;
}
