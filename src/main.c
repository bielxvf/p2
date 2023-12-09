#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>

#include "../libargparse/argparse.c"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PROGRAM_NAME "p2"
#define PATH_MAX 4096

#define P2_ERROR "[p2] [Error] "

static const char *const usages[] = {
    PROGRAM_NAME" [list|create|remove|copy] [args]",
    NULL,
};

struct cmd_struct {
    const char *cmd;
    int (*fn) (int, const char **);
};

int
cmd_list(int argc, char **argv)
{
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    char config_path[PATH_MAX] = {0};
    strcat(config_path, homedir);
    strcat(config_path, "/.config/"PROGRAM_NAME);

    DIR *config_dir;
    struct dirent *entity;
    config_dir = opendir(config_path);
    if (config_dir == NULL) {
        fprintf(stderr, P2_ERROR"Could not open directory '%s'\n", config_path);
        if (mkdir(config_path, 0700) != 0) {
            fprintf(stderr, P2_ERROR"Could not create directory '%s' (errno = %d)\n", config_path, errno);
            return errno;
        }
    }

    printf("Contents of %s:\n", config_path);
    while ((entity = readdir(config_dir)) != NULL) {
        if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            printf("  %s\n", entity->d_name);
        }
    }
    closedir(config_dir);

    return 0;
}

static struct cmd_struct commands[] = {
    {"list", cmd_list},
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
