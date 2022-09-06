/* fraction.cpp
 * IEEE754 compatible floating point for 65C02
 * See https://github.com/AndreAdrian/65C02-ICP/blob/main/floating-point.md
 * (C) 2022 Andre Adrian
 *
 * 2022-08-26: first version
 * 2022-08-30: dcm2fpu() for 10^-5 to 10^5, fpu2dcm() for 2^-7 to 2^7
 * 2022-08-31: use program fpconsts to create c1 to c5
 * 2022-09-02: allow Ibits in range 4 to 5, firstdigit=10.0, pseudo radix 16 exponent
 * 2022-09-03: fpu2dcm() output always X.XXXXXX
 * 2022-09-05: use 8 decimal digits because of Matula condition
 */

#include <cstdio>
#include <cstring>
#include <cstdint>

// The radix conversion algorithms need one 32bit multiplication and
// some 32bit additions or subtractions. The number 5.454545 needs
// 39 add or sub. This fast conversion needs (huge) tables, a
// space/time compromise. See Altair 4KByte Microsoft BASIC for
// slow conversion algorithms.

 // c1 Q constants 10.0, 1.0, ... 0.0000001 for fraction conversion
 // c2 Q fraction constants to convert exponent 10^x to exponent 2^y
 // first: x=Emin10, y=c3
 // c2 = 2^Fbits * 10^x / 2^y
 // c3 integer exponent constants, radix 10 to radix 2, e.g. 10^0 converts to 2^0 .. 2^3
 // c3 = y
 // c4 Q fraction constants to convert exponent 2^x to exponent 10^y
 // first: x=Emin2, y=c5
 // c4 = 2^Fbits * 2^x / 10^y
 // c5 integer exponent constants, radix 2 to radix 10, e.g. 2^3 converts to 10^0 .. 10^1

 // Program fpconsts creates the following values:
enum {
    Digits = 8,
    Ibits = 4,
    Emin10 = -31,
    Emin2 = -108,
};

unsigned long c1[] = {
0xA0000000, 0x10000000, 0x199999A, 0x28F5C3, 0x41893, 0x68DC, 0xA7C, 0x10C, 0x1B, 0x3,
};

unsigned long c2[] = {
0x1039D666, 0x14484BFF, 0xCAD2F7F, 0xFD87B5F, 0x13CE9A37, 0xC612062,
0xF79687B, 0x1357C29A, 0xC16D9A0, 0xF1C9008, 0x12E3B40A, 0xBCE5086,
0xEC1E4A8, 0x12725DD2, 0xB877AA3, 0xE69594C, 0x1203AF9F, 0x16849B87,
0xE12E134, 0x11979981, 0x15FD7FE1, 0xDBE6FED, 0x112E0BE8, 0x15798EE2,
0xD6BF94D, 0x10C6F7A1, 0x14F8B589, 0xD1B7176, 0x10624DD3, 0x147AE148,
0xCCCCCCD, 0x10000000, 0x14000000, 0xC800000, 0xFA00000, 0x13880000,
0xC350000, 0xF424000, 0x1312D000, 0xBEBC200, 0xEE6B280, 0x12A05F20,
0xBA43B74, 0xE8D4A51, 0x12309CE5, 0xB5E620F, 0xE35FA93, 0x11C37938,
0x16345786, 0xDE0B6B4, 0x1158E461, 0x15AF1D79, 0xD8D726B, 0x10F0CF06,
0x152D02C8, 0xD3C21BD, 0x108B2A2C, 0x14ADF4B7, 0xCECB8F2, 0x1027E72F,
0x1431E0FB, 0xC9F2C9D, 0xFC6F7C4, 0x13B8B5B5,
};

signed char c3[] = {
-103, -100, -96, -93, -90, -86, -83, -80, -76, -73, -70, -66,
-63, -60, -56, -53, -50, -47, -43, -40, -37, -33, -30, -27,
-23, -20, -17, -13, -10, -7, -3, 0, 3, 7, 10, 13,
17, 20, 23, 27, 30, 33, 37, 40, 43, 47, 50, 53,
56, 60, 63, 66, 70, 73, 76, 80, 83, 86, 90, 93,
96, 100, 103, 106,
};

