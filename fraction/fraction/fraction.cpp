/* fraction.cpp
 * IEEE754 compatible floating point for 65C02
 * See https://github.com/AndreAdrian/65C02-ICP/blob/main/floating-point.md
 * (C) 2022 Andre Adrian
 *
 * 2022-08-26: first version
 * 2022-08-30: dcm2fpu() for 10^-5 to 10^5, fpu2dcm() for 2^-7 to 2^7
 */

#include <cstdio>

enum {
    leadbit = 0x800000,     // implicit/explicit leading bit
    expbias = 127,          // Exponent bias
    roundbit = 5,
    round = (1<<roundbit)-1,    // fraction radix conversion rounding
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

// unsigned Q4.28 multiplication
unsigned long mul28(unsigned long a, unsigned long b)
{
    return ((unsigned long long)a * b) >> 28;
}

// converts ASCII to int [-32768 .. 32767]
short dcm2int(char* buf)
{
    unsigned char s = 0;
    short v = 0;
    if ('-' == *buf) {
        s = 1;
        ++buf;
    }
    while (*buf >= '0') {
        v *= 10;
        v += (*buf - '0');
        ++buf;
    }
    if (s) v = 0 - v;
    return v;
}

// Q4.28 constants 1.0, 0.1, ... 0.000001 for fraction conversion
unsigned long c1[] = { 268435456, 26843546, 2684355, 268435, 26844, 2684, 268 };

// Q4.28 fraction constants to convert exponent 10^x to exponent 2^y
unsigned long c2[] = { 351843721, 439804651, 274877907, 343597384, 429496730,
    268435456, 335544320, 419430400, 524288000, 327680000,  409600000 };

// integer exponent constants, radix 10 to radix 2, e.g. 10^-5 converts to 2^-17
signed char c3[] = { -17, -14, -10, -7, -4, 0, 3, 6, 9, 13, 16 };

int e10;    // use globals for easy print function internals
int endx;

// convert ASCII string -?[1-9].[0-9]*(e-?[0-9]+)? to fp
// RE: ?= 0 to 1 repetition, *=0 to N repetition, +=1 to N repetition
FPU dcm2fpu(char* buf)
{
    FPU f;
    f.s = 0;
    f.e = 0;
    f.f = 0;

    // do fraction part -?[1-9].[0-9]*
    if ('-' == *buf) {
        f.s = 1;
        ++buf;
    }
    unsigned long digit = *buf++ - '0';
    f.f = digit * c1[0];
    ++buf;  // read over .
    for (int i = 1; i < sizeof c1 / sizeof c1[0]; ++i) {
        if (*buf < '0' || *buf > '9') break;
        digit = *buf++ - '0';
        f.f += digit * c1[i];
    }
    // do exponent part e-?[0-9]+
    if ('e' == *buf++) {
        e10 = dcm2int(buf);
        if (0 == f.f && 0 == e10) {     // zero 0.e0
            f.e = -expbias;
            return f;
        }
        endx = e10 + 5;
        f.f += round;
        f.f = mul28(f.f, c2[endx]);
        f.e = c3[endx];
    }

    // normalize
    while (f.f & 0xE0000000) { // fraction to large
        f.f >>= 1;
        ++f.e;
    }
    while (0 == (f.f & 0xF0000000)) { // fraction to small
        f.f <<= 1;
        --f.e;
    }
    f.f >>= roundbit;   // remove "rounding" bits
    return f;
}

// converts int [-99..99] to ASCII string
void exp2dcm(char* buf, signed char e)
{
    if (e < 0) {
        *buf++ = '-';
        e = 0 - e;
    }
    signed char tens = e / 10;
    if (tens != 0) {
        *buf++ = e / 10 + '0';
    }
    *buf++ = e % 10 + '0';
    *buf = '\0';
}

// Q4.28 fraction constants to convert exponent 2^x to exponent 10^y
unsigned long c4[] = {
    209715200, 419430400, 838860800, 167772160, 335544320, 671088640, 1342177280,
    268435456, 536870912, 1073741824, 214748365, 429496730, 268435456, 171798692, 343597384 };

// integer exponent constants, radix 2 to radix 10, e.g. 2^-7 converts to 10^-2
signed char c5[] = { -2, -2, -2, -1, -1, -1, -1, 0, 0, 0, 1, 1, 1, 2, 2 };

// convert fraction to ASCII string
void fpu2dcm(char* buf, FPU f)
{
    if (1 == f.s) *buf++ = '-';
    if (-127 == f.e && 0 == f.f) {      // zero
        *buf++ = '0';
        *buf++ = '.';
        *buf++ = 'e';
        *buf++ = '0';
        *buf = '\0';
        return;
    }
    f.f <<= roundbit;       // add "rounding" bits
    f.f += round;

    endx = f.e + 7;
    f.f = mul28(f.f, c4[endx]);

    // do fraction part
    char digit = '0';
    while (f.f >= c1[0]) {
        ++digit;
        f.f -= c1[0];
    }
    *buf++ = digit;
    *buf++ = '.';
    for (int i = 1; i < sizeof c1 / sizeof c1[0]; ++i) {
        char digit = '0';
        while (f.f >= c1[i]) {
            ++digit;
            f.f -= c1[i];
        }
        *buf++ = digit;
    }
    // do exponent part
    *buf++ = 'e';
    exp2dcm(buf, c5[endx]);
}

// test cases
int main()
{
    float floattst1[] = {
        0.0f, -0.0f, 0.125f, 0.25f, 0.5f, 1.0f, -1.0f,
        1.5f, 1.9999999f, 2.0f, 4.0f, 8.0f, 9.999999f
    };
    printf("  IEEE754  =  hexfloat  = s exp implicit = fraction  2^\n");
    for (int i = 0; i < sizeof floattst1 / sizeof floattst1[0]; ++i) {
        UFPP u;
        u.f = floattst1[i];
        int ex = (signed)u.p.e - expbias;
        double fr = (double)(u.p.f | leadbit) / 8388608.0;
        printf("%10.7f = 0x%08X = %d %3d 0x%06X = %9.7f %4d\n", 
            u.f, u.l, u.p.s, u.p.e, u.p.f, fr, ex);
    }

    char dcmtst1[][12] = {
        "1.", "1.1", "1.01", "1.001", "1.0001", "1.00001", "1.000001",
        "5.", "5.5", "5.05", "5.005", "5.0005", "5.00005", "5.000005",
        "9.", "9.9", "9.09", "9.009", "9.0009", "9.00009", "9.000009",
        "9.999999",
        "-1.000001", "-5.000005", "-9.000009", "-9.999999",
        "0.e0", "-0.e0"
    };
    printf("\nfrom ASCII= s explicit fraction 2^   =   to ASCII  =  IEEE754\n");
    for (int i = 0; i < sizeof dcmtst1 / sizeof dcmtst1[0]; ++i) {
        FPU f = dcm2fpu(dcmtst1[i]);
        double fr = (double)f.f / 8388608.0;
        char buf[16];
        fpu2dcm(buf, f);
        UFPP u = fpu2fpp(f);
        printf("%-9s = %d 0x%06X %1.6f %4d = %11s = %9.6f\n",
            dcmtst1[i], f.s, f.f, fr, f.e, buf, u.f);
    }

    float floattst2[] = {
        1.e-5f, 1.e-4f, 1.e-3f, 1.e-2f, 1.e-1f, 
        1.e0f, 1.e1f, 1.e2f, 1.e3f, 1.e4f, 1.e5f
    };
    printf("\n   IEEE754    = s exp implicit = fraction  2^\n");
    for (int i = 0; i < sizeof floattst2 / sizeof floattst2[0]; ++i) {
        UFPP u;
        u.f = floattst2[i];
        int ex = (signed)u.p.e - expbias;
        double fr = (double)(u.p.f | leadbit) / 8388608.0;
        printf("%13.6e = %d %3d 0x%06X = %8.6f %4d\n",
            u.f, u.p.s, u.p.e, u.p.f, fr, ex);
    }

    char dcmtst2[][14] = {
         "1.e-5",  "1.e-4", "1.e-3", "1.e-2", "1.e-1",
         "1.e0", "8.e0", "0.8e1", "1.e1", "1.e2", "1.e3", "1.e4", "1.e5",
         "5.e-3", "5.5e-2", "5.05e-1", "5.005e0", "5.0005e1", "5.00005e2", "5.000005e3"
    };
    printf("\n from ASCII = 10^ ndx= explicit fraction 2^   =   IEEE754\n");
    for (int i = 0; i < sizeof dcmtst2 / sizeof dcmtst2[0]; ++i) {
        FPU f = dcm2fpu(dcmtst2[i]);
        UFPP u = fpu2fpp(f);
        double fr = (double)f.f / 8388608.0;
        float f1;
        sscanf_s(dcmtst2[i], "%f", &f1);
        printf("%-11s = %3d %2d = 0x%06X %f %4d = %12.6e\n",
            dcmtst2[i], e10, endx, f.f, fr, f.e, u.f);
    }

    float floattst3[] = {
        0.78125e-2f, 1.5625e-2f, 3.125e-2f, 0.625e-1f, 1.25e-1f, 2.5e-1f, 5.e-1f,
        1.e0f, 2.e0f, 4.e0f, 8.e0f, 1.6e1f, 3.2e1f, 6.4e1f, 1.28e2f
    };
    printf("\n   IEEE754    = fraction 2^   =  to ASCII\n");
    for (int i = 0; i < sizeof floattst3 / sizeof floattst3[0]; ++i) {
        UFPP u;
        u.f = floattst3[i];
        FPU f;
        f.s = u.p.s;
        f.e = (signed)u.p.e - expbias;
        f.f = u.p.f | leadbit;
        double fr = (double)f.f / 8388608.0;
        char buf[16];
        fpu2dcm(buf, f);
        printf("%13.6e = %1.6f %4d = %-12s\n", 
            u.f, fr, f.e, buf);
    }
    return 0;
}
