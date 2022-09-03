/* fpconsts.cpp
 * Generate constant arrays c1 to c5 for fraction.cpp
 * 
 * 2022-09-02: use "pseudo radix 16" for c4, c5
 * 2022-09-03: emit hexadecimal numbers
 */

#include <cstdio>
#include <math.h>

// My FPU format has unsigned Q1.23 fraction with explicit leading bit.
// This fraction gets 3 more leading integer bits and 5 more trailing 
// rounding bits for radix 2 to radix 10 conversion and vice versa.
// The conversion "accumulator" has unsigned Q4.28 fraction.
// It can store values from 0.0 to 15.99999999.

enum {
	Ibits = 4,		// integer bits 0 .. 15
	Emin10 = -31,	// Radix 10 minimal exponent
	Emin2 = -108,	// Radix 2 minimal exponent

	Digits = 9,
	Fbits = 32 - Ibits,		// Fraction bits 0.0 .. 0.99999999
	Fvalue = 1 << Fbits,	// Fraction value
	Emax10 = 1 - Emin10,	// Radix 10 maximum exponent
	Emax2 = 4 - Emin2,		// Radix 2 maximum exponent
};

const double Log2_10 = 3.321928094887;	// log base 2 of 10
const double Firstdigit = 10.0;			// radix 10 value of first digit
const double Rvalue = 0.5;				// rounding value

// Q constants 1.0, 0.1, ... 0.000001 for fraction conversion
void c1()
{
	double fc1 = Firstdigit;	// float c1
	for (int i = 0; i < Digits; ++i, fc1 = fc1 / 10.0) {
		double sc1 = fc1 * Fvalue;	// scaled c1
		unsigned long c1 = (unsigned long)(sc1 + Rvalue);
		// printf("%10.7f %12.1f %12d\n", fc1, sc1, c1);
//		printf("%d, ", c1);
		printf("0x%X, ", c1);
	}
	printf("\n");
}

double c2min = 10.0;
double c2max = 0.0;

// Q fraction constants to convert exponent 10^x to exponent 2^y
// y = x * log2(10) = x * 3,321928094887...
// c2 = Fvalue * 10^x / 2^y
void c2c3(char mode)
{
	//	printf("\n10^x   2^y   c3  10^x/2^y     c2\n");
	int cnt = 0;
	for (int ix = Emin10; ix <= Emax10; ++ix) {
		double fc2n = pow(10.0, ix);	// nominator
		double fy = (double)ix * Log2_10;
		int c3 = (int)(fy < 0.0)? fy- Rvalue : fy+ Rvalue;
		double fc2d = pow(2.0, c3);		// denominator
		double fc2 = fc2n / fc2d;
		double sc2 = fc2 * Fvalue;		// scaled c1
		unsigned long c2 = (unsigned long)(sc2 + Rvalue);
//		printf("%5d %6.1f %4d %8.6f %10d\n", ix, fy, c3, fc2, c2);
		if (fc2 < c2min) c2min = fc2;
		if (fc2 > c2max) c2max = fc2;
		switch (mode) {
		case 2:
//			printf("%d, ", c2);
			printf("0x%X, ", c2);
			++cnt;
			if (6 == cnt) {
				cnt = 0;
				printf("\n");
			}
			break;
		case 3:
			printf("%d, ", c3);
			++cnt;
			if (12 == cnt) {
				cnt = 0;
				printf("\n");
			}
			break;
		}
	}
	printf("\n");
}

double c4min = 10.0;
double c4max = 0.0;

// Q fraction constants to convert exponent 10^x to exponent 2^y
// y = x / log2(10) = x / 3,321928094887...
// c4 = Fvalue * 2^x / 10^y
void c4c5(char mode)
{
//	printf("\n  2^x   10^y   c5 2^x/10^y     c4\n");
	int cnt = 0;
	// step size ix += 4 is pseudo radix 16
	for (int ix = Emin2; ix <= Emax2; ix += 4) {
		double fc4n = pow(2.0, ix);	// nominator
		double fy = (double)ix / Log2_10;
		int c5 = (int)(fy < 0.0) ? fy - Rvalue : fy + Rvalue;
		double fc4d = pow(10.0, c5);		// denominator
		double fc4 = fc4n / fc4d;
		double sc4 = fc4 * Fvalue;	// scaled c1
		unsigned long c4 = (unsigned long)(sc4 + Rvalue);
//		printf("%5d %6.1f %4d %8.6f %10d\n", ix, fy, c5, fc4, c4);
		if (fc4 < c4min) c4min = fc4;
		if (fc4 > c4max) c4max = fc4;
		switch (mode) {
		case 4:
//			printf("%d, ", c4);
			printf("0x%X, ", c4);
			++cnt;
			if (6 == cnt) {
				cnt = 0;
				printf("\n");
			}
			break;
		case 5:
			printf("%d, ", c5);
			++cnt;
			if (12 == cnt) {
				cnt = 0;
				printf("\n");
			}
			break;
		}
	}
	printf("\n");
}

int main()
{
	printf("enum {\n");
	printf("Ibits = %d,\n", Ibits);
	printf("Emin10 = %d,\n", Emin10);
	printf("Emin2 = %d,\n", Emin2);
	printf("};\n\n");

	printf("unsigned long c1[] = {\n");
	c1();
	printf("};\n\n");

	printf("unsigned long c2[] = {\n");
	c2c3(2);
	printf("};\n\n");

	printf("signed char c3[] = {\n");
	c2c3(3);
	printf("};\n\n");

	printf("unsigned long c4[] = {\n");
	c4c5(4);
	printf("};\n\n");

	printf("signed char c5[] = {\n");
	c4c5(5);
	printf("};\n\n");

	printf("\n\nc2 min/max = %f %f\n", c2min, c2max);
	printf("c4 min/max = %f %f\n", c4min, c4max);
}

