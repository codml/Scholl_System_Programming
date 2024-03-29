#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char **argv)
{
    int aflag= 0, bflag= 0;
    char *cvalue= NULL;
    int index, c;
    opterr = 0;

    while ((c = getopt(argc, argv, "abc:")) != -1)
    {
        switch (c)
        {
        case 'a':
            aflag++;
            break;
        case 'b':
            bflag++;
            break;
        case 'c':
            cvalue = optarg;
            break;
        }
    }
    printf("aflag= %d, bflag= %d, cvalue= %s\n", aflag, bflag, cvalue);
 
    for (index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]);
    return 0;
}
