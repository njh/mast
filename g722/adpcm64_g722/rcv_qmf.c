/******************************* RECIEVE QMF ************************/

#include <stdio.h>
main ()
{
	int i;                 /* counter                       */
	int rl, rh;            /* low and high band pcm input   */
	int xout1, xout2;      /* even and odd tap accumulators */
        FILE *infile;          /* input file pointer            */        
        FILE *outfile;         /* output file pointer           */         
	static int h[24] =     /* qmf tap coefficients          */
		{3,	-11,	-11,	53,	12,	-156,
		32,	362,	-210,	-805,	951,	3876,
		3876,	951,	-805,	-210,	362,	32,
		-156,	12,	53,	-11,	-11,	3} ;
	static int xd[12] =
		{0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0} ;
	static int xs[12] =
		{0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0} ;

        infile = fopen("testout.dat", "r");
        outfile = fopen("pcmout.dat", "w+");
nextpcm:
	fscanf(infile, "%d%d", &rl, &rh) ;
	if (rl == 32767) goto finish;
	for (i=11; i>0; i--) { xd[i] = xd[i-1]; xs[i] = xs[i-1]; }
/************************************* RECA ***************************/
	xd[0] = rl - rh ;
        if (xd[0] > 16383) xd[0] = 16383;
        if (xd[0] < -16384) xd[0] = -16384;
/************************************* RECB ***************************/
	xs[0] = rl + rh ;
        if (xs[0] > 16383) xs[0] = 16383;
        if (xs[0] < -16384) xs[0] = -16384;
/************************************* ACCUMC *************************/
	xout1 = 0;
	for (i=0; i<12; i+=1) xout1 += xd[i] * h[2*i];
	xout1 = xout1 >> 12 ;
	if (xout1 >  16383)  xout1 =  16383 ;
	if (xout1 < -16384)  xout1 = -16384 ;
/************************************* ACCUMD *************************/
	xout2 = 0;
	for (i=0; i<12; i+=1) xout2 += xs[i] * h[2*i+1];
	xout2  = xout2  >> 12 ;
	if (xout2  >  16383)  xout2  =  16383 ;
	if (xout2  < -16384)  xout2  = -16384 ;
/************************************* SELECT *************************/
	fprintf (outfile, "%5d\n%5d\n", xout1, xout2 ) ;
	goto nextpcm;

finish:
	fprintf (outfile, "32767 32767 \n");

}
