/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */

#include "./macros.h"

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
