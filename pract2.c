#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int aflag = 0, bflag = 0;
    char *cvalue = NULL;
    int c = 0;

    while ((c = getopt(argc, argv, "abdc:")) != -1)
    {
        //===========print the getopt variable==========//
        printf("optarg %s\t optind = %d\t opterr = %d\t optopt = %c\n", optarg, optind, opterr, optopt);
        //==============================================//

        switch(c)
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
            case 'd':
                opterr = 0;
                break;
            case '?':
                printf("Unknown option character\n");
                break;
        }
    }
    //==========print flags and c's argument===========//
	printf("\n aflag = %d\t bflag = %d\t cvalue = %s\n", aflag, bflag, cvalue);
	return 0;
}