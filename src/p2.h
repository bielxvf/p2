#include <sys/stat.h>
#include <sys/time.h>
#include <sodium.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <termios.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <libtar.h>
#include <fcntl.h>

#define COPT_IMPLEMENTATION
#include "./copt.h"

#define PASSWORD_MAX 4096
#define EXTENSION_LOCKED ".locked"

#define ERROR  "\033[31;1;3m[ERROR] \033[0m"
#define INFO   "\033[36;1;3m[INFO]  \033[0m"

#define ITALIC_BOLD_BLUE "\033[94;1;3m"
#define COLOR_RESET "\033[0m"

#define UNUSED(a)   \
    do {            \
        (void) (a); \
    } while(0)

void printError(const char *fmt, ...);
void printInfo(const char *fmt, ...);
void memWipe(void *p, int len);
void fileWipe(const char *path);
void mkConfigDir();
char *getConfigPath();
void printDirContents(char *path);
void printBaseName(char *name);
char *getPassPhrase(const char *prompt);
char *getNewPath(const char *path_prefix, const char *name, const char *extension);
void writeDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size);
void readHexFromStr(unsigned char *hex_arr, const long int hex_arr_size, const char *str);

int cmdHelp(const int argc, const char **argv);
int cmdVersion(const int argc, const char **argv);
int cmdList(const int argc, const char **argv);
int cmdNew(const int argc, const char **argv);
int cmdPrint(const int argc, const char **argv);
int cmdDelete(const int argc, const char **argv);
int cmdCopy(const int argc, const char **argv);
int cmdRename(const int argc, const char **argv);

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

