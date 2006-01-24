/* arguments (band[0|1|2], mode[1|2|3])*/
#include <stdio.h>
 

main ( )
{
	int band ;
	int mode = 1 ;
	int ilowr, dlowt, ylow, rlow ;
	int slow = 0 ;
	int detlow  = 32 ;
	int block2l(), block3l(), block4l(), block5l(), block6l() ;
	int ihigh, dhigh, rhigh ;
        int shigh = 0;
	int dethigh = 8 ;
	int block2h(), block3h(), block4h(), block5h() ;
        char ch = ' ';
        FILE *infile;
        FILE *outfile;

        printf("%16c bands: 0: Both, \n %22c 1: Low only, \n %22c",  
ch,ch,ch);
        printf(" 2: High only. \n\n");
        printf("%17c mode: 1: 6 bit codeword, \n %23c", ch, ch);
        printf("2: 5 bit codeword, \n %22c 3: 4 bit codeword. \n\n",  
ch);
        printf("Type bands and mode \n");

	scanf ("%d %d", &band, &mode ) ;

        infile = fopen("adpcm.dat", "r");
        outfile = fopen("testout.dat", "w+");
 

next:	if (band == 2) goto high ;
low:	fscanf (infile, "%o", &ilowr) ;                 /*input low  
band adpcm*/

	if (ilowr == 32767) goto finish ;
	ylow = block5l (ilowr, slow, detlow, mode) ;	/*low band*/
	rlow = block6l (ylow) ;				/*decoder */
	dlowt = block2l (ilowr, detlow) ;
	detlow = block3l (ilowr) ;
	slow = block4l (dlowt) ;
	fprintf (outfile, "%d ", rlow) ;                 /*output low  
band pcm*/

 

	if (band == 1) goto low;
high:	fscanf (infile, "%o", &ihigh) ;		       /*input high  
band adpcm*/

	if (ihigh == 32767) goto finish ;
	dhigh = block2h (ihigh, dethigh) ;		/*high band*/
	rhigh = block5h (dhigh, shigh) ;		/*decoder  */
	dethigh = block3h (ihigh) ;
	shigh = block4h (dhigh) ;
	fprintf (outfile, "%d \n", rhigh) ;			 
/*output high ba
nd pcm*/
	goto next;
 

 

finish: fprintf (outfile, "32767 32767\n") ;
}
 

/*************************** BLOCK 2L  
*******************************/
 

block2l (il, detl)
int il, detl ;
{
int dlt ;
	int ril, wd2 ;
	static int qm4[16] = 

		{0,	-20456,	-12896,	-8968, 

		-6288,	-4240,	-2584,	-1200,
		20456,	12896,	8968,	6288,
		4240,	2584,	1200,	0 } ;
/************************************** BLOCK 2L, INVQAL  
************/
 

	ril = il >> 2 ;
	wd2 = qm4[ril] ;
	dlt = (detl * wd2) >> 15 ;
 

	return (dlt) ;
}
 

/***************************** BLOCK 3L  
*******************************/
 

block3l (il)
int il ;
{
int detl ;
	int ril, il4, wd, wd1, wd2, wd3, nbpl, depl ;
	static int nbl = 0 ;
	static int wl[8] = {-60, -30, 58, 172, 334, 538, 1198, 3042 }  
;
	static int rl42[16] = {0, 7, 6, 5, 4, 3, 2, 1, 7, 6, 5, 4, 3,  
2,
		1, 0 } ;
	static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282,  
2332,
		2383, 2435, 2489, 2543, 2599, 2656, 2714, 

		2774, 2834, 2896, 2960, 3025, 3091, 3158, 

		3228, 3298, 3371, 3444, 3520, 3597, 3676,
		3756, 3838, 3922, 4008 } ;
/************************************** BLOCK 3L, LOGSCL  
*************/
 

	ril = il >> 2 ;
	il4 = rl42[ril] ;
	wd = (nbl * 32512) >> 15 ;
	nbpl = wd + wl[il4] ;
 

	if (nbpl <     0) nbpl = 0 ;
	if (nbpl > 18432) nbpl = 18432 ;
 

