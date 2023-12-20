/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */
#include <dirent.h>

void
PrintDirContents(char *path)
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
