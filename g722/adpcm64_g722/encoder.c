#include <stdio.h>
int prlev = 0 ;

main ( )
{

	int band = 0 ;
	int xlow, ilow, dlowt;
	int slow = 0 ;
	int detlow  = 32 ;
	int block1l(), block2l(), block3l(), block4l() ;
	int xhigh, ihigh, dhigh;
	int shigh = 0 ;
	int dethigh = 8 ;
	int block1h(), block2h(), block3h(), block4h() ;
        char ch = ' ';
        FILE *infile;
        FILE *outfile;

        printf("%16c bands: 0: Both, \n %22c 1: Low only, \n %22c", ch,ch,ch);
        printf(" 2: High only. \n");
        printf("Type bands code \n");
	scanf ("%d", &band ) ;

        infile = fopen("testin.dat", "r");
        outfile = fopen("adpcm.dat", "w+");

next:	if (band == 2) goto high ;
low:	fscanf (infile, "%d", &xlow) ;           /*input low band pcm*/
	if (xlow == 32767) goto finish ;
	ilow = block1l (xlow, slow, detlow) ;    /*low band encoder  */
	dlowt = block2l (ilow, detlow) ;
	detlow = block3l (ilow) ;
	slow = block4l (dlowt) ;
	fprintf (outfile, "%o ", ilow) ;         /*output low band code*/

if (prlev > 0)
	printf ("MAIN LOW ilow=%5d,  dlowt=%5d, detlow=%5d, slow=%5d\n",
		ilow, dlowt, detlow, slow) ;


	if (band == 1) goto newln ;
high:	fscanf (infile, "%d", &xhigh) ;             /*input high band pcm*/
	if (xhigh == 32767) goto finish ;
	ihigh = block1h (xhigh, shigh, dethigh) ;   /*high band*/
	dhigh = block2h (ihigh, dethigh) ;          /*encoder  */
	dethigh = block3h (ihigh) ;
	shigh = block4h (dhigh) ;
	fprintf (outfile, "%o ", ihigh) ;           /*output high band adpcm*/


newln:	fprintf (outfile, "\n") ;
	goto next;

finish:	fprintf (outfile, "77777 77777 \n") ;
}

/************************** BLOCK 1L ********************************/

block1l (xl, sl, detl)
int xl, sl, detl ;
{
int il ;

	int i, el, sil, mil, wd, wd1, hdu ;

	static int q6[32] = {0, 35, 72, 110, 150, 190, 233, 276, 323,
		370, 422, 473, 530, 587, 650, 714, 786,
		858, 940, 1023, 1121, 1219, 1339, 1458,
		1612, 1765, 1980, 2195, 2557, 2919, 0, 0} ;

	static int iln[32] = {0, 63, 62, 31, 30, 29, 28, 27, 26, 25,
		24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,
		13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 0 } ;

	static int ilp[32] = {0, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52,
		51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
		40, 39, 38, 37, 36, 35, 34, 33, 32, 0 } ;

/*************************************** BLOCK 1L, SUBTRA ************/

	el = xl - sl ;
        if ( el > 32767 ) el = 32767;
        if ( el < -32768 ) el = -32768;

/*************************************** BLOCK 1L, QUANTL ************/

	sil = el >> 15 ;
	if (sil == 0 )  wd = el ;
	else wd = 32767 - el & 32767 ;
	
	mil = 1 ;

	for (i = 1; i < 30; i++) {
		hdu = (q6[i] << 3) * detl;
		wd1 = (hdu >> 15) ;
		if (wd >= wd1)  mil = (i + 1) ;
		else break ;
	}

	if (sil == -1 ) il = iln[mil] ;
	else il = ilp[mil] ;

	return (il) ;
}

/*************************** BLOCK 2L *******************************/

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
/************************************** BLOCK 2L, INVQAL ************/

	ril = il >> 2 ;
	wd2 = qm4[ril] ;
	dlt = (detl * wd2) >> 15 ;

	return (dlt) ;
}

/***************************** BLOCK 3L *******************************/

block3l (il)
int il ;
{
int detl ;
	int ril, il4, wd, wd1, wd2, wd3, nbpl, depl ;
	static int nbl = 0 ;
	static int wl[8] = {-60, -30, 58, 172, 334, 538, 1198, 3042 } ;
	static int rl42[16] = {0, 7, 6, 5, 4, 3, 2, 1, 7, 6, 5, 4, 3, 2,
		1, 0 } ;
	static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282, 2332,
		2383, 2435, 2489, 2543, 2599, 2656, 2714,
		2774, 2834, 2896, 2960, 3025, 3091, 3158,
		3228, 3298, 3371, 3444, 3520, 3597, 3676,
		3756, 3838, 3922, 4008 } ;
