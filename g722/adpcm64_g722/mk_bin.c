#include <stdio.h>
main()
{
    int i;
    char sc[2], t;
    short s;

    while (fscanf(stdin, "%d", &i) == 1) {
	s = i;
	fwrite(&s, sizeof(short), 1, stdout);
    }
    exit(0);
}
