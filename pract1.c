#include <stdio.h>
#include <string.h>

int main(void)
{
    char string[80];
    strcpy(string, "Hello world from ");
    strcat(string, "strcpy");
    strcat(string, "and ");
    strcat(string, "strcat!");
    printf("String = %s\n", string);
    return 0;
}