/************************************** BLOCK 3L, LOGSCL *************/

	ril = il >> 2 ;
	il4 = rl42[ril] ;
	wd = (nbl * 32512) >> 15 ;
	nbpl = wd + wl[il4] ;

	if (nbpl <     0) nbpl = 0 ;
	if (nbpl > 18432) nbpl = 18432 ;

/************************************** BLOCK 3L, SCALEL *************/
	wd1 =  (nbpl >> 6) & 31 ;
	wd2 = nbpl >> 11 ;
	if ((8 - wd2) < 0)    wd3 = ilb[wd1] << (wd2 - 8) ;
	else   wd3 = ilb[wd1] >> (8 - wd2) ;
	depl = wd3 << 2 ;
/************************************** BLOCK 3L, DELAYA *************/
	nbl = nbpl ;
/************************************** BLOCK 3L, DELAYL *************/
	detl = depl ;

if (prlev > 1) {
	printf ("BLOCK3L il=%4d, ril=%4d, il4=%4d, nbl=%4d, wd=%4d, nbpl=%4d\n",
		 il, ril, il4, nbl, wd, nbpl) ;
	printf ("wd1=%4d, wd2=%4d, wd3=%4d, depl=%4d, detl=%4d\n",
		wd1, wd2, wd3, depl, detl) ;
	}

	return (detl) ;
}

/**************************** BLOCK 4L *******************************/

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
/*************************************** BLOCK 4L, RECONS ***********/

	rlt[0] = sl + dlt[0] ;
        if ( rlt[0] > 32767 ) rlt[0] = 32767;
        if ( rlt[0] < -32768 ) rlt[0] = -32768;

/*************************************** BLOCK 4L, PARREC ***********/

	plt[0] = dlt[0] + szl ;
        if ( plt[0] > 32767 ) plt[0] = 32767;
        if ( plt[0] < -32768 ) plt[0] = -32768;

/*****************************BLOCK 4L, UPPOL2*************************/

	sg[0] = plt[0] >> 15 ;
	sg[1] = plt[1] >> 15 ;
	sg[2] = plt[2] >> 15 ;

	wd1 = al[1] + al[1] ;
	wd1 = wd1 + wd1 ;
        if ( wd1 > 32767 ) wd1 = 32767;
        if ( wd1 < -32768 ) wd1 = -32768;

	if ( sg[0] == sg[1] )  wd2 = - wd1 ;
	else  wd2 = wd1 ;
        if ( wd2 > 32767 ) wd2 = 32767;

	wd2 = wd2 >> 7 ;

	if ( sg[0] == sg[2] )  wd3 = 128 ;
		   else  wd3 = - 128 ;

	wd4 = wd2 + wd3 ;
	wd5 = (al[2] * 32512) >> 15 ;

	apl[2] = wd4 + wd5 ;
	if ( apl[2] >  12288 )  apl[2] =  12288 ;
	if ( apl[2] < -12288 )  apl[2] = -12288 ;
/************************************* BLOCK 4L, UPPOL1 ***************/

	sg[0] = plt[0] >> 15 ;
	sg[1] = plt[1] >> 15 ;

	if ( sg[0] == sg[1] )  wd1 = 192 ;
	if ( sg[0] != sg[1] )  wd1 = - 192 ;

	wd2 = (al[1] * 32640) >> 15 ;

	apl[1] = wd1 + wd2 ;
        if ( apl[1] > 32767 ) apl[1] = 32767;
        if ( apl[1] < -32768 ) apl[1] = -32768;

	wd3 = (15360 - apl[2]) ;
        if ( wd3 > 32767 ) wd3 = 32767;
        if ( wd3 < -32768 ) wd3 = -32768;
	if ( apl[1] >  wd3)  apl[1] =  wd3 ;
	if ( apl[1] < -wd3)  apl[1] = -wd3 ;

/*************************************** BLOCK 4L, UPZERO ************/

	if ( dlt[0] == 0 )  wd1 = 0 ;
	if ( dlt[0] != 0 )  wd1 = 128 ;

	sg[0] = dlt[0] >> 15 ;

	for ( i = 1; i < 7; i++ ) {
		sg[i] = dlt[i] >> 15 ;
		if ( sg[i] == sg[0] )  wd2 = wd1 ;
		if ( sg[i] != sg[0] )  wd2 = - wd1 ;
		wd3 = (bl[i] * 32640) >> 15 ;
		bpl[i] = wd2 + wd3 ;
                if ( bpl[i] > 32767 ) bpl[i] = 32767;
                if ( bpl[i] < -32768 ) bpl[i] = -32768;
	}
