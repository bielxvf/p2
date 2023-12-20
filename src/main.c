/*
 * p2 is a rewrite of passman (https://github.com/bielsalamimo/passman)
 * Version:  0.1.0
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 *
 */
#define  _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
#include <sodium.h>

#include "./PrintError.h"
#include "./MemWipe.h"
#include "./FileWipe.h"
#include "./MkConfigDir.h"
#include "./PrintDirContents.h"
#include "./GetPassPhrase.h"
#include "./GetNewPath.h"
#include "./WriteDataToFile.h"
#include "./ReadHexFromStr.h"

#include "./macros.h"

#include "./libargparse/argparse.c"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static const char *const usages[] = {
    PROGRAM_NAME" [command] [arg]\n\n"
    "    Commands:\n"
    "        help\tShow this help message and exit\n"
    "        list\tList passwords\n"
    "        new [name]\tCreate a new password (encrypt)\n"
    "        print [name]\tPrints a password (decrypt)\n"
    "        remove [name]\tRemoves a password",
    NULL,
};

struct cmd_struct {
    const char *cmd;
    int (*fn) (int, const char **);
};

int
CmdHelp(int argc, const char **argv)
{
    (void) argc;
    (void) argv;

    struct argparse argparse;
    struct argparse_option options[] = {
        OPT_END(),
    };
    argparse_init(&argparse, options, usages, ARGPARSE_STOP_AT_NON_OPTION);

    argparse_usage(&argparse);

    return 0;
}

int
CmdList(int argc, const char **argv)
{
    /* We don't need argv */
    (void)argv;

    if (argc != 1) {
        PrintError(ERR "Unnecessary argument(s) for subcommand 'list'");
        return 1;
    }

    MkConfigDir();

    PrintDirContents(GetConfigPath());

    return 0;
}

int
CmdNew(int argc, const char **argv)
{

    /* Argument should be the name of the password we want to create */
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
        PrintError(ERR "Invalid name: '%s'. File '%s' already exists", argv[1], new_path);
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
        PrintError(ERR "Sodium could not init in '%s'", __func__);
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

    WriteDataToFile(new_path, nonce, nonce_size, ciphertext, ciphertext_size);

    MemWipe(plaintext, sizeof(char)*plaintext_len);
    MemWipe(password, sizeof(char)*password_len);
    free(new_path);
    free(plaintext);
    free(password);
    return 0;
}

int
CmdPrint(int argc, const char **argv)
{
    /* Argument should be the name of the password we want to print */
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'print'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'print'");
        return 1;
    }

    MkConfigDir();

    char *print_path = GetNewPath(GetConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(print_path, &st) != 0) {
        PrintError(ERR "Invalid name: '%s'. File '%s' does not exist", argv[1], print_path);
        free(print_path);
        return 1;
    }

    char *line = (char *) malloc(sizeof(char) * PASSWORD_MAX);
    size_t line_len = 0;
    size_t nonce_size = crypto_secretbox_NONCEBYTES;
    size_t ciphertext_size;
    size_t key_size = crypto_secretbox_KEYBYTES;
    unsigned char key[key_size];

    FILE *fptr;
    fptr = fopen(print_path, "r");
    char *str_nonce = (char *) malloc(nonce_size);
    getline(&str_nonce, &line_len, fptr);

    getline(&line, &line_len, fptr);
    ciphertext_size = strtol(line, NULL, 10);
    size_t plaintext_len = ciphertext_size - crypto_secretbox_MACBYTES;

    char *str_ciphertext = (char *) malloc(sizeof(char) * PASSWORD_MAX);
    getline(&str_ciphertext, &line_len, fptr);
    fclose(fptr);

    unsigned char *nonce      = (unsigned char *) malloc(nonce_size);
    unsigned char *ciphertext = (unsigned char *) malloc(ciphertext_size);

    ReadHexFromStr(nonce, nonce_size, str_nonce);
    ReadHexFromStr(ciphertext, ciphertext_size, str_ciphertext);

    char *password = GetPassPhrase("Master password: ");
    size_t password_len = strlen(password);

    if (sodium_init() < 0) {
        PrintError(ERR "Sodium could not init in '%s'", __func__);
        MemWipe(password, sizeof(char) * password_len);
        MemWipe(key, key_size);
        free(password);
        free(print_path);
        free(line);
        free(str_nonce);
        free(str_ciphertext);
        free(nonce);
        return 1;
    }

    crypto_generichash(key, sizeof(key), (unsigned char *)password, password_len, NULL, 0);

    unsigned char decrypted[plaintext_len];
    if (crypto_secretbox_open_easy(decrypted, ciphertext, ciphertext_size, nonce, key) != 0) {
        MemWipe(password, sizeof(char) * password_len);
        MemWipe(key, key_size);
        free(password);
        free(print_path);
        free(line);
        free(str_nonce);
        free(str_ciphertext);
        free(nonce);
        /* free(ciphertext); */
        PrintError(ERR "Decryption failed");
        return 1;
    }
    MemWipe(password, sizeof(char) * password_len);
    MemWipe(key, key_size);

    for (size_t i = 0; i < plaintext_len; i++) {
        printf("%c", decrypted[i]);
    }
    printf("\n");

    MemWipe(decrypted, sizeof(char) * plaintext_len);
    free(password);
    free(print_path);
    free(line);
    free(str_nonce);
    free(str_ciphertext);
    free(nonce);
    /* free(ciphertext); */
    return 0;
}

