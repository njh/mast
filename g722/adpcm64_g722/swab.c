#include <stdio.h>
main()
{
    int i;
    char sc[2], t;
    short s;

    while (fscanf(stdin, "%d", &i) == 1) {
	s = i;
	bcopy(&s, sc, sizeof(short));
	t = sc[0]; sc[0] = sc[1]; sc[1] = t;
	bcopy(sc, &s, sizeof(short));
	fprintf(stdout, "%d\n", s);
    }
}