/************************************** BLOCK 3L, SCALEL  
*************/
	wd1 =  (nbpl >> 6) & 31 ;
	wd2 = nbpl >> 11 ;
	if ((8 - wd2) < 0)   wd3 = ilb[wd1] << (wd2 - 8) ;
	else wd3 = ilb[wd1] >> (8 - wd2) ;
	depl = wd3 << 2 ;
/************************************** BLOCK 3L, DELAYA  
*************/
	nbl = nbpl ;
/************************************** BLOCK 3L, DELAYL  
*************/
	detl = depl ;
 

	return (detl) ;
}
 

/**************************** BLOCK 4L  
*******************************/
 

block4l (dl)
int dl ;
{
static int sl = 0 ;
	int i ;
	int wd, wd1, wd2, wd3, wd4, wd5, wd6 ;
	static int spl = 0 ;
	static int szl = 0 ;
	static int rlt  [3] = { 0, 0, 0 } ;
	static int al   [3] = { 0, 0, 0 } ;
	static int apl  [3] = { 0, 0, 0 } ;
	static int plt  [3] = { 0, 0, 0 } ;
	static int dlt  [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
	static int bl   [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
	static int bpl  [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
	static int sg   [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
 

	dlt[0] = dl;
/*************************************** BLOCK 4L, RECONS  
***********/ 

 

	rlt[0] = sl + dlt[0] ;
        if (rlt[0] > 32767) rlt[0] = 32767;
        if (rlt[0] < -32768) rlt[0] = -32768;
/*************************************** BLOCK 4L, PARREC  
***********/
 

	plt[0] = dlt[0] + szl ;
        if (plt[0] > 32767) plt[0] = 32767;
        if (plt[0] < -32768) plt[0] = -32768;
 

/*****************************BLOCK 4L,  
UPPOL2*************************/
 

	sg[0] = plt[0] >> 15 ;
	sg[1] = plt[1] >> 15 ;
	sg[2] = plt[2] >> 15 ;
 

	wd1 = al[1] + al[1] ;
	wd1 = wd1 + wd1 ;
        if (wd1 > 32767) wd1 = 32767;
        if (wd1 < -32768) wd1 = -32768;
 

	if ( sg[0] == sg[1] )  wd2 = - wd1 ;
	else  wd2 = wd1 ;
        if (wd2 > 32767) wd2 = 32767;

	wd2 = wd2 >> 7 ;
 

	if ( sg[0] == sg[2] )  wd3 = 128 ; 

		   else  wd3 = - 128 ;
 

	wd4 = wd2 + wd3 ;
	wd5 = (al[2] * 32512) >> 15 ;
 

	apl[2] = wd4 + wd5 ;
	if ( apl[2] >  12288 )  apl[2] =  12288 ;
	if ( apl[2] < -12288 )  apl[2] = -12288 ;
/************************************* BLOCK 4L, UPPOL1  
***************/
 

	sg[0] = plt[0] >> 15 ;
	sg[1] = plt[1] >> 15 ;
 

	if ( sg[0] == sg[1] )  wd1 = 192 ;
	if ( sg[0] != sg[1] )  wd1 = - 192 ;
 

	wd2 = (al[1] * 32640) >> 15 ;
 

	apl[1] = wd1 + wd2 ;
        if (apl[1] > 32767) apl[1] = 32767;
        if (apl[1] < -32768) apl[1] = -32768;
 

	wd3 = (15360 - apl[2]) ;
        if (wd3 > 32767) wd3 = 32767;
        if (wd3 < -32768) wd3 = -32768;
	if ( apl[1] >  wd3)  apl[1] =  wd3 ;
	if ( apl[1] < -wd3)  apl[1] = -wd3 ;
 

/*************************************** BLOCK 4L, UPZERO  
************/
 

	if ( dlt[0] == 0 )  wd1 = 0 ;
	if ( dlt[0] != 0 )  wd1 = 128 ;
 

	sg[0] = dlt[0] >> 15 ;
 

	for ( i = 1; i < 7; i++ ) {
		sg[i] = dlt[i] >> 15 ;
		if ( sg[i] == sg[0] )  wd2 = wd1 ;
		if ( sg[i] != sg[0] )  wd2 = - wd1 ;
		wd3 = (bl[i] * 32640) >> 15 ;
		bpl[i] = wd2 + wd3 ;
                if (bpl[i] > 32767) bpl[i] = 32767;
                if (bpl[i] < -32768) bpl[i] = -32768;
	}
/********************************* BLOCK 4L, DELAYA  
******************/
	for ( i = 6; i > 0; i-- ) {
		dlt[i] = dlt[i-1] ;
		bl[i]  = bpl[i] ;
	}
 

	for ( i = 2; i > 0; i-- ) {
		rlt[i] = rlt[i-1] ;
		plt[i] = plt[i-1] ;
		al[i] = apl[i] ;
	}
/********************************* BLOCK 4L, FILTEP  
******************/
 

	wd1 = ( rlt[1] + rlt[1] ) ;
        if (wd1 > 32767) wd1 = 32767;
        if (wd1 < -32768) wd1 = -32768;
	wd1 = ( al[1] * wd1 ) >> 15 ;
 

	wd2 = ( rlt[2] + rlt[2] ) ;
        if (wd2 > 32767) wd2 = 32767;
        if (wd2 < -32768) wd2 = -32768;
	wd2 = ( al[2] * wd2 ) >> 15 ;
 

	spl = wd1 + wd2 ;
        if (spl > 32767) spl = 32767;
        if (spl < -32768) spl = -32768;
 

/*************************************** BLOCK 4L, FILTEZ  
***********/
 

	szl = 0 ;
	for (i=6; i>0; i--) {
		wd = (dlt[i] + dlt[i]) ;
                if (wd > 32767) wd = 32767;
                if (wd < -32768) wd = -32768;
    		szl += (bl[i] * wd) >> 15 ;
                if (szl > 32767) szl = 32767;
                if (szl < -32768) szl = -32768;
   

	}
 

/*********************************BLOCK 4L, PREDIC  
*******************/
 

	sl = spl + szl ;
        if (sl > 32767) sl = 32767;
        if (sl < -32768) sl = -32768;

        return (sl) ;
}
 

/*********************************BLOCK 5L  
***************************/
 

block5l (ilr, sl, detl, mode)
int ilr, sl, detl, mode ;
{
int yl ;
 

	int ril, dl, wd2 ;
	static int qm4[16] = 

		{0,	-20456,	-12896,	-8968, 

		-6288,	-4240,	-2584,	-1200,
		20456,	12896,	8968,	6288,
		4240,	2584,	1200,	0 } ;
	static int qm5[32] =
		{-280,	-280,	-23352,	-17560,
		-14120,	-11664,	-9752,	-8184,
		-6864,	-5712,	-4696,	-3784,
		-2960,	-2208,	-1520,	-880,
		23352,	17560,	14120,	11664,
		9752,	8184,	6864,	5712,
		4696,	3784,	2960,	2208,
		1520,	880,	280,	-280 } ;
	static int qm6[64] =
		{-136,	-136,	-136,	-136,
		-24808,	-21904,	-19008,	-16704,
		-14984,	-13512,	-12280,	-11192,
		-10232,	-9360,	-8576,	-7856,
		-7192,	-6576,	-6000,	-5456,
		-4944,	-4464,	-4008,	-3576,
		-3168,	-2776,	-2400,	-2032,
		-1688,	-1360,	-1040,	-728,
		24808,	21904,	19008,	16704,
		14984,	13512,	12280,	11192,
		10232,	9360,	8576,	7856,
		7192,	6576,	6000,	5456,
		4944,	4464,	4008,	3576,
		3168,	2776,	2400,	2032,
		1688,	1360,	1040,	728,
		432,	136,	-432,	-136 } ;
/********************************** BLOCK 5L, INVQBL  
****************/
 

	if (mode == 1) {
	ril = ilr ;
	wd2 = qm6[ril] ;
	}
 

	if (mode == 2) {
	ril = ilr >> 1 ;
	wd2 = qm5[ril] ;
	}
 

	if (mode == 3) {
	ril = ilr >> 2 ;
	wd2 = qm4[ril] ;
	}
 

	dl = (detl * wd2 ) >> 15 ;
 

/******************************** BLOCK 5L, RECONS ****************/
 

	yl = sl + dl ;
        if (yl > 32767) yl = 32767;
        if (yl < -32768) yl = -32768;

	return (yl) ;
}
/******************************** BLOCK 6L, LIMIT ******************/
block6l (yl)
int yl ;
{
int rl ;
 

	rl = yl ;
	if (yl >  16383 )  rl =  16383 ;
	if (yl < -16384 )  rl = -16384 ;
 

	return (rl) ;
}
 

/*************************** BLOCK 2H  
*******************************/
 

block2h (ih, deth)
int ih, deth ;
{
int dh ;
	int wd2 ;
	static int qm2[4] = 

		{-7408,	-1616,	7408,	1616} ;
/************************************** BLOCK 2H, INVQAH  
************/
 

	wd2 = qm2[ih] ;
	dh = (deth * wd2) >> 15 ;
 

	return (dh) ;
}
 

/***************************** BLOCK 3H  
*******************************/
 

block3h (ih)
int ih ;
{
int deth ;
	int ih2, wd, wd1, wd2, wd3, nbph, deph ;
	static int nbh = 0 ;
	static int wh[3] = {0, -214, 798} ;
	static int rh2[4] = {2, 1, 2, 1} ;
	static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282,  
2332,
		2383, 2435, 2489, 2543, 2599, 2656, 2714, 

		2774, 2834, 2896, 2960, 3025, 3091, 3158, 

		3228, 3298, 3371, 3444, 3520, 3597, 3676,
		3756, 3838, 3922, 4008 } ;
/************************************** BLOCK 3H, LOGSCH  
*************/
 

	ih2 = rh2[ih] ;
	wd = (nbh * 32512) >> 15 ;
	nbph = wd + wh[ih2] ;
 

	if (nbph <     0) nbph = 0 ;
	if (nbph > 22528) nbph = 22528 ;
 

/************************************** BLOCK 3H, SCALEH  
*************/
	wd1 =  (nbph >> 6) & 31 ;
	wd2 = nbph >> 11 ;
	if ((10 - wd2) < 0) wd3 = ilb[wd1] << (wd2 - 10) ;
	else wd3 = ilb[wd1] >> (10 - wd2) ;
	deph = wd3 << 2 ;
/************************************** BLOCK 3L, DELAYA  
*************/
	nbh = nbph ;
/************************************** BLOCK 3L, DELAYH  
*************/
	deth = deph ;
 

	return (deth) ;
}
 

/**************************** BLOCK 4H  
*******************************/
 

block4h (d)
int d ;
{
static int sh = 0 ;
	int i ;
	int wd, wd1, wd2, wd3, wd4, wd5, wd6 ;
	static int sph = 0 ;
	static int szh = 0 ;
	static int rh  [3] = { 0, 0, 0 } ;
	static int ah   [3] = { 0, 0, 0 } ;
	static int aph  [3] = { 0, 0, 0 } ;
	static int ph  [3] = { 0, 0, 0 } ;
	static int dh  [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
	static int bh   [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
	static int bph  [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
	static int sg   [7] = { 0, 0, 0, 0, 0, 0, 0 } ;
 

	dh[0] = d;
/*************************************** BLOCK 4H, RECONS  
***********/ 

 

	rh[0] = sh + dh[0] ;
        if (rh[0] > 32767) rh[0] = 32767;
        if (rh[0] < -32768) rh[0] = -32768;

/*************************************** BLOCK 4H, PARREC  
***********/
 

	ph[0] = dh[0] + szh ;
        if (ph[0] > 32767) ph[0] = 32767;
        if (ph[0] < -32768) ph[0] = -32768;
 

/*****************************BLOCK 4H,  
UPPOL2*************************/
 

	sg[0] = ph[0] >> 15 ;
	sg[1] = ph[1] >> 15 ;
	sg[2] = ph[2] >> 15 ;
 

	wd1 = ah[1] + ah[1] ;
	wd1 = wd1 + wd1 ;
        if (wd1 > 32767) wd1 = 32767;
        if (wd1 < -32768) wd1 = -32768;

	if ( sg[0] == sg[1] )  wd2 = - wd1 ;
	else  wd2 = wd1 ;
        if (wd2 > 32767) wd2 = 32767;
 

	wd2 = wd2 >> 7 ;
 

	if ( sg[0] == sg[2] )  wd3 = 128 ; 

		   else  wd3 = - 128 ;
 

	wd4 = wd2 + wd3 ;
	wd5 = (ah[2] * 32512) >> 15 ;
 

	aph[2] = wd4 + wd5 ;
	if ( aph[2] >  12288 )  aph[2] =  12288 ;
	if ( aph[2] < -12288 )  aph[2] = -12288 ;
/************************************* BLOCK 4H, UPPOL1  
***************/
 

	sg[0] = ph[0] >> 15 ;
	sg[1] = ph[1] >> 15 ;
 

	if ( sg[0] == sg[1] )  wd1 = 192 ;
	if ( sg[0] != sg[1] )  wd1 = - 192 ;
 

	wd2 = (ah[1] * 32640) >> 15 ;
 

	aph[1] = wd1 + wd2 ;
        if (aph[2] > 32767) aph[2] = 32767;
        if (aph[2] < -32768) aph[2] = -32768;
 

	wd3 = (15360 - aph[2]) ;
        if (wd3 > 32767) wd3 = 32767;
        if (wd3 < -32768) wd3 = -32768;
	if ( aph[1] >  wd3)  aph[1] =  wd3 ;
	if ( aph[1] < -wd3)  aph[1] = -wd3 ;
 

/*************************************** BLOCK 4H, UPZERO  
************/
 

	if ( dh[0] == 0 )  wd1 = 0 ;
	if ( dh[0] != 0 )  wd1 = 128 ;
 

	sg[0] = dh[0] >> 15 ;
 

	for ( i = 1; i < 7; i++ ) {
		sg[i] = dh[i] >> 15 ;
		if ( sg[i] == sg[0] )  wd2 = wd1 ;
		if ( sg[i] != sg[0] )  wd2 = - wd1 ;
		wd3 = (bh[i] * 32640) >> 15 ;
		bph[i] = wd2 + wd3 ;
 

	}
/********************************* BLOCK 4H, DELAYA  
******************/
	for ( i = 6; i > 0; i-- ) {
		dh[i] = dh[i-1] ;
		bh[i]  = bph[i] ;
	}
 

	for ( i = 2; i > 0; i-- ) {
		rh[i] = rh[i-1] ;
		ph[i] = ph[i-1] ;
		ah[i] = aph[i] ;
	}
/********************************* BLOCK 4H, FILTEP  
******************/
 

	wd1 = ( rh[1] + rh[1] ) ;
        if (wd1 > 32767) wd1 = 32767;
        if (wd1 < -32768) wd1 = -32768;
	wd1 = ( ah[1] * wd1 ) >> 15 ;
 

	wd2 = ( rh[2] + rh[2] ) ;
        if (wd2 > 32767) wd2 = 32767;
        if (wd2 < -32768) wd2 = -32768;
	wd2 = ( ah[2] * wd2 ) >> 15 ;
 

	sph = wd1 + wd2 ;
        if (sph > 32767) sph = 32767;
        if (sph < -32768) sph = -32768;
 

/*************************************** BLOCK 4H, FILTEZ  
***********/
 

	szh = 0 ;
	for (i=6; i>0; i--) {
		wd = (dh[i] + dh[i]) ;
                if (wd > 32767) wd = 32767;
                if (wd < -32768) wd = -32768;
  		szh += (bh[i] * wd) >> 15 ;
                if (szh > 32767) szh = 32767;
                if (szh < -32768) szh = -32768;
 

	}
 

/*********************************BLOCK 4L, PREDIC  
*******************/
 

	sh = sph + szh ;
        if (sh > 32767) sh = 32767;
        if (sh < -32768) sh = -32768;
 

	return (sh) ;
}
/******************************** BLOCK 5H, LIMIT ******************/
block5h (dh, sh)
int dh, sh;
{
int rh ;
 

	rh = dh + sh;
	if (rh >  16383 )  rh =  16383 ;
	if (rh < -16384 )  rh = -16384 ;
 

	return (rh) ;
}


