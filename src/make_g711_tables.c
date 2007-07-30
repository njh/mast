/*
	A quick bit of code to generate static G.711 decode tables
	(to linear float32 samples) 
*/

#include <samplerate.h>
#include <stdio.h>

static int alaw_to_s16 (char a_val)
{
	int t;
	int seg;
	
	a_val ^= 0x55;
	t = a_val & 0x7f;
	if (t < 16) {
		t = (t << 4) + 8;
	} else {
		seg = (t >> 4) & 0x07;
		t = ((t & 0x0f) << 4) + 0x108;
		t <<= seg - 1;
	}
	return ((a_val & 0x80) ? t : -t);
}

static void print_alaw_decode()
{
	int i;

	printf("static float pcma_decode[128] =\n");
	printf("{");

	for(i = 0; i < 128; i++)
	{
		short s16 = alaw_to_s16(i);
		float float32 = 0;

		if (i) printf(", ");
		if (i%8==0) printf("\n\t");

		
		src_short_to_float_array( &s16, &float32, 1 );
		printf("%f", float32);
		
	}

	printf("\n};\n\n");
	
}

static void print_ulaw_decode()
{
	static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
	int sign, exponent, mantissa, sample;
	short s16 = 0;
	float float32 = 0.0f;
	unsigned char ulawbyte;
	int i;
	
	
	printf("static float pcmu_decode[128] =\n");
	printf("{");

	for(i = 0; i < 128; i++)
	{
		if (i) printf(", ");
		if (i%8==0) printf("\n\t");
		
		ulawbyte = (unsigned char)i;
		ulawbyte = ~ulawbyte;
		sign = (ulawbyte & 0x80);
		exponent = (ulawbyte >> 4) & 0x07;
		mantissa = ulawbyte & 0x0F;
		sample = exp_lut[exponent] + (mantissa << (exponent + 3));
		if(sign != 0) sample = -sample;
		s16=sample;
		src_short_to_float_array( &s16, &float32, 1 );
		printf("%f", float32);
	}

	printf("\n};\n\n");
}

int main(int argc, char** argv) 
{
	print_ulaw_decode();
	print_alaw_decode();
	
	return 0;
}

