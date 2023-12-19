/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */

void
WriteDataToFile(const char *path, const unsigned char *nonce, const size_t nonce_size, const unsigned char *ciphertext, const size_t ciphertext_size)
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