/********************************* BLOCK 4L, DELAYA ******************/
	for ( i = 6; i > 0; i-- ) {
		dlt[i] = dlt[i-1] ;
		bl[i]  = bpl[i] ;
	}

	for ( i = 2; i > 0; i-- ) {
		rlt[i] = rlt[i-1] ;
		plt[i] = plt[i-1] ;
		al[i] = apl[i] ;
	}
/********************************* BLOCK 4L, FILTEP ******************/

	wd1 = ( rlt[1] + rlt[1] ) ;
        if ( wd1 > 32767 ) wd1 = 32767;
        if ( wd1 < -32768 ) wd1 = -32768;
	wd1 = ( al[1] * wd1 ) >> 15 ;

	wd2 = ( rlt[2] + rlt[2] ) ;
        if ( wd2 > 32767 ) wd2 = 32767;
        if ( wd2 < -32768 ) wd2 = -32768;
	wd2 = ( al[2] * wd2 ) >> 15 ;

	spl = wd1 + wd2 ;
        if ( spl > 32767 ) spl = 32767;
        if ( spl < -32768 ) spl = -32768;

/*************************************** BLOCK 4L, FILTEZ ***********/

	szl = 0 ;
	for (i=6; i>0; i--) {
		wd = (dlt[i] + dlt[i]) ;
                if ( wd > 32767 ) wd = 32767;
                if ( wd < -32768 ) wd = -32768;
		szl += (bl[i] * wd) >> 15 ;
                if ( szl > 32767 ) szl = 32767;
                if ( szl < -32768 ) szl = -32768;
	}

/*********************************BLOCK 4L, PREDIC *******************/

	sl = spl + szl ;
        if ( sl > 32767 ) sl = 32767;
        if ( sl < -32768 ) sl = -32768;

	return (sl) ;
}

/************************** BLOCK 1H ********************************/

block1h (xh, sh, deth)
int xh, sh, deth ;
{
int ih ;

	int eh, sih, mih, wd, wd1, hdu ;

	static int ihn[3] = { 0, 1, 0 } ;
	static int ihp[3] = { 0, 3, 2 } ;

/*************************************** BLOCK 1H, SUBTRA ************/

	eh = xh - sh ;
        if ( eh > 32767 ) eh = 32767;
        if ( eh < -32768 ) eh = -32768;

/*************************************** BLOCK 1H, QUANTH ************/

	sih = eh >> 15 ;
	if (sih == 0 )  wd = eh ;
	else wd = 32767 - eh & 32767 ;
	
	hdu = (564 << 3) * deth;
	wd1 = (hdu >> 15) ;
	if (wd >= wd1)  mih = 2 ;
	else mih = 1 ;

	if (sih == -1 ) ih = ihn[mih] ;
	else ih = ihp[mih] ;

	return (ih) ;
}

/*************************** BLOCK 2H *******************************/

block2h (ih, deth)
int ih, deth ;
{
int dh ;
	int wd2 ;
	static int qm2[4] =
		{-7408,	-1616,	7408,	1616} ;
/************************************** BLOCK 2H, INVQAH ************/

	wd2 = qm2[ih] ;
	dh = (deth * wd2) >> 15 ;

	return (dh) ;
}

/***************************** BLOCK 3H *******************************/

block3h (ih)
int ih ;
{
int deth ;
	int ih2, wd, wd1, wd2, wd3, nbph, deph ;
	static int nbh = 0 ;
	static int wh[3] = {0, -214, 798} ;
	static int rh2[4] = {2, 1, 2, 1} ;
	static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282, 2332,
		2383, 2435, 2489, 2543, 2599, 2656, 2714,
		2774, 2834, 2896, 2960, 3025, 3091, 3158,
		3228, 3298, 3371, 3444, 3520, 3597, 3676,
		3756, 3838, 3922, 4008 } ;
/************************************** BLOCK 3H, LOGSCH *************/

	ih2 = rh2[ih] ;
	wd = (nbh * 32512) >> 15 ;
	nbph = wd + wh[ih2] ;

	if (nbph <     0) nbph = 0 ;
	if (nbph > 22528) nbph = 22528 ;

/************************************** BLOCK 3H, SCALEH *************/
	wd1 =  (nbph >> 6) & 31 ;
	wd2 = nbph >> 11 ;
	if ((10-wd2) < 0) wd3 = ilb[wd1] << (wd2-10) ;
	else wd3 = ilb[wd1] >> (10-wd2) ;
	deph = wd3 << 2 ;
/************************************** BLOCK 3L, DELAYA *************/
	nbh = nbph ;
/************************************** BLOCK 3L, DELAYH *************/
	deth = deph ;

	return (deth) ;
}

/**************************** BLOCK 4H *******************************/

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
/*************************************** BLOCK 4H, RECONS ***********/

	rh[0] = sh + dh[0] ;
        if ( rh[0] > 32767 ) rh[0] = 32767;
        if ( rh[0] < -32768 ) rh[0] = -32768;

