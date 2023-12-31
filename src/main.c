/*
 * p2 is a rewrite of passman (https://github.com/bielsalamimo/passman)
 * Version:  0.1.0
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 *
 */
#define  _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
#include <sodium.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include "../libargparse/argparse.h"

#define PROGRAM_NAME "p2"
#define PATH_MAX 4096
#define PASSWORD_MAX 4096
#define EXTENSION_LOCKED ".locked"

#define ERR  "[ERROR] "
#define INFO "[INFO]  "

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

void PrintError(const char *fmt, ...);
void MemWipe(void *p, int len);
void FileWipe(const char *path);
void MkConfigDir(void);
char *GetConfigPath(void);
void PrintDirContents(char *path);
char *GetPassPhrase(const char *prompt);
char *GetNewPath(const char *path_prefix, const char *name, const char *extension);
void WriteDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size);
void ReadHexFromStr(unsigned char *hex_arr, const long int hex_arr_size, const char *str);

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

int CmdHelp(int argc, const char **argv)
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

int CmdList(int argc, const char **argv)
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

int CmdNew(int argc, const char **argv)
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

int CmdPrint(int argc, const char **argv)
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

int CmdRemove(int argc, const char **argv)
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

int CmdCopy(int argc, const char **argv)
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

int main(int argc, const char **argv)
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

void PrintError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    printf("\n");
    va_end(ap);
}

void MemWipe(void *p, int len)
{
    memset(p, 0, len);
}

void FileWipe(const char *path)
{
    struct stat st;
    stat(path, &st);
    FILE *fptr = fopen(path, "w");
    for (int i = 0; i < st.st_size; i++) {
        fprintf(fptr, "%c", 0);
    }
    fclose(fptr);
}

void MkConfigDir(void)
{
    struct stat st = {0};

    char *config_path = GetConfigPath();

    if (stat(config_path, &st) == -1) {
        PrintError(INFO "'%s' does not exist, creating new", config_path);
        mkdir(config_path, 0700);
    }
}

void PrintDirContents(char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entity;
    size_t i = 0;
    printf("Contents of '%s':\n", path);
    while ((entity = readdir(dir)) != NULL) {
        if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            printf("  %s\n", entity->d_name);
            i++;
        }
    }
    closedir(dir);
    if (i == 0) {
        PrintError(INFO "'%s' looks empty. Create a new password with `p2 new [NAME]`", path);
    }
}

char *GetPassPhrase(const char *prompt)
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

char *GetNewPath(const char *path_prefix, const char *name, const char *extension)
{
    char *path = (char *) malloc(sizeof(char)*PATH_MAX);
    MemWipe(path, sizeof(char)*PATH_MAX);
    strcat(path, path_prefix);
    strcat(path, "/");
    strcat(path, name);
    strcat(path, extension);
    return path;
}

void WriteDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size)
{
    FILE *fileptr;
    fileptr = fopen(path, "w");
    for (size_t i = 0; i < nonce_size; i++) {
        fprintf(fileptr, "%X", nonce[i]);
        if (i < nonce_size - 1) {
            fprintf(fileptr, " ");
        }
    }
    fprintf(fileptr, "\n");
    fprintf(fileptr, "%ld\n", ciphertext_size);
    for (size_t i = 0; i < ciphertext_size; i++) {
        fprintf(fileptr, "%X", ciphertext[i]);
        if (i < ciphertext_size - 1) {
            fprintf(fileptr, " ");
        }
    }
    fprintf(fileptr, "\n");
    fclose(fileptr);
}

void ReadHexFromStr(unsigned char *hex_arr, const long int hex_arr_size, const char *str)
{
    int i = 0, j = 0;
    while (j < hex_arr_size) {
        if (*(str+i) != ' ') {
            if (*(str+i+1) != ' ') {
                sscanf(str+i, "%2X", (unsigned int *) &hex_arr[j]);
                i += 3;
            } else {
                sscanf(str+i, "%X", (unsigned int *) &hex_arr[j]);
                i += 2;
            }
        } else {
            i++;
        }
        j++;
    }
}

char *GetConfigPath(void)
{
    char *config_path = (char *) malloc(sizeof(char)*PATH_MAX);
    MemWipe(config_path, sizeof(char)*PATH_MAX);
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strcat(config_path, homedir);
    strcat(config_path, "/.config/" PROGRAM_NAME);
    return config_path;
}
