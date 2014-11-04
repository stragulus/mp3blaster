#include <stdio.h>

main(int argc, char **argv)
{
    FILE *fFrom, *fTo;
    unsigned char in, out;
    int i=0;

    fFrom=fopen(argv[1], "r");
    fTo=fopen(argv[2], "r");

    while(i<256)
    {
        in=fgetc(fFrom);
        out=fgetc(fTo);
        printf("0x%.2x\t0x%.2x\n", in, out);
        i++;
    }
    fclose(fFrom);
    fclose(fTo);
}