/*************************************** BLOCK 4H, PARREC ***********/

	ph[0] = dh[0] + szh ;
        if ( ph[0] > 32767 ) ph[0] = 32767;
        if ( ph[0] < -32768 ) ph[0] = -32768;

/*****************************BLOCK 4H, UPPOL2*************************/

	sg[0] = ph[0] >> 15 ;
	sg[1] = ph[1] >> 15 ;
	sg[2] = ph[2] >> 15 ;

	wd1 = ah[1] + ah[1] ;
	wd1 = wd1 + wd1 ;
        if ( wd1 > 32767 ) wd1 = 32767;
        if ( wd1 < -32768 ) wd1 = -32768;

	if ( sg[0] == sg[1] )  wd2 = - wd1 ;
	else  wd2 = wd1 ;
        if ( wd2 > 32767 ) wd2 = 32767;

	wd2 = wd2 >> 7 ;

	if ( sg[0] == sg[2] )  wd3 = 128 ;
		   else  wd3 = - 128 ;

	wd4 = wd2 + wd3 ;
	wd5 = (ah[2] * 32512) >> 15 ;

	aph[2] = wd4 + wd5 ;
	if ( aph[2] >  12288 )  aph[2] =  12288 ;
	if ( aph[2] < -12288 )  aph[2] = -12288 ;
/************************************* BLOCK 4H, UPPOL1 ***************/

	sg[0] = ph[0] >> 15 ;
	sg[1] = ph[1] >> 15 ;

	if ( sg[0] == sg[1] )  wd1 = 192 ;
	if ( sg[0] != sg[1] )  wd1 = - 192 ;

	wd2 = (ah[1] * 32640) >> 15 ;

 	aph[1] = wd1 + wd2 ;
        if ( aph[1] > 32767 ) aph[1] = 32767;
        if ( aph[1] < -32768 ) aph[1] = -32768;

	wd3 = (15360 - aph[2]) ;
        if ( wd3 > 32767 ) wd3 = 32767;
        if ( wd3 < -32768 ) wd3 = -32768;
	if ( aph[1] >  wd3)  aph[1] =  wd3 ;
	if ( aph[1] < -wd3)  aph[1] = -wd3 ;

/*************************************** BLOCK 4H, UPZERO ************/

	if ( dh[0] == 0 )  wd1 = 0 ;
	if ( dh[0] != 0 )  wd1 = 128 ;

	sg[0] = dh[0] >> 15 ;

	for ( i = 1; i < 7; i++ ) {
		sg[i] = dh[i] >> 15 ;
		if ( sg[i] == sg[0] )  wd2 = wd1 ;
		if ( sg[i] != sg[0] )  wd2 = - wd1 ;
		wd3 = (bh[i] * 32640) >> 15 ;
		bph[i] = wd2 + wd3 ;
                if ( bph[i] > 32767 ) bph[i]= 32767;
                if ( bph[i] < -32768 ) bph[i] = -32768;

	}
/********************************* BLOCK 4H, DELAYA ******************/
	for ( i = 6; i > 0; i-- ) {
		dh[i] = dh[i-1] ;
		bh[i]  = bph[i] ;
	}

	for ( i = 2; i > 0; i-- ) {
		rh[i] = rh[i-1] ;
		ph[i] = ph[i-1] ;
		ah[i] = aph[i] ;
	}
/********************************* BLOCK 4H, FILTEP ******************/

	wd1 = ( rh[1] + rh[1] ) ;
        if ( wd1 > 32767 ) wd1 = 32767;
        if ( wd1 < -32768 ) wd1 = -32768;
	wd1 = ( ah[1] * wd1 ) >> 15 ;

	wd2 = ( rh[2] + rh[2] ) ;
        if ( wd2 > 32767 ) wd2 = 32767;
        if ( wd2 < -32768 ) wd2 = -32768;
	wd2 = ( ah[2] * wd2 ) >> 15 ;

	sph = wd1 + wd2 ;
        if ( sph > 32767 ) sph = 32767;
        if ( sph < -32768 ) sph = -32768;

/*************************************** BLOCK 4H, FILTEZ ***********/

	szh = 0 ;
	for (i=6; i>0; i--) {
		wd = (dh[i] + dh[i]) ;
                if ( wd > 32767 ) wd = 32767;
                if ( wd < -32768 ) wd = -32768;
		szh += (bh[i] * wd) >> 15 ;
                if ( szh > 32767 ) szh = 32767;
                if ( szh < -32768 ) szh = -32768;
	}

/*********************************BLOCK 4L, PREDIC *******************/

	sh = sph + szh ;
        if ( sh > 32767 ) sh = 32767;
        if ( sh < -32768 ) sh = -32768;

	return (sh) ;
}

