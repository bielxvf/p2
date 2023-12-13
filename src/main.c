#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <termios.h>

#include <monocypher.h>

#include "./PrintError.h"

#include "../libargparse/argparse.c"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PROGRAM_NAME "p2"
#define PATH_MAX 4096
#define PASSWORD_MAX 4096
#define EXTENSION_LOCKED ".locked"

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
    char *config_path = (char *) malloc(sizeof(char)*PATH_MAX);
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strcat(config_path, homedir);
    strcat(config_path, "/.config/" PROGRAM_NAME);
    return config_path;
}

void
MkConfigDir(const char *config_path) // Will create ~/.config/p2 if it doesn't exist
{
    struct stat st = {0};

    if (stat(config_path, &st) == -1) {
        PrintError(ERR "%s does not exist, creating new", config_path);
        mkdir(config_path, 0700);
    }
}

void
PrintDirContents(char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entity;
    size_t i = 0;
    printf("Contents of %s:\n", path);
    while ((entity = readdir(dir)) != NULL) {
        if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            printf("  %s\n", entity->d_name);
            i++;
        }
    }
    closedir(dir);
    if (i == 0) {
        PrintError(ERR "%s looks empty. Create a new password with `p2 new [NAME]`", path);
    }
}

int
CmdList(int argc, char **argv)
{
    /* We don't need argv, so we just cast it to void */
    (void)argv;

    // Check that we have no arguments
    if (argc != 1) {
        PrintError(ERR "Unnecessary argument(s) for subcommand 'list'");
        return 1;
    }

    char *config_path = GetConfigPath();
    MkConfigDir(config_path); // Make sure we have a config directory

    // List everything in our config directory
    PrintDirContents(config_path);

    free(config_path);
    return 0;
}

char *
GetNewPath(char *path_prefix, char *name, char *extension)
{
    char *path = (char *) malloc(sizeof(char)*PATH_MAX);
    memset(path, 0, sizeof(char)*PATH_MAX);
    strcat(path, path_prefix);
    strcat(path, "/");
    strcat(path, name);
    strcat(path, extension);
    return path;
}

char *
GetPassPhrase(const char *prompt)
{
    struct termios oldtc;
    struct termios newtc;
    tcgetattr(STDIN_FILENO, &oldtc);
    newtc = oldtc;
    newtc.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newtc);

    char *phrase = (char *) malloc(sizeof(char)*PASSWORD_MAX);
    memset(phrase, 0, sizeof(char)*PASSWORD_MAX);
    printf("%s", prompt);
    scanf("%s", phrase);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
    printf("\n");
    return phrase;
}

int
CmdNew(int argc, char **argv)
{
    // Check that we have just one argument, the name of the new password
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'new'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'new'");
        return 1;
    }

    char *config_path = GetConfigPath();
    MkConfigDir(config_path);

    char *new_path = GetNewPath(config_path, argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(new_path, &st) == 0) {
        PrintError(ERR "Invalid name: %s. File %s already exists", argv[1], new_path);
        free(config_path);
        free(new_path);
        return 1;
    }

    char *plaintext = GetPassPhrase("Enter password: ");
    char *password = GetPassPhrase("Master password: ");

    size_t text_size = strlen(plaintext);
    char *cipher_text = (char *) malloc(sizeof(char)*text_size);

    /* TODO:
       1. Derive KEY from password
       2. Generate random NONCE
       3. Generate a MAC to be stored along with the ciphertext
       4. NULL additional data
       5. ad_size is 0
       6. Wipe sensitive data
     */

    free(config_path);
    free(new_path);
    free(plaintext);
    free(password);
    free(cipher_text);
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
