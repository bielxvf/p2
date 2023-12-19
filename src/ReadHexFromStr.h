/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */

void
ReadHexFromStr(unsigned char *hex_arr, const long int hex_arr_size, const char *str)
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
