#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/random.h>
#include <pwd.h>
#include <dirent.h>
#include <termios.h>

#include <sodium.h>

#include "./PrintError.h"

#include "./libargparse/argparse.c"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PROGRAM_NAME "p2"
#define PATH_MAX 4096
#define PASSWORD_MAX 4096
#define EXTENSION_LOCKED ".locked"

#define ERR  "[ERROR] "
#define INFO "[INFO]  "

static const char *const usages[] = {
    PROGRAM_NAME" [options] [command] [args]\n\n"
    "    Commands:\n"
    "        list\tList passwords\n"
    "        new\tCreate a new password\n",
    NULL,
};

struct cmd_struct {
    const char *cmd;
    int (*fn) (int, const char **);
};

void
MemWipe(void *p, int len)
{
    memset(p, 0, len);
}

void
FileWipe(const char *path)
{
    FILE *fptr = fopen(remove_path, "w");
    for (int i = 0; i < st.st_size; i++) {
        fprintf(fptr, "%c", 0);
    }
    fclose(fptr);
}

char *
GetConfigPath(void)
{
    char *config_path = (char *) malloc(sizeof(char)*PATH_MAX);
    MemWipe(config_path, sizeof(char)*PATH_MAX);
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strcat(config_path, homedir);
    strcat(config_path, "/.config/" PROGRAM_NAME);
    return config_path;
}

void
MkConfigDir(void) // Will create ~/.config/p2 if it doesn't exist
{
    struct stat st = {0};

    char *config_path = GetConfigPath();

    if (stat(config_path, &st) == -1) {
        PrintError(INFO "%s does not exist, creating new", config_path);
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
        PrintError(INFO "%s looks empty. Create a new password with `p2 new [NAME]`", path);
    }
}

int
CmdList(int argc, const char **argv)
{
    /* We don't need argv, so we just cast it to void */
    (void)argv;

    // Check that we have no arguments
    if (argc != 1) {
        PrintError(ERR "Unnecessary argument(s) for subcommand 'list'");
        return 1;
    }

    MkConfigDir(); // Make sure we have a config directory

    // List everything in our config directory
    PrintDirContents(GetConfigPath());

    return 0;
}

char *
GetNewPath(const char *path_prefix, const char *name, const char *extension)
{
    char *path = (char *) malloc(sizeof(char)*PATH_MAX);
    MemWipe(path, sizeof(char)*PATH_MAX);
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
    MemWipe(phrase, sizeof(char)*PASSWORD_MAX);
    printf("%s", prompt);
    scanf("%s", phrase);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
    printf("\n");
    return phrase;
}

void
WriteDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size)
{
    FILE *fileptr;
    fileptr = fopen(path, "w");
    for (size_t i = 0; i < nonce_size; i++) {
        fprintf(fileptr, "%X", nonce[i]);
    }
    fprintf(fileptr, ":");
    for (size_t i = 0; i < ciphertext_size; i++) {
        fprintf(fileptr, "%X", ciphertext[i]);
    }
    fclose(fileptr);
}

int
CmdNew(int argc, const char **argv)
{

    // Check that we have just one argument, the name of the new password
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'new'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'new'");
        return 1;
    }

    MkConfigDir();

    char *new_path = GetNewPath(GetConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(new_path, &st) == 0) {
        PrintError(ERR "Invalid name: %s. File %s already exists", argv[1], new_path);
        free(new_path);
        return 1;
    }

    char *plaintext = GetPassPhrase("Enter password: ");
    char *password = GetPassPhrase("Master password: ");

    size_t password_len = strlen(password);
    size_t plaintext_len = strlen(plaintext);
    size_t key_size = crypto_secretbox_KEYBYTES;
    size_t nonce_size = crypto_secretbox_NONCEBYTES;
    size_t ciphertext_size = crypto_secretbox_MACBYTES + plaintext_len;

    unsigned char key[key_size];
    unsigned char nonce[nonce_size];
    unsigned char ciphertext[ciphertext_size];

    if (sodium_init() < 0) {
        // Death
        PrintError(ERR "Sodium could not init in %s", __func__);
        MemWipe(plaintext, sizeof(char)*plaintext_len);
        MemWipe(password, sizeof(char)*password_len);
        free(new_path);
        free(plaintext);
        free(password);
        return 1;
    }

    crypto_generichash(key, sizeof(key), (unsigned char *)password, password_len, NULL, 0);

    randombytes_buf(nonce, sizeof(nonce));
    crypto_secretbox_easy(ciphertext, (unsigned char *)plaintext, plaintext_len, nonce, key);

    // Writes NONCE:CIPHERTEXT
    WriteDataToFile(new_path, nonce, nonce_size, ciphertext, ciphertext_size);

    // Wipe and free
    MemWipe(plaintext, sizeof(char)*plaintext_len);
    MemWipe(password, sizeof(char)*password_len);
    free(new_path);
    free(plaintext);
    free(password);
    return 0;
}

int
CmdRemove(int argc, const char **argv)
{
    // Check that we have just one argument, the name of the new password
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'new'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'new'");
        return 1;
    }

    MkConfigDir();

    char *remove_path = GetNewPath(GetConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(remove_path, &st) != 0) {
        PrintError(ERR "Invalid name: %s. File %s does not exist", argv[1], remove_path);
        free(remove_path);
        return 1;
    } else {
        FileWipe(remove_path);
        remove(remove_path);
        printf("Removed file: %s\n", remove_path);
    }

    return 0;
}

int
CmdPrint(int argc, const char **argv)
{
    (void)argc;
    (void)argv;
    /*
    unsigned char decrypted[4];
    if (crypto_secretbox_open_easy(decrypted, ciphertext, crypto_secretbox_MACBYTES + plaintext_len, nonce, key) != 0) {
        return 1;
    }
    */
    return 0;
}

static struct cmd_struct commands[] = {
    { "list"  , CmdList   },
    { "new"   , CmdNew    },
    { "print" , CmdPrint  },
    { "remove", CmdRemove },
};

int
main(int argc, const char **argv)
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
    for (long unsigned int i = 0; i < ARRAY_SIZE(commands); i++) {
        if (!strcmp(commands[i].cmd, argv[0])) {
            cmd = &commands[i];
        }
    }
    if (cmd) {
        return cmd->fn(argc, argv);
    }
    return 0;
}