unsigned long c4[] = {
0x314DC645, 0x7E37BE2, 0xC9F2C9D, 0x1431E0FB, 0x204FCE5E, 0x52B7D2E,
0x8459516, 0xD3C21BD, 0x152D02C8, 0x21E19E0D, 0x56BC75E, 0x8AC7230,
0xDE0B6B4, 0x16345786, 0x2386F270, 0x5AF3108, 0x9184E73, 0xE8D4A51,
0x174876E8, 0x2540BE40, 0x5F5E100, 0x9896800, 0xF424000, 0x186A0000,
0x27100000, 0x6400000, 0xA000000, 0x10000000, 0x1999999A, 0x28F5C28F,
0x68DB8BB, 0xA7C5AC4, 0x10C6F7A1, 0x1AD7F29B, 0x2AF31DC4, 0x6DF37F6,
0xAFEBFF1, 0x11979981, 0x1C25C268, 0x2D09370D, 0x734ACA6, 0xB877AA3,
0x12725DD2, 0x1D83C950, 0x2F394219, 0x78E4804, 0xC16D9A0, 0x1357C29A,
0x1EF2D0F6, 0x31848189, 0x7EC3DB0, 0xCAD2F7F, 0x14484BFF, 0x2073ACCB,
0x5313A5E, 0x84EC3C9,
};

signed char c5[] = {
-33, -31, -30, -29, -28, -26, -25, -24, -23, -22, -20, -19,
-18, -17, -16, -14, -13, -12, -11, -10, -8, -7, -6, -5,
-4, -2, -1, 0, 1, 2, 4, 5, 6, 7, 8, 10,
11, 12, 13, 14, 16, 17, 18, 19, 20, 22, 23, 24,
25, 26, 28, 29, 30, 31, 33, 34,
};

// No more fpconsts values.

enum {
    Ebias = 127,                // Exponent bias
    Leadbit = 0x800000,         // implicit/explicit leading bit
    Fbits = 32 - Ibits,         // proper fraction bits
    Rbits = Fbits - 23,         // rounding bits
    Rvalue = 1 << (Rbits - 1),  // 0.5 as fraction
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
    f.e = (signed)u.p.e - Ebias;
    f.f = u.p.f | Leadbit;
    return f;
}

// convert internal fp format to IEEE754
UFPP fpu2fpp(FPU f)
{
    UFPP u;
    u.p.s = f.s;
    u.p.e = (unsigned)f.e + Ebias;
    u.p.f = f.f & (~Leadbit);
    return u;
}

// 6502 style 8bit unsigned subtraction
// retval carry
inline int sub8(uint8_t& a, uint8_t b)
{
    int carry = b > a;
    a -= b;
    return carry;
}

// 6502 style 32bit unsigned addition
void add32(unsigned long& a, unsigned long b)
{
    a += b;
}

// 6502 style 32bit unsigned subtraction
// retval carry
int sub32(unsigned long& a, unsigned long b)
{
    int carry = b > a;
    a -= b;
    return carry;
}

// 6502 style 32bit unsigned multiply
void mul32(unsigned long long& a, unsigned long b)
{
    a *= b;
}

// converts ASCII to int [-32768 .. 32767]
int16_t dcm2int(char* buf)
{
    unsigned char s = 0;
    uint16_t v = 0;
    if ('-' == *buf) {
        s = 1;
        ++buf;
    }
    while (*buf >= '0') {
        v <<= 1;            // do v *= 10; with shifts
        v += (v << 2);
        v += (*buf - '0');
        ++buf;
    }
    int16_t w = (int16_t)v; // v and w can be a union
    if (s)
        w = 0 - w;
    return w;
}

int e10;    // use globals for easy print function internals
int endx;

