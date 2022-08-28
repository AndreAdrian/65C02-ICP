/* fraction.cpp
 * IEEE754 compatible fraction prototype for 65C02
 * (C) 2022 Andre Adrian
 *
 * 2022-08-26: first version
 * 2022-08-27: print radix conversion exponent fraction correction constants
 * 2022-08-27: do some exponent fraction correction for dcm2fpu
 * 2022-08-28: refactoring
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

unsigned long mul32(unsigned long a, unsigned long b)
{
    return ((unsigned long long)a * b) >> 32;
}

// converts int [-32768 .. 32767] from ASCII string
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

// Constants for decimal to fraction conversion and vice versa range +/-[0.1 .. 1)
// Constants are unsigned q 0.32, that is q(0.1) = 0.1 * 2^32
unsigned long c[] = { 429496730, 42949673, 4294967, 429497, 42950, 4295, 429, 43 };

// radix 10 exp to radix 2 exp fraction conversion constants
// a=radix 10 exp, b=radix 2 exp, c=10^a * 2^(32 - b)
// Constants are unsigned Q0.32, a=3, b=10, c=4194304000 (0.9765625)
unsigned long c2[] = { 2417851639, 2361183241, 2305843009, 2251799814, 2199023256, 0,
    4194304000, 4096000000, 4000000000,  3906250000,  3814697266 };    // 32bit

// radix 10 exp to radix 2 exp exp conversion constants
// Constants are signed int
signed char c3[] = { -49, -39, -29, -19, -9, 0, 10, 20, 30, 40, 50 };

int e10;    // use globals for easy print function internals
int endx;

// converts fp (-1 .. -0.1] and [0.1 .. 1) from ASCII string
FPU dcm2fpu(char* buf)
{
    FPU f;
    f.s = 0;
    f.e = -1;
    f.f = round;

    // do fraction part -?0.[0-9]+
    // regular expression ?= 0 to 1 repetition, +=1 to N repetition
    if ('-' == *buf) {
        f.s = 1;
        ++buf;
    }
    ++buf;  // read over 0
    ++buf;  // read over .
    for (int i = 0; i < sizeof c / sizeof c[0]; ++i) {
        if (*buf < '0' || *buf > '9') break;
        unsigned long digit = *buf++ - '0';
        f.f += digit * c[i];
    }
    // do exponent part e-?[0-9]+
    if ('e' == *buf++) {
        e10 = dcm2int(buf);
        endx = (e10 + 15) / 3;
        if (c3[endx] != 0) {
            f.f = mul32(f.f, c2[endx]); // disadvantage: needs 32bit mul
            f.e += c3[endx];
        }
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
int main()
{
    float floattst1[] = {
        0.0f, -0.0f, 0.1f, 0.125f, 0.25f, 0.5f, 0.75f, 0.99999995f, 1.0f, -1.0f,
        1.5f, 1.9999999f, 2.0f, 4.0f, 8.0f
    };
    printf("  IEEE754  =  hexfloat  = s exp hexfrac. = fraction  2^\n");
    for (int i = 0; i < sizeof floattst1 / sizeof floattst1[0]; ++i) {
        UFPP u;
        u.f = floattst1[i];
        int ex = (signed)u.p.e - expbias;
        double fr = (double)(u.p.f | leadbit) / 8388608.0;
        printf("%10.7f = 0x%08X = %d %3d 0x%06X = %9.7f %4d\n", 
            u.f, u.l, u.p.s, u.p.e, u.p.f, fr, ex);
    }

    char dcmtst1[][12] = {
        "0.1", "0.11", "0.101", "0.1001", "0.10001", "0.100001", "0.1000001", "0.10000001",
        "0.5", "0.55", "0.505", "0.5005", "0.50005", "0.500005", "0.5000005", "0.50000005",
        "0.9", "0.99", "0.909", "0.9009", "0.90009", "0.900009", "0.9000009", "0.90000009",
        "0.99999995",
        "-0.10000001", "-0.50000005", "-0.90000009", "-0.99999995"
    };
    printf("\nfrom string = s  2^  hexfrac.  = to string  =   IEEE754\n");
    for (int i = 0; i < sizeof dcmtst1 / sizeof dcmtst1[0]; ++i) {
        FPU f = dcm2fpu(dcmtst1[i]);
        char buf[16];
        fpu2dcm(buf, f);
        UFPP u = fpu2fpp(f);
        printf("%-11s = %d %4d 0x%06X = %11s = %11.8f\n",
            dcmtst1[i], f.s, f.e, f.f, buf, u.f);
    }

    float floattst2[] = {
        0.1e-15f, 0.1e-12f, 0.1e-9f, 0.1e-6f, 0.1e-3f, 0.1f,
        0.1e3f, 0.1e6f, 0.1e9f, 0.1e12f, 0.1e15f
    };
    printf("\n   IEEE754    = s exp hexfrac. = fraction  2^   = fraction  2^\n");
    for (int i = 0; i < sizeof floattst2 / sizeof floattst2[0]; ++i) {
        UFPP u;
        u.f = floattst2[i];
        int ex = (signed)u.p.e - expbias;
        double fr = (double)(u.p.f | leadbit) / 8388608.0;
        int ex2 = (signed)u.p.e - expbias + 1;
        double fr2 = (double)(u.p.f | leadbit) / 16777216.0;
        printf("%13.6e = %d %3d 0x%06X = %9.7f %4d = %9.7f %4d\n",
            u.f, u.p.s, u.p.e, u.p.f, fr, ex, fr2, ex2);
    }

    char dcmtst2[][14] = {
         "0.1e-15",  "0.1e-12", "0.1e-9", "0.1e-6", "0.1e-3",
         "0.1e0", "0.1e3", "0.1e6", "0.1e9", "0.1e12", "0.1e15",
         "0.55e0", "0.505e3", "0.5005e6", "0.50005e9", "0.500005e12", "0.5000005e15"
    };
    printf("\n from string  = 10^ ndx= hexfrac. fraction 2^ =   IEEE754\n");
    for (int i = 0; i < sizeof dcmtst2 / sizeof dcmtst2[0]; ++i) {
        FPU f = dcm2fpu(dcmtst2[i]);
        UFPP u = fpu2fpp(f);
        double fr2 = (double)f.f / 16777216.0;
        float f1;
        sscanf_s(dcmtst2[i], "%f", &f1);
        printf("%-13s = %3d %2d = 0x%06X %f %3d = %12.6e\n",
            dcmtst2[i], e10, endx, f.f, fr2, f.e, u.f);
    }

    return 0;
}
