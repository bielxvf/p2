/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */
#include <sys/stat.h>

void
FileWipe(const char *path)
{
    struct stat st;
    stat(path, &st);
    FILE *fptr = fopen(path, "w");
    for (int i = 0; i < st.st_size; i++) {
        fprintf(fptr, "%c", 0);
    }
    fclose(fptr);
}
