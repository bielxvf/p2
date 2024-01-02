/*
 * p2 is a rewrite of passman (https://github.com/bielxvf/passman)
 * Version:  0.1.0
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 *
 */

// TODO: Backup subcommand
// TODO: Restore subcommand
// TODO: Colors

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
#define PASSWORD_MAX 4096
#define EXTENSION_LOCKED ".locked"

#define ERROR  "[ERROR] "
#define INFO   "[INFO]  "

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define UNUSED(a)   \
    do {            \
        (void) (a); \
    } while(0)


void printError(const char *fmt, ...);
void printInfo(const char *fmt, ...);
void memWipe(void *p, int len);
void fileWipe(const char *path);
void mkConfigDir(void);
char *getConfigPath(void);
void printDirContents(char *path);
void printBaseName(char *name);
char *getPassPhrase(const char *prompt);
char *getNewPath(const char *path_prefix, const char *name, const char *extension);
void writeDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size);
void readHexFromStr(unsigned char *hex_arr, const long int hex_arr_size, const char *str);

static const char *const usages[] = {
    PROGRAM_NAME" [command] [arg]\n\n"
    "    Commands:\n"
    "        help    Show this help message and exit\n"
    "        list    List passwords\n"
    "        new [name]    Create a new password\n"
    "        print [name]    Prints a password\n"
    "        remove [name]    Removes a password\n"
    "        copy [name]    Copies a password to clipboard",
    NULL,
};

struct cmd_struct {
    const char *cmd;
    int (*fn) (int, const char **);
};

int cmdHelp(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    struct argparse argparse;
    struct argparse_option options[] = {
        OPT_END(),
    };
    argparse_init(&argparse, options, usages, ARGPARSE_STOP_AT_NON_OPTION);

    argparse_usage(&argparse);

    return 0;
}

int cmdList(int argc, const char **argv)
{
    UNUSED(argv);

    if (argc != 1) {
        printError("Unnecessary argument(s) for subcommand 'list'");
        return 1;
    }

    mkConfigDir();

    char *path = getConfigPath();
    DIR *dir = opendir(path);
    struct dirent *entity;
    printf("Contents of '%s':\n", path);
    size_t i = 0;
    while ((entity = readdir(dir)) != NULL) {
        if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            printBaseName(entity->d_name);
            i++;
        }
    }
    closedir(dir);
    if (i == 0) {
        printInfo("'%s' looks empty. Create a new password with `"PROGRAM_NAME" new [NAME]`", path);
        return 0;
    }

    return 0;
}

int cmdNew(int argc, const char **argv)
{

    if (argc > 2) {
        printError("Too many arguments for subcommand 'new'");
        return 1;
    } else if (argc < 2) {
        printError("Not enough arguments for subcommand 'new'");
        return 1;
    }

    mkConfigDir();

    char *new_path = getNewPath(getConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(new_path, &st) == 0) {
        printError("Invalid name: '%s'. File '%s' already exists", argv[1], new_path);
        free(new_path);
        return 1;
    }

    char *plaintext = getPassPhrase("Enter password: ");
    char *password = getPassPhrase("Master password: ");

    size_t password_len = strlen(password);
    size_t plaintext_len = strlen(plaintext);
    size_t key_size = crypto_secretbox_KEYBYTES;
    size_t nonce_size = crypto_secretbox_NONCEBYTES;
    size_t ciphertext_size = crypto_secretbox_MACBYTES + plaintext_len;

    unsigned char key[key_size];
    unsigned char nonce[nonce_size];
    unsigned char ciphertext[ciphertext_size];

    if (sodium_init() < 0) {
        printError("Sodium could not init in '%s'", __func__);
        memWipe(plaintext, sizeof(*plaintext) * plaintext_len);
        memWipe(password, sizeof(*password) * password_len);
        free(new_path);
        free(plaintext);
        free(password);
        return 1;
    }

    crypto_generichash(key, sizeof(key), (unsigned char *)password, password_len, NULL, 0);

    randombytes_buf(nonce, sizeof(nonce));
    crypto_secretbox_easy(ciphertext, (unsigned char *)plaintext, plaintext_len, nonce, key);

    writeDataToFile(new_path, nonce, nonce_size, ciphertext, ciphertext_size);

    memWipe(plaintext, sizeof(*plaintext) * plaintext_len);
    memWipe(password, sizeof(*password) * password_len);
    free(new_path);
    free(plaintext);
    free(password);
    return 0;
}