// convert ASCII string -?[1-9].[0-9]*(e-?[0-9]+)? to fp
// RE: ?= 0 to 1 repetition, *=0 to N repetition, +=1 to N repetition
// () optional
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
    int8_t digits = 0;
    for (int8_t i = 1; i < sizeof c1 / sizeof c1[0]; ++i) {
        if ('.' == *buf) {
            digits = i;
            ++buf;
        }
        if (*buf < '0' || *buf > '9') 
            break;
        int8_t digit = *buf++ - '0';
        while (digit != 0) {
            add32(f.f, c1[i]);
            --digit;
        }
    }
    // do exponent part e-?[0-9]+
    if ('e' == *buf++) {
        e10 = dcm2int(buf);
        if (0 == f.f && 0 == e10) {     // zero 0.e0
            f.e = -Ebias;
            return f;
        }
        endx = e10 - Emin10;
        unsigned long long facc = f.f;
        mul32(facc, c2[endx]);
        f.f = facc >> Fbits;
        f.e = c3[endx];
    }

    f.f += Rvalue;
    // normalize
    const unsigned long Imask1 = ((1 << Ibits) - 2) << Fbits;
    while (f.f & Imask1) { // fraction to large
        f.f >>= 1;
        ++f.e;
    }
    const unsigned long Imask2 = ((1 << Ibits) - 1) << Fbits;
    while (0 == (f.f & Imask2)) { // fraction to small
        f.f <<= 1;
        --f.e;
    }
    f.f >>= Rbits;   // remove "rounding" bits
    return f;
}

// converts int [-99..99] to ASCII string
void exp2dcm(char* buf, int8_t e)
{
    if (e < 0) {
        *buf++ = '-';
        e = 0 - e;
    }
    uint8_t n = (uint8_t)e;     // n and e can be a union
    uint8_t digit = '0';
    // 6502 style unsigned division 8bit/8bit, result is digit
    while (0 == sub8(n, 10)) {
        ++digit;
    }
    n += 10;    // make result positive again
    if (digit != '0')
        *buf++ = digit;
    *buf++ = n + '0';   // remainder is n
    *buf = '\0';
}

// convert fraction to ASCII string (tricky)
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
    while ((unsigned char)f.e & 3) { // pseudo radix 16 exponent
        f.f <<= 1;
        --f.e;
    }
    f.f <<= Rbits;       // add "rounding" bits
    f.f += Rvalue;

    endx = (f.e - Emin2) >> 2;      // pseudo radix 16 exponent
    unsigned long long facc = f.f;  // 6502 style 32bit multiply 
    mul32(facc, c4[endx]);
    f.f = facc >> Fbits;

    // do fraction part
    // without flag output can be 00.XXXXXXX, 0X.XXXXXX or XX.XXXXX
    // pseudo mul10, div10 by suppress leading zeros and correct radix 10 exponent
    uint8_t flag = 0;
    uint8_t digits = Digits+1;  // 8 will print 7 digits because "real" output is 0X.XXXXXX
    int8_t exp10 = c5[endx];
    for (int8_t i = 0; i < digits; ++i) {
        int8_t digit = '0';
        // 6502 style unsigned division 32bit/8bit, result is digit
        while (0 == sub32(f.f, c1[i])) {
            ++digit;
        }
        add32(f.f, c1[i]);  // make result positive again
        *buf = digit;
#if 1
        if (0 == flag) {            // the tricky part
            if (digit != '0') {
                flag = 1;
                ++buf;
                *buf++ = '.';       // output dot after first non zero digit
                exp10 -= (i - 1);   // i=0 decrement exp, i=2 increment exp
                digits += (i - 1);  // i=0 one output digit less, i=2 one output digit more
            }
        }
        else
#else
        if (1 == i) *++buf = '.';
#endif
            ++buf;
    }
    if (exp10 != 0) {
        // do exponent part
        *buf++ = 'e';
        exp2dcm(buf, exp10);
    } else 
        *buf = '\0';
}

