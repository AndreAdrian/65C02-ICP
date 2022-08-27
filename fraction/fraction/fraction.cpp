/* fraction.cpp
 * IEEE754 compatible fraction prototype for 65C02
 * (C) 2022 Andre Adrian
 *
 * 2022-08-26: first version
 * 2022-08-27: print radix conversion exponent fraction correction constants
 */

 /*
The IEEE754 single (32bit) format fraction f is normalized to 1 <= f < 2 for biased
exponent = 127. It is an unsigned q 1.23 number (one bit left to the binary point and
23 bits right). The leading bit is, for normalized numbers, always 1 and is implicit.

Another fraction interpretation is 0.5 <= f < 1 for biased exponent = 126. and
leading bit is 1. I use this interpretation to convert between decimal (ASCII format)
to IEEE754 32bit for the range -1.0 < f <= -0.1 and  0.1 <= f < 1.0.
For these two ranges my routines are compatible to IEEE754.
 */

#include <cstdio>

enum {
    leadbit = 0x800000,     // implicit/explicit leading bit
    expbias = 127,          // Exponent bias
    round = 128,            // decimal fraction conversion rounding
};

// IEEE754 Single (32bit) Format Floating Point
typedef struct {
    unsigned long f : 23;   // fraction, implicit leading bit
    unsigned long e : 8;    // biased exponent
    unsigned long s : 1;    // sign, 0 is positive
} FPP;      // Floating Point Packed

typedef union {
    float f;
    unsigned long l;
    FPP p;
} UFPP;     // Union Floating Point Packed

// internal Floating Point format for 65C02
typedef struct {
    unsigned long f;    // fraction, explicit leading bit
    signed char e;      // exponent, 2ers complement
    unsigned char s;    // sign, 0 is positive
} FPU;      // Floating Point Unpacked

// convert IEEE754 to internal fp format
FPU fpp2fpu(UFPP u)
{
    FPU f;
    f.s = u.p.s;
    f.e = (signed)u.p.e - expbias;
    f.f = u.p.f | leadbit;
    return f;
}

// convert internal fp format to IEEE754
UFPP fpu2fpp(FPU f)
{
    UFPP u;
    u.p.s = f.s;
    u.p.e = (unsigned)f.e + expbias;
    u.p.f = f.f & (~leadbit);
    return u;
}

// print IEEE754 format 1
void fpp_print(UFPP u)
{
    printf("%11.8f = 0x%08X = %d %3d %6X\n", u.f, u.l, u.p.s, u.p.e, u.p.f);
}

// print IEEE754 format 2
void fpp2_print(UFPP u)
{
    int ex = (signed)u.p.e - expbias;
    double fr = (double)(u.p.f | leadbit) / 8388608.0;
    int ex2 = (signed)u.p.e - expbias + 1;
    double fr2 = (double)(u.p.f | leadbit) / 16777216.0;
    printf("%13.6e = %d %3d %6X = %9.7f %4d = %9.7f %4d\n", u.f, u.p.s, u.p.e, u.p.f, 
        fr, ex, fr2, ex2);
}

// print internal fp format
void fpu_print(FPU f)
{
    printf("%d %4d %6X\n", f.s, f.e, f.f);
}

// Constants for decimal to fraction conversion and vice versa range +/-[0.1 .. 1)
// Constants are unsigned q 0.32, that is q(0.1) = 0.1 * 2^32
unsigned long c[] = { 429496730, 42949673, 4294967, 429497, 42950, 4295, 429, 43 };

// converts fp (-1 .. -0.1] and [0.1 .. 1) from ASCII string
FPU dcm2fpu(char* buf)
{
    FPU f;
    f.s = 0;
    f.e = -1;
    f.f = round;

    if ('-' == *buf) {
        f.s = 1;
        ++buf;
    }
    ++buf;  // read over 0
    ++buf;  // read over .
    for (int i = 0; i < sizeof c / sizeof c[0]; ++i) {
        if ('\0' == *buf) break;
        unsigned long digit = *buf++ - '0';
        f.f += digit * c[i];
    }
    // normalize
    while (0 == (f.f & 0x80000000)) {
        f.f <<= 1;
        --f.e;
    }
    f.f >>= 8;              // remove "precision" bits
    return f;
}

// converts fp (-1 .. -0.1] and [0.1 .. 1) to ASCII string
void fpu2dcm(char* buf, FPU f)
{
    f.f <<= 8;              // add "precision" bits
    f.f += round;
    f.f >>= (-1 - f.e);     // de-normalize
    if (1 == f.s) *buf++ = '-';
    *buf++ = '0';
    *buf++ = '.';
    for (int i = 0; i < sizeof c / sizeof c[0]; ++i) {
        char digit = '0';
        while (f.f >= c[i]) {
            ++digit;
            f.f -= c[i];
        }
        *buf++ = digit;
    }
    *buf = '\0';
}

// test cases
char fractions[][12] = {
    "0.1", "0.11", "0.101", "0.1001", "0.10001", "0.100001", "0.1000001", "0.10000001",
    "0.5", "0.55", "0.505", "0.5005", "0.50005", "0.500005", "0.5000005", "0.50000005",
    "0.9", "0.99", "0.909", "0.9009", "0.90009", "0.900009", "0.9000009", "0.90000009",
    "0.99999995",
    "-0.10000001", "-0.50000005", "-0.90000009", "-0.99999995"
};

int main()
{
    UFPP u;

    printf(" IEEE754    = as bits    = s exp fraction\n");
    u.f = 0.0f; fpp_print(u);
    u.f = -0.0f; fpp_print(u);
    u.f = 0.0625f; fpp_print(u);
    u.f = 0.125f; fpp_print(u);
    u.f = 0.25f; fpp_print(u);
    u.f = 0.5f; fpp_print(u);
    u.f = 0.75f; fpp_print(u);
    u.f = 0.99999995f; fpp_print(u);
    u.f = 1.0f; fpp_print(u);
    u.f = -1.0f; fpp_print(u);
    u.f = 1.5f; fpp_print(u);
    u.f = 1.9999999f; fpp_print(u);

    printf("\nfrom string = s exp fraction = to string  =  IEEE754\n");
    for (int i = 0; i < sizeof fractions / sizeof fractions[0]; ++i) {
        FPU f = dcm2fpu(fractions[i]);
        char buf[16];
        fpu2dcm(buf, f);
        UFPP u = fpu2fpp(f);
        printf("%-11s = %d %4d %6X = %11s = %11.8f\n",
            fractions[i], f.s, f.e, f.f, buf, u.f);
    }

    printf("\n    IEEE754   = s exp fract. = fraction  2^   = fraction  2^\n");
    u.f = 1.0e-12f; fpp2_print(u);
    u.f = 1.0e-9f; fpp2_print(u);
    u.f = 1.0e-6f; fpp2_print(u);
    u.f = 1.0e-3f; fpp2_print(u);
    u.f = 1.0f; fpp2_print(u);
    u.f = 1.0e3f; fpp2_print(u);
    u.f = 1.0e6f; fpp2_print(u);
    u.f = 1.0e9f; fpp2_print(u);
    u.f = 1.0e12f; fpp2_print(u);

    return 0;
}
