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

#include "./PrintError.h"

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

void
CreateConfigDirIfNonExistant(const char *config_path)
{
    struct stat st = {0};

    if (stat(config_path, &st) == -1) {
        PrintError(ERR "'%s' does not exist, creating new", config_path);
        mkdir(config_path, 0700);
    }
}

void
PrintDirContents(char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entity;
    while ((entity = readdir(dir)) != NULL) {
        if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            printf("  %s\n", entity->d_name);
        }
    }
    closedir(dir);
}

int
CmdList(int argc, char **argv)
{
    /* We don't need argv, so we just cast it to void */
    (void)argv;

    // Check that we have no arguments
    if (argc != 1) {
        PrintError(ERR "Unnecessary argument(s) for subcommand 'list'");
        PrintError(ERR "argc: %d", argc);
        return 1;
    }

    char *config_path = GetConfigPath();
    CreateConfigDirIfNonExistant(config_path);

    printf("Contents of %s:\n", config_path);
    PrintDirContents(config_path);

    free(config_path);
    return 0;
}

int
CmdNew(int argc, char **argv)
{
    char *config_path = GetConfigPath();
    CreateConfigDirIfNonExistant(config_path);

    // Check that we have just one argument, the name of the new password
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'new'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'new'");
        return 1;
    } else {
    }

    free(config_path);
    return 0;
}

static struct cmd_struct commands[] = {
    { "list", CmdList },
    { "new", CmdNew },
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