int
CmdRemove(int argc, const char **argv)
{
    /* Argument should be the name of the password we want to remove */
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'remove'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'remove'");
        return 1;
    }

    MkConfigDir();

    char *remove_path = GetNewPath(GetConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(remove_path, &st) != 0) {
        PrintError(ERR "Invalid name: '%s'. File '%s' does not exist", argv[1], remove_path);
        free(remove_path);
        return 1;
    } else {
        FileWipe(remove_path);
        remove(remove_path);
        printf("Removed file: '%s'\n", remove_path);
    }

    return 0;
}

int
CmdCopy(int argc, const char **argv)
{

    /* Argument should be the name of the password we want to copy to clipboard */
    if (argc > 2) {
        PrintError(ERR "Too many arguments for subcommand 'copy'");
        return 1;
    } else if (argc < 2) {
        PrintError(ERR "Not enough arguments for subcommand 'copy'");
        return 1;
    }

    MkConfigDir();

    char *copy_path = GetNewPath(GetConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(copy_path, &st) != 0) {
        PrintError(ERR "Invalid name: '%s'. File '%s' does not exist", argv[1], copy_path);
        free(copy_path);
        return 1;
    }

    char *line = (char *) malloc(sizeof(char) * PASSWORD_MAX);
    size_t line_len = 0;
    size_t nonce_size = crypto_secretbox_NONCEBYTES;
    size_t ciphertext_size;
    size_t key_size = crypto_secretbox_KEYBYTES;
    unsigned char key[key_size];

    FILE *fptr;
    fptr = fopen(copy_path, "r");
    char *str_nonce = (char *) malloc(nonce_size);
    getline(&str_nonce, &line_len, fptr);

    getline(&line, &line_len, fptr);
    ciphertext_size = strtol(line, NULL, 10);
    size_t plaintext_len = ciphertext_size - crypto_secretbox_MACBYTES;

    char *str_ciphertext = (char *) malloc(sizeof(char) * PASSWORD_MAX);
    getline(&str_ciphertext, &line_len, fptr);
    fclose(fptr);

    unsigned char *nonce      = (unsigned char *) malloc(nonce_size);
    unsigned char *ciphertext = (unsigned char *) malloc(ciphertext_size);

    ReadHexFromStr(nonce, nonce_size, str_nonce);
    ReadHexFromStr(ciphertext, ciphertext_size, str_ciphertext);

    char *password = GetPassPhrase("Master password: ");
    size_t password_len = strlen(password);

    if (sodium_init() < 0) {
        PrintError(ERR "Sodium could not init in '%s'", __func__);
        MemWipe(password, sizeof(char) * password_len);
        MemWipe(key, key_size);
        free(password);
        free(copy_path);
        free(line);
        free(str_nonce);
        free(str_ciphertext);
        free(nonce);
        return 1;
    }

    crypto_generichash(key, sizeof(key), (unsigned char *)password, password_len, NULL, 0);

    unsigned char decrypted[plaintext_len];
    if (crypto_secretbox_open_easy(decrypted, ciphertext, ciphertext_size, nonce, key) != 0) {
        MemWipe(password, sizeof(char) * password_len);
        MemWipe(key, key_size);
        free(password);
        free(copy_path);
        free(line);
        free(str_nonce);
        free(str_ciphertext);
        free(nonce);
        /* free(ciphertext); */
        PrintError(ERR "Decryption failed");
        return 1;
    }
    MemWipe(password, sizeof(char) * password_len);
    MemWipe(key, key_size);

    /* TODO: Copy decrypted[] to clipboard */
    for (size_t i = 0; i < plaintext_len; i++) {
        printf("%c", decrypted[i]);
    }
    printf("\n");

    MemWipe(decrypted, sizeof(char) * plaintext_len);
    free(password);
    free(copy_path);
    free(line);
    free(str_nonce);
    free(str_ciphertext);
    free(nonce);
    /* free(ciphertext); */
    return 0;
}

static struct cmd_struct commands[] = {
    { "help"  , CmdHelp   },
    { "list"  , CmdList   },
    { "new"   , CmdNew    },
    { "print" , CmdPrint  },
    { "remove", CmdRemove },
    { "copy"  , CmdCopy   },
};

int
main(int argc, const char **argv)
{
    struct argparse argparse;
    struct argparse_option options[] = {
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
    } else {
        PrintError(ERR "Invalid subcommand '%s'", argv[0]);
        argparse_usage(&argparse);
        return 1;
    }
}
