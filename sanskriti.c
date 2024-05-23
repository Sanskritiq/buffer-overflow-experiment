#include <stdio.h>
#include <time.h>

int main()
{
    printf("Name: Sanskriti\n");
    printf("Class: CSE\n");
    time_t now = time(NULL);
    printf("Date and Time: %s\n", ctime(&now));
    return 0;
}