void mkConfigDir()
{
    struct stat st = {0};

    char *config_path = getConfigPath();

    if (stat(config_path, &st) == -1) {
        printInfo("'%s' does not exist, creating new\n", config_path);
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
    fprintf(stderr, "%s", prompt);
    (void)!scanf("%s", phrase);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
    fprintf(stderr, "\n");
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

char *getConfigPath()
{
    char *config_path = (char *) malloc(sizeof(*config_path) * FILENAME_MAX);
    memWipe(config_path, sizeof(*config_path) * FILENAME_MAX);
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    strcat(config_path, homedir);
    strcat(config_path, "/.config/");
    strcat(config_path, program.name);
    return config_path;
}

void printBaseName(char *name) {
    size_t k = strlen(name) - 1;
    while (k > 0 && name[k] != '.') {
        k--;
    }

    printf("\t"ITALIC_BOLD_BLUE);
    for (size_t i = 0; i < k; i++) {
        printf("%c", name[i]);
    }
    printf("\n"COLOR_RESET);
}

int cmdHelp(const int argc, const char **argv)
{
    UNUSED(argv);

    if (argc != 2 && argc != 1) {
        printError("Incorrect arguments for subcommand 'HELP'");
        return 1;
    }

    copt_print_help();
    return 1;
}

int cmdVersion(const int argc, const char **argv)
{
    UNUSED(argv);

    if (argc != 2) {
        printError("Incorrect arguments for subcommand 'VERSION'");
        return 1;
    }

    copt_print_version();
    return 0;
}

int cmdList(const int argc, const char **argv)
{
    UNUSED(argv);

    if (argc != 2) {
        printError("Incorrect arguments for subcommand 'LIST'");
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
        printInfo("'%s' looks empty. Create a new password with `%s new [NAME]`\n", path, program.name);
        return 0;
    }

    return 0;
}

int cmdNew(const int argc, const char **argv)
{
    if (argc != 3) {
        printError("Incorrect arguments for subcommand 'NEW'");
        return 1;
    }

    mkConfigDir();

    char *new_path = getNewPath(getConfigPath(), argv[2], EXTENSION_LOCKED);
    struct stat st;
    if (!stat(new_path, &st)) {
        printError("Invalid name: '%s'. File '%s' already exists", argv[2], new_path);
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

int cmdPrint(const int argc, const char **argv)
{
    if (argc != 3) {
        printError("Incorrect arguments for subcommand 'PRINT'");
        return 1;
    }

    mkConfigDir();

    char *print_path = getNewPath(getConfigPath(), argv[2], EXTENSION_LOCKED);
    struct stat st;
    if (stat(print_path, &st)) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[2], print_path);
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
    (void)!getline(&str_nonce, &line_len, fptr);

    (void)!getline(&line, &line_len, fptr);
    ciphertext_size = strtol(line, NULL, 10);
    size_t plaintext_len = ciphertext_size - crypto_secretbox_MACBYTES;

    char *str_ciphertext = (char *) malloc(sizeof(*str_ciphertext) * PASSWORD_MAX);
    (void)!getline(&str_ciphertext, &line_len, fptr);
    fclose(fptr);

    unsigned char *nonce = (unsigned char *) malloc(nonce_size);
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

int cmdDelete(const int argc, const char **argv)
{
    if (argc != 3) {
        printError("Incorrect arguments for subcommand 'DELETE'");
        return 1;
    }

    mkConfigDir();

    char *remove_path = getNewPath(getConfigPath(), argv[2], EXTENSION_LOCKED);
    struct stat st;
    if (stat(remove_path, &st)) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[2], remove_path);
        free(remove_path);
        return 1;
    }

    printInfo("Are you sure you want to remove '%s'?\n", remove_path);
    printInfo("You will not be able to recover the data. Remove? [y/N] ");
    char a = '\0';
    a = getchar();
    if (a != 'y' && a != 'Y') {
        printError("Aborting deletion");
        return 1;
    }

    fileWipe(remove_path);
    remove(remove_path);
    printInfo("Removed file: '%s'\n", remove_path);

    return 0;
}

int cmdCopy(const int argc, const char **argv)
{
    if (argc != 3) {
        printError("Incorrect arguments for subcommand 'COPY'");
        return 1;
    }

    mkConfigDir();

    char *copy_path = getNewPath(getConfigPath(), argv[2], EXTENSION_LOCKED);
    struct stat st;
    if (stat(copy_path, &st)) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[2], copy_path);
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
    (void)!getline(&str_nonce, &line_len, fptr);

    (void)!getline(&line, &line_len, fptr);
    ciphertext_size = strtol(line, NULL, 10);
    size_t plaintext_len = ciphertext_size - crypto_secretbox_MACBYTES;

    char *str_ciphertext = (char *) malloc(sizeof(*str_ciphertext) * PASSWORD_MAX);
    (void)!getline(&str_ciphertext, &line_len, fptr);
    fclose(fptr);

    unsigned char *nonce = (unsigned char *) malloc(nonce_size);
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

    char filename_out[FILENAME_MAX];
    struct timeval time;
    gettimeofday(&time, NULL);
    sprintf(filename_out, "/tmp/%s.%ld.txt", program.name, time.tv_usec);
    FILE *file_out = fopen(filename_out, "w");
    for (size_t i = 0; i < plaintext_len; i++) {
        fprintf(file_out, "%c", decrypted[i]);
    }
    fclose(file_out);
    char cmd[strlen(filename_out) + 34];
    sprintf(cmd, "cat %s | xclip -selection CLIPBOARD", filename_out);
    (void)!system(cmd);
    remove(filename_out);

    memWipe(decrypted, sizeof(*decrypted) * plaintext_len);
    free(password);
    free(copy_path);
    free(line);
    free(str_nonce);
    free(str_ciphertext);
    free(nonce);
    return 0;
}

int cmdRename(const int argc, const char **argv)
{
    if (argc != 4) {
        printError("Incorrect arguments for subcommand 'RENAME'");
        return 1;
    }

    char *rename_path = getNewPath(getConfigPath(), argv[2], EXTENSION_LOCKED);
    struct stat st;
    if (stat(rename_path, &st)) {
        printError("Invalid name: '%s'. File '%s' does not exist", argv[2], rename_path);
        free(rename_path);
        return 1;
    }

    char *new_path = getNewPath(getConfigPath(), argv[3], EXTENSION_LOCKED);
    if (!stat(new_path, &st)) {
        printError("Invalid name: '%s'. File '%s' already exists", argv[3], new_path);
        free(new_path);
        return 1;
    }

    if (rename(rename_path, new_path)) {
        printError("%s", strerror(errno));
        return 1;
    }

    return 0;
}

int cmdBackup(const int argc, const char **argv)
{
    if (argc != 3) {
        printError("Incorrect arguments for subcommand 'BACKUP'");
        return 1;
    }

    TAR *ptar;
    char *program_name = (char *)program.name;
    tar_open(&ptar, argv[2], NULL, O_WRONLY | O_CREAT, 0644, TAR_GNU);
    tar_append_tree(ptar, getConfigPath(), program_name);
    tar_append_eof(ptar);
    tar_close(ptar);

    return 0;
}

int cmdRestore(const int argc, const char **argv)
{
    if (argc != 3) {
        printError("Incorrect arguments for subcommand 'RESTORE'");
        return 1;
    }

    return 0;
}
