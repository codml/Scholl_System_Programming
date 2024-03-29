#include <unistd.h>
#include <stdio.h>

int main (int argc, char **argv)
{
    int aflag= 0, bflag= 0;
    char *cvalue= NULL;
    int index, c;
    opterr = 0;

    index = 1;

    while ((c = getopt(argc, argv, "abc:")) != -1)
    {
        switch (c)
        {
        case 'a':
            aflag = 1;
            break;
        case 'b':
            bflag = 1;
            break;
        case 'c':
            cvalue = optarg;
            break;
        }
        index++;
    }
    printf("aflag= %d, bflag= %d, cvalue= %s\n", aflag, bflag, cvalue);
    
    for (int i = index; i < argc; i++)
        printf("Non-option argument %s\n", argv[i]);

    return 0;
}