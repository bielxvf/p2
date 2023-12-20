/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */
#include <sys/stat.h>
#include "./GetConfigPath.h"
#include "./macros.h"

void
MkConfigDir(void) // Will create ~/.config/p2 if it doesn't exist
{
    struct stat st = {0};

    char *config_path = GetConfigPath();

    if (stat(config_path, &st) == -1) {
        PrintError(INFO "'%s' does not exist, creating new", config_path);
        mkdir(config_path, 0700);
    }
}
