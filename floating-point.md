In my Z80 dayes, I wrote a 32bit integer package and a 32bit [CORDIC package](http://www.andreadrian.de/oldcpu/Z80_number_cruncher.html). Now, I write a IEEE754 compatible floating point (fp) package for the 6502. There are some 6502 fp packages available. First the Steve Wozniak/Roy Rankin package that was published in Dr.Dobb's Journal in August 1976. Second the fp package in Microsoft BASIC for Apple 2, Commodore PET and others.

Since 1985 IEEE754, the "IEEE Standard for Binary Floating-Point Arithmetic" is used for floating point. The Intel 8087 co-processor was one of the first implementations of IEEE754 in hardware. For me, IEEE754 is a very compatible way to exchange fp numbers between different computers. Every IEEE754 fp number has three parts:
- a sign bit
- a biased exponent
- an unsigned improper fraction

An improper fraction has a numerator that is greater than the denominator, like 3/2. The [Q notation](https://en.wikipedia.org/wiki/Q_(number_format)) or binary fixed point is helpful to understand improper binary fractions. The IEEE754 single (32bit) format has an unsigned Q1.23 improper fraction, that is one integer bit before the binary dot and 23 proper fraction bits after the binary bit. An unsigned Q1.23 number can store a decimal number in the range from 1.0 to 2.0 - 2^-22 or 1.9999997615... The right most bit in the Q1.23 number has the value 2^-22 or 0.0000002384...
You can write a Q number as a decimal fraction or as an integer. The decimal fraction 1.0 is 1.0 * 2^23 = 8388608 = 0x800000. The decimal fraction 1.5 is 1.5 * 2^23 = 12582912 = 0xC00000, and 1.9999997615... is 16777215 = 0xFFFFFF. For decimal fraction to integer conversion, you multiply with 2^number_of_fraction_bits.
For integer to decimal fraction conversion, you divide by 2^number_of_fraction_bits.

Here are some IEEE754 single (32bit) numbers as printed by fraction.cpp:
```
  IEEE754  =  hexfloat  = s exp hexfrac. = fraction  2^
 0.0000000 = 0x00000000 = 0   0 0x000000 = 1.0000000 -127
-0.0000000 = 0x80000000 = 1   0 0x000000 = 1.0000000 -127
 0.1000000 = 0x3DCCCCCD = 0 123 0x4CCCCD = 1.6000000   -4
 0.1250000 = 0x3E000000 = 0 124 0x000000 = 1.0000000   -3
 0.2500000 = 0x3E800000 = 0 125 0x000000 = 1.0000000   -2
 0.5000000 = 0x3F000000 = 0 126 0x000000 = 1.0000000   -1
 0.7500000 = 0x3F400000 = 0 126 0x400000 = 1.5000000   -1
 0.9999999 = 0x3F7FFFFF = 0 126 0x7FFFFF = 1.9999999   -1
 1.0000000 = 0x3F800000 = 0 127 0x000000 = 1.0000000    0
-1.0000000 = 0xBF800000 = 1 127 0x000000 = 1.0000000    0
 1.5000000 = 0x3FC00000 = 0 127 0x400000 = 1.5000000    0
 1.9999999 = 0x3FFFFFFF = 0 127 0x7FFFFF = 1.9999999    0
 2.0000000 = 0x40000000 = 0 128 0x000000 = 1.0000000    1
 4.0000000 = 0x40800000 = 0 129 0x000000 = 1.0000000    2
 8.0000000 = 0x41000000 = 0 130 0x000000 = 1.0000000    3
```
Column s shows the sign, exp shows the biased exponent in decimal and hexfrac. shows the unsigned fraction in hexadecimal. Column fraction shows the decimal fraction and column 2^ shows the "true" exponent. Did you know that the IEEE754 format has a positive and a negative zero? And that you can express 510 different valid numbers that all have a fraction value of 0? These numbers are -0, -2^-126 to -2^127, +0 and 2^-126 to 2^127. Some examples are 0.125, 0.25, 0.5, 1.0, 2.0, 4.0 and the negative versions like -2.0, -1.0, -0.5.

The number 1.0 is a good example to explain details of the IEEE754 32bit format. The improper fraction part uses 24 bits for an unsigned Q1.23 number. That is one binary bit, the leading bit, before the binary point and 23 bits after the binary point. The leading bit of a "normalized" IEEE754 fraction always has the value 1. The leading bit is "implicit" or not stored in the IEEE754 format, you have to add it if you need. The "true" binary exponent of 1.0 is 2^0. IEEE754 uses biased exponent with a bias value of 127 for single format, not 2ers complement. You add 127 to convert from "true" binary exponent to biased exponent. Likewise, you subtract 127 to convert from biased exponent to "true" exponent. It is easier to compare fp numbers that use biased exponent and use the order sign, exponent, fraction. You can use signed 32bit integer compare, you need no fp compare.

There are "best practices" for an integer package, which I used in int.asm. With a fp package you have more alternatives. Let's start with radix conversion, that is the conversion between human readable ASCII string in decimal (Radix 10) system to and from computer internal binary (Radix 2) system. Donald Knuth writes in "The Art of Computer programming, Volume 2 Seminumerical Algorithms, chapter 4.4 Radix conversion" about "The four basic methods" for radix conversion. The subchapter "Converting fractions" tells us "No equally fast method of converting fractions manually is known. The best way seems to be Method 2a \[Multiplication by B using radix-b arithmetric\]". The subchapter "Floating point conversion" tells the complete story of radix conversion: "it is necessary to deal with both the exponent and the fraction parts simultaneously, since conversion of the exponent will affect the fraction part".

The fraction program can convert fp numbers between -1 and -0.1 and +0.1 to 1 from decimal to fp and vice versa without "to deal with both the exponent and the fraction parts simultaneously". As you see above, the decimal number 0.1 is stored as IEEE754 number 1.6 * 2^-4 and 0.9999999 is stored as 1.9999999 * 2^-1. The ASCII (decimal) to improper fraction function produces a "denormalized" fraction. The radix 10 to radix 2 conversion constants are defined for an exponent of 2^-1. To store the denormalized fraction, we need at least 27 bits. We use 32 bits. The additional 5 bits are used as "rounding" bits. To "normalize" the fraction, we shift the fraction to the left, until the leading bit is 1. For every successful shift, we decrease the exponent by one.

The following table shows the quality of my decimal-to-fp function dcm2fpu() and fp-to-decimal function fpu2dcm(). The "to string" column shows the result of a decimal to fp to decimal again conversion. The "IEEE754" column uses the conversion that is part of printf() as reference. The columns s, 2^, hexfrac. show the internal fp format with 2ers complement exponent and explicit leading bit fraction.
```
from string = s  2^  hexfrac.  = to string  =   IEEE754
0.1         = 0   -4 0xCCCCD0 =  0.10000002 =  0.10000002
0.11        = 0   -4 0xE147B2 =  0.11000003 =  0.11000003
0.101       = 0   -4 0xCED91A =  0.10100002 =  0.10100003
0.1001      = 0   -4 0xCD013E =  0.10010002 =  0.10010003
0.10001     = 0   -4 0xCCD20F =  0.10001003 =  0.10001003
0.100001    = 0   -4 0xCCCD57 =  0.10000103 =  0.10000103
0.1000001   = 0   -4 0xCCCCDE =  0.10000013 =  0.10000013
0.10000001  = 0   -4 0xCCCCD2 =  0.10000004 =  0.10000004
0.5         = 0   -1 0x800000 =  0.50000002 =  0.50000000
0.55        = 0   -1 0x8CCCCD =  0.55000004 =  0.55000001
0.505       = 0   -1 0x8147AE =  0.50500002 =  0.50500000
0.5005      = 0   -1 0x8020C5 =  0.50050005 =  0.50050002
0.50005     = 0   -1 0x800347 =  0.50005003 =  0.50005001
0.500005    = 0   -1 0x800054 =  0.50000503 =  0.50000501
0.5000005   = 0   -1 0x800008 =  0.50000050 =  0.50000048
0.50000005  = 0   -1 0x800001 =  0.50000008 =  0.50000006
0.9         = 0   -1 0xE66666 =  0.90000000 =  0.89999998
0.99        = 0   -1 0xFD70A4 =  0.99000003 =  0.99000001
0.909       = 0   -1 0xE8B439 =  0.90900000 =  0.90899998
0.9009      = 0   -1 0xE6A162 =  0.90090003 =  0.90090001
0.90009     = 0   -1 0xE66C4C =  0.90009000 =  0.90008998
0.900009    = 0   -1 0xE666FD =  0.90000900 =  0.90000898
0.9000009   = 0   -1 0xE66675 =  0.90000090 =  0.90000087
0.90000009  = 0   -1 0xE66668 =  0.90000012 =  0.90000010
0.99999995  = 0   -1 0xFFFFFF =  0.99999996 =  0.99999994
-0.10000001 = 1   -4 0xCCCCD2 = -0.10000004 = -0.10000004
-0.50000005 = 1   -1 0x800001 = -0.50000008 = -0.50000006
-0.90000009 = 1   -1 0xE66668 = -0.90000012 = -0.90000010
-0.99999995 = 1   -1 0xFFFFFF = -0.99999996 = -0.99999994
```
You find the fraction.cpp source code in folder fraction, ready to compile with Microsoft Visual Studio. But the source code should compile and run on Linux (very good OS) and MacOS (just another UNIX), too.

The following table looks into "deal with both the exponent and the fraction parts simultaneously". I use the engineering scaling of radix 10 exponents, that is steps of 3, like 10^-3, 10^0, 10^3 in the range from 10^-15 to 10^15. The first column shows the IEEE754 number as converted by printf(). The second column shows the IEEE754 parts sign, biased exponent and fraction with implicit leading bit. The third column shows the fraction as decimal fraction and the "true" exponent for radix 2 in the normal IEEE754 notation with 1.0 <= fraction < 2.0. The last column shows the fraction with 0.5 <= fraction < 1.0. I use this interpretation of IEEE754, because it makes floating point conversion from ASCII to binary easier.
```
   IEEE754    = s exp hexfrac. = fraction  2^   = fraction  2^
 1.000000e-16 = 0  73 0x669595 = 1.8014399  -54 = 0.9007199  -53
 1.000000e-13 = 0  83 0x612E13 = 1.7592186  -44 = 0.8796093  -43
 1.000000e-10 = 0  93 0x5BE6FF = 1.7179869  -34 = 0.8589935  -33
 1.000000e-07 = 0 103 0x56BF95 = 1.6777216  -24 = 0.8388608  -23
 1.000000e-04 = 0 113 0x51B717 = 1.6384000  -14 = 0.8192000  -13
 1.000000e-01 = 0 123 0x4CCCCD = 1.6000000   -4 = 0.8000000   -3
 1.000000e+02 = 0 133 0x480000 = 1.5625000    6 = 0.7812500    7
 1.000000e+05 = 0 143 0x435000 = 1.5258789   16 = 0.7629395   17
 1.000000e+08 = 0 153 0x3EBC20 = 1.4901161   26 = 0.7450581   27
 1.000000e+11 = 0 163 0x3A43B7 = 1.4551915   36 = 0.7275957   37
 1.000000e+14 = 0 173 0x35E621 = 1.4210855   46 = 0.7105427   47
```
The following table shows ASCII (decimal) conversion for fraction and exponent from radix 10 to radix 2. The fraction and exponent conversion constants are only correct for one exponent value. Therefore we have to do the "fraction correction" multiplication with at least 27 bits before we can do normalization from 27 bits to 24 bits. 
```
 from string  = 10^ ndx= hexfrac. fraction 2^ =   IEEE754
0.1e-15       = -15  0 = 0xE69599 0.900720 -54 = 1.000000e-16
0.1e-12       = -12  1 = 0xE12E17 0.879610 -44 = 1.000000e-13
0.1e-9        =  -9  2 = 0xDBE703 0.858994 -34 = 1.000000e-10
0.1e-6        =  -6  3 = 0xD6BF99 0.838861 -24 = 1.000000e-07
0.1e-3        =  -3  4 = 0xD1B71B 0.819200 -14 = 1.000000e-04
0.1e0         =   0  5 = 0xCCCCD0 0.800000  -4 = 1.000000e-01
0.1e3         =   3  6 = 0xC80003 0.781250   6 = 1.000000e+02
0.1e6         =   6  7 = 0xC35003 0.762940  16 = 1.000000e+05
0.1e9         =   9  8 = 0xBEBC23 0.745058  26 = 1.000000e+08
0.1e12        =  12  9 = 0xBA43BA 0.727596  36 = 1.000000e+11
0.1e15        =  15 10 = 0xB5E624 0.710543  46 = 1.000000e+14
0.55e0        =   0  5 = 0x8CCCCD 0.550000  -1 = 5.500000e-01
0.505e3       =   3  6 = 0xFC8000 0.986328   8 = 5.050000e+02
0.5005e6      =   6  7 = 0xF46280 0.954628  18 = 5.005000e+05
0.50005e9     =   9  8 = 0xEE7143 0.931416  28 = 5.000500e+08
0.500005e12   =  12  9 = 0xE8D53E 0.909504  38 = 5.000050e+11
0.5000005e15  =  15 10 = 0xE35FB8 0.888179  48 = 5.000005e+14
```

To be continued ...