// test cases
int main()
{
    float floattst1[] = {
        0.0f, -0.0f, 0.125f, 0.25f, 0.5f, 1.0f, -1.0f,
        1.5f, 1.9999999f, 2.0f, 3.999999f, 4.0f, 8.0f, 9.999999f
    };
    printf("  IEEE754  =  hexfloat  = s exp implicit = fraction  2^\n");
    for (int i = 0; i < sizeof floattst1 / sizeof floattst1[0]; ++i) {
        UFPP u;
        u.f = floattst1[i];
        int ex = (signed)u.p.e - Ebias;
        double fr = (double)(u.p.f | Leadbit) / 8388608.0;
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
    printf("\nfrom ASCII= s explicit fraction 2^   =   to ASCII   =  IEEE754\n");
    for (int i = 0; i < sizeof dcmtst1 / sizeof dcmtst1[0]; ++i) {
        FPU f = dcm2fpu(dcmtst1[i]);
        double fr = (double)f.f / 8388608.0;
        char buf[16];
        fpu2dcm(buf, f);
        UFPP u = fpu2fpp(f);
        printf("%-9s = %d 0x%06X %1.6f %4d = %12s = %9.6f\n",
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
        int ex = (signed)u.p.e - Ebias;
        double fr = (double)(u.p.f | Leadbit) / 8388608.0;
        printf("%13.6e = %d %3d 0x%06X = %8.6f %4d\n",
            u.f, u.p.s, u.p.e, u.p.f, fr, ex);
    }

    char dcmtst2[][14] = {
         "1.e-31",  "1.e-5",  "1.e-4", "1.e-3", "1.e-2", "1.e-1",
         "1.e0", "8.e0", "0.8e1", "1.e1", "1.e2", "1.e3", "1.e4", "1.e5", "1.e32",
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
        1.e-31f, 0.78125e-2f, 1.5625e-2f, 3.125e-2f, 0.625e-1f, 1.25e-1f, 1.e-1f, 
        2.5e-1f, 5.e-1f, 1.e0f, 2.e0f, 4.e0f, 8.e0f, 1.6e1f, 3.2e1f, 6.4e1f, 
        1.28e2f, 1.e32f, 9.999999e32f
    };
    printf("\n   IEEE754    = fraction  2^   =   to ASCII\n");
    for (int i = 0; i < sizeof floattst3 / sizeof floattst3[0]; ++i) {
        UFPP u;
        u.f = floattst3[i];
        FPU f;
        f.s = u.p.s;
        f.e = (signed)u.p.e - Ebias;
        f.f = u.p.f | Leadbit;
        double fr = (double)f.f / 8388608.0;
        char buf[16];
        fpu2dcm(buf, f);
        printf("%13.6e = %9.7f %4d = %-12s\n", 
            u.f, fr, f.e, buf);
    }

    unsigned long floattst4[] = {
        0x3D000000, 0x3D7FFFFF, 0x3D800000, 0x3DFFFFFF,
        0x3E000000, 0x3E7FFFFF, 0x3E800000, 0x3EFFFFFF,
        0x3F000000, 0x3F7FFFFF, 0x3F800000, 0x3FFFFFFF,
        0x40000000, 0x407FFFFF, 0x40800000, 0x40FFFFFF, 
        0x41000000, 0x417FFFFF, 0x41800000, 0x41FFFFFF,
    };
    printf("\n IEEE754   = fraction  2^   =   to ASCII   =  IEEE754\n");
    for (int i = 0; i < sizeof floattst4 / sizeof floattst4[0]; ++i) {
        UFPP u;
        u.l = floattst4[i];
        FPU f;
        f.s = u.p.s;
        f.e = (signed)u.p.e - Ebias;
        f.f = u.p.f | Leadbit;
        double fr = (double)f.f / 8388608.0;
        char buf[16];
        fpu2dcm(buf, f);        // to ASCII
        FPU f2 = dcm2fpu(buf);
        UFPP u2 = fpu2fpp(f2);
        printf("0x%06X = %9.7f %4d = %-12s = 0x%06X\n",
            u.l, fr, f.e, buf, u2.l);
    }

    long double s, t = 3.0;
    s = 1.0 - (4.0 / t - 1.0) * t;
    printf("%e sizeof %d\n", s, sizeof s);
    return 0;
}
