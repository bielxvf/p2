#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "../libargparse/argparse.c"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PROGRAM_NAME "p2"
#define PATH_MAX 4096

static const char *const usages[] = {
    "p2 [list|create|remove|copy] [args]",
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
    printf("Contents of %s:\n", config_path);
    // TODO: Print contents of config directory
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
        return -1;
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