int cmdPrint(int argc, const char **argv)
{
    if (argc > 2) {
        printError("Too many arguments for subcommand 'print'");
        return 1;
    } else if (argc < 2) {
        printError("Not enough arguments for subcommand 'print'");
        return 1;
    }

    mkConfigDir();

    char *print_path = getNewPath(getConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(print_path, &st) != 0) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[1], print_path);
        free(print_path);
        return 1;
    }

    char *line = (char *) malloc(sizeof(*line) * PASSWORD_MAX);
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

    char *str_ciphertext = (char *) malloc(sizeof(*str_ciphertext) * PASSWORD_MAX);
    getline(&str_ciphertext, &line_len, fptr);
    fclose(fptr);

    unsigned char *nonce      = (unsigned char *) malloc(nonce_size);
    unsigned char *ciphertext = (unsigned char *) malloc(ciphertext_size);

    readHexFromStr(nonce, nonce_size, str_nonce);
    readHexFromStr(ciphertext, ciphertext_size, str_ciphertext);

    char *password = getPassPhrase("Master password: ");
    size_t password_len = strlen(password);

    if (sodium_init() < 0) {
        printError("Sodium could not init in '%s'", __func__);
        memWipe(password, sizeof(*password) * password_len);
        memWipe(key, key_size);
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
        memWipe(password, sizeof(*password) * password_len);
        memWipe(key, key_size);
        free(password);
        free(print_path);
        free(line);
        free(str_nonce);
        free(str_ciphertext);
        free(nonce);
        printError("Decryption failed");
        return 1;
    }
    memWipe(password, sizeof(*password) * password_len);
    memWipe(key, key_size);

    for (size_t i = 0; i < plaintext_len; i++) {
        printf("%c", decrypted[i]);
    }
    printf("\n");

    memWipe(decrypted, sizeof(*decrypted) * plaintext_len);
    free(password);
    free(print_path);
    free(line);
    free(str_nonce);
    free(str_ciphertext);
    free(nonce);
    return 0;
}

int cmdRemove(int argc, const char **argv)
{
    if (argc > 2) {
        printError("Too many arguments for subcommand 'remove'");
        return 1;
    } else if (argc < 2) {
        printError("Not enough arguments for subcommand 'remove'");
        return 1;
    }

    mkConfigDir();

    char *remove_path = getNewPath(getConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(remove_path, &st) != 0) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[1], remove_path);
        free(remove_path);
        return 1;
    } else {
        fileWipe(remove_path);
        remove(remove_path);
        printf("Removed file: '%s'\n", remove_path);
    }

    return 0;
}

