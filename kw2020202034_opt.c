///////////////////////////////////////////////////////////////////
// File Name    : kw2020202034.c                                 //
// Date         : 2024/04/01                                     //
// OS           : Ubuntu 20.04.6. LTS 64bit                      //
        //
// Author       : Kim Tae Wan                                    //
// Student ID   : 2020202034                                     //
// ------------------------------------------------------------- //
// Title : System Programming Assignment #1-1 (ftp server)       //
// Description : parsing arguments using getopt                  //
///////////////////////////////////////////////////////////////////

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
        //////////////// actions for optional arguments ////////////////
        switch (c)
        {
        case 'a':
            aflag++; // aflag = aflag + 1
            break;
        case 'b':
            bflag++; // bflag = bflag + 1
            break;
        case 'c':
            cvalue = optarg; // cvalue points to the argument(string)
            break;
        }
    }
    printf("aflag = %d, bflag = %d, cvalue = %s\n", aflag, bflag, cvalue);
 
    ////////////////////print non-option arguments///////////////////
    for (index = optind; index < argc; index++) // start from optind to argc - 1
        printf("Non-option argument %s\n", argv[index]);
    return 0;
}
