#include <stdio.h>
main ()
{

/*  This program performs the transmit qmf function for the 64 kb/s */
/*  codec.  It reads 14 bit decimial PCM values from standard input */
/*  and writes 14 bit high and low band PCM values to standard      */
/*  output, two to the line.  The input must be terminated by the   */
/*  PCM value of 32767, and the last output line contains 32767     */
/*  and 32767 as a terminator for that file.                        */

	int i;                     /* counter                       */
	int pcm;                   /* pcm input from stdin          */
	int pcmlow, pcmhigh;       /* low and high band pcm from qmf*/
	int sumeven, sumodd;       /* even and odd tap accumulators */
	int decimate = 1;          /* switch used to decimate qmf   */
        FILE *infile;              /* input file pointer            */
        FILE *outfile;             /* output file pointer           */
				   /* output every other time       */

	static int h[24] =         /* qmf tap coefficients          */
		{3,	-11,	-11,	53,	12,	-156,
		32,	362,	-210,	-805,	951,	3876,
		3876,	951,	-805,	-210,	362,	32,
		-156,	12,	53,	-11,	-11,	3} ;
	static int x[24] =         /* storage for signal passing    */
				   /* through the qmf               */
		{0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0};

/* BEGINNING OF EXECUTION, READ IN A PCM SAMPLE FROM STDIN          */

        infile = fopen("pcmin.dat", "r");
        outfile = fopen("testin.dat", "w+"); 
nextpcm:
	fscanf(infile, "%d", &pcm);
	if (pcm == 32767) goto finish;

/* PROCESS PCM THROUGH THE QMF FILTER                               */

	for (i=23; i>0; i--) x[i] = x[i-1];
	x[0] = pcm;

/* DISCARD EVERY OTHER QMF OUTPUT                                   */

        decimate = - decimate;
	if ( decimate < 0 ) goto nextpcm;

	sumodd = 0;
	for (i=1; i<24; i+=2) sumodd += x[i] * h[i];

	sumeven = 0;
	for (i=0; i<24; i+=2) sumeven += x[i] * h[i];

	pcmlow  = (sumeven + sumodd) >> 13  ;
	pcmhigh = (sumeven - sumodd) >> 13  ;

	if (pcmlow  >  16383) pcmlow  =  16383 ;
	if (pcmlow  < -16384) pcmlow  = -16384 ;
	if (pcmhigh >  16383) pcmhigh =  16383 ;
	if (pcmhigh < -16383) pcmhigh = -16383 ;


writeadp:
	fprintf (outfile, "%d %d \n", pcmlow, pcmhigh);
	goto nextpcm;

/* FINISH UP BY WRITING 32767 AT THE END OF THE FILE                */

finish:
	fprintf (outfile, "32767 32767 \n");

}