int cmdCopy(int argc, const char **argv)
{
    // TODO: Copy decrypted[] to clipboard

    if (argc > 2) {
        printError("Too many arguments for subcommand 'copy'");
        return 1;
    } else if (argc < 2) {
        printError("Not enough arguments for subcommand 'copy'");
        return 1;
    }

    mkConfigDir();

    char *copy_path = getNewPath(getConfigPath(), argv[1], EXTENSION_LOCKED);

    struct stat st;
    if (stat(copy_path, &st) != 0) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[1], copy_path);
        free(copy_path);
        return 1;
    }

    char *line = (char *) malloc(sizeof(*line) * PASSWORD_MAX);
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

    char *str_ciphertext = (char *) malloc(sizeof(*str_ciphertext) * PASSWORD_MAX);
    getline(&str_ciphertext, &line_len, fptr);
    fclose(fptr);

    unsigned char *nonce      = (unsigned char *) malloc(nonce_size);
    unsigned char *ciphertext = (unsigned char *) malloc(ciphertext_size);

    readHexFromStr(nonce, nonce_size, str_nonce);
    readHexFromStr(ciphertext, ciphertext_size, str_ciphertext);

    char *password = getPassPhrase("Master password: ");
    size_t password_len = strlen(password);

    if (sodium_init() < 0) {
        printError("Sodium could not init in '%s'", __func__);
        memWipe(password, sizeof(*password) * password_len);
        memWipe(key, key_size);
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
        memWipe(password, sizeof(*password) * password_len);
        memWipe(key, key_size);
        free(password);
        free(copy_path);
        free(line);
        free(str_nonce);
        free(str_ciphertext);
        free(nonce);
        printError("Decryption failed");
        return 1;
    }
    memWipe(password, sizeof(*password) * password_len);
    memWipe(key, key_size);

    for (size_t i = 0; i < plaintext_len; i++) {
        printf("%c", decrypted[i]);
    }
    printf("\n");

    memWipe(decrypted, sizeof(*decrypted) * plaintext_len);
    free(password);
    free(copy_path);
    free(line);
    free(str_nonce);
    free(str_ciphertext);
    free(nonce);
    return 0;
}

static struct cmd_struct commands[] = {
    { "help"  , cmdHelp   },
    { "list"  , cmdList   },
    { "new"   , cmdNew    },
    { "print" , cmdPrint  },
    { "remove", cmdRemove },
    { "copy"  , cmdCopy   },
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
        printError("Invalid subcommand '%s'", argv[0]);
        argparse_usage(&argparse);
        return 1;
    }
}

void printError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, ERROR);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void printInfo(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, INFO);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void memWipe(void *p, int len)
{
    memset(p, 0, len);
}

void fileWipe(const char *path)
{
    struct stat st;
    stat(path, &st);
    FILE *fptr = fopen(path, "w");
    for (int i = 0; i < st.st_size; i++) {
        fprintf(fptr, "%c", 0);
    }
    fclose(fptr);
}

void mkConfigDir(void)
{
    struct stat st = {0};

    char *config_path = getConfigPath();

    if (stat(config_path, &st) == -1) {
        printInfo("'%s' does not exist, creating new", config_path);
        mkdir(config_path, 0700);
    }
}

char *getPassPhrase(const char *prompt)
{
    struct termios oldtc;
    struct termios newtc;
    tcgetattr(STDIN_FILENO, &oldtc);
    newtc = oldtc;
    newtc.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newtc);

    char *phrase = (char *) malloc(sizeof(*phrase) * PASSWORD_MAX);
    memWipe(phrase, sizeof(*phrase) * PASSWORD_MAX);
    printf("%s", prompt);
    scanf("%s", phrase);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
    printf("\n");
    return phrase;
}

char *getNewPath(const char *path_prefix, const char *name, const char *extension)
{
    char *path = (char *) malloc(sizeof(*path) * FILENAME_MAX);
    memWipe(path, sizeof(*path) * FILENAME_MAX);
    strcat(path, path_prefix);
    strcat(path, "/");
    strcat(path, name);
    strcat(path, extension);
    return path;
}

void writeDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size)
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

void readHexFromStr(unsigned char *hex_arr, const long int hex_arr_size, const char *str)
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

char *getConfigPath(void)
{
    char *config_path = (char *) malloc(sizeof(*config_path) * FILENAME_MAX);
    memWipe(config_path, sizeof(*config_path) * FILENAME_MAX);
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strcat(config_path, homedir);
    strcat(config_path, "/.config/" PROGRAM_NAME);
    return config_path;
}

void printBaseName(char *name) {
    size_t k = strlen(name) - 1;
    while (k > 0 && name[k] != '.') {
        k--;
    }

    printf("    ");
    for (size_t i = 0; i < k; i++) {
        printf("%c", name[i]);
    }
    printf("\n");
}
