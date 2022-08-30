In my Z80 dayes, I wrote a 32bit integer package and a 32bit [CORDIC package](http://www.andreadrian.de/oldcpu/Z80_number_cruncher.html). Now, I write a IEEE754 compatible floating point (fp) package for the 6502. There are some 6502 fp packages available. First the Steve Wozniak/Roy Rankin package that was published in Dr.Dobb's Journal in August 1976. Second the fp package in Microsoft BASIC for Apple 2, Commodore PET and others.

Since 1985 IEEE754, the "IEEE Standard for Binary Floating-Point Arithmetic" is used for floating point. The Intel 8087 co-processor was one of the first implementations of IEEE754 in hardware. For me, IEEE754 is a good fp format for the 6502. Every IEEE754 fp number has three parts:
- a sign bit
- a biased exponent
- an unsigned improper fraction

An improper fraction has a numerator that is greater than the denominator, like 3/2. The [Q notation](https://en.wikipedia.org/wiki/Q_(number_format)) or binary fixed point is helpful to understand improper binary fractions. The IEEE754 single (32bit) format has an unsigned Q1.23 improper fraction, that is one integer bit before the binary dot and 23 proper fraction bits after the binary bit. An unsigned Q1.23 number can store a decimal number in the range from 1.0 to 2.0 - 2^-22 or 1.9999997615... The right most bit in the Q1.23 number has the value 2^-22 or 0.0000002384...
You can write a Q number as a decimal fraction or as an integer. The decimal fraction 1.0 is 1.0 * 2^23 = 8388608 = 0x800000. The decimal fraction 1.5 is 1.5 * 2^23 = 12582912 = 0xC00000, and 1.9999997615... is 16777215 = 0xFFFFFF. For decimal fraction to integer conversion, you multiply with 2^number_of_fraction_bits.
For integer to decimal fraction conversion, you divide by 2^number_of_fraction_bits.

Here are some IEEE754 single (32bit) numbers as printed by fraction.cpp:
```
  IEEE754  =  hexfloat  = s exp implicit = fraction  2^
 0.0000000 = 0x00000000 = 0   0 0x000000 = 1.0000000 -127
-0.0000000 = 0x80000000 = 1   0 0x000000 = 1.0000000 -127
 0.1250000 = 0x3E000000 = 0 124 0x000000 = 1.0000000   -3
 0.2500000 = 0x3E800000 = 0 125 0x000000 = 1.0000000   -2
 0.5000000 = 0x3F000000 = 0 126 0x000000 = 1.0000000   -1
 1.0000000 = 0x3F800000 = 0 127 0x000000 = 1.0000000    0
-1.0000000 = 0xBF800000 = 1 127 0x000000 = 1.0000000    0
 1.5000000 = 0x3FC00000 = 0 127 0x400000 = 1.5000000    0
 1.9999999 = 0x3FFFFFFF = 0 127 0x7FFFFF = 1.9999999    0
 2.0000000 = 0x40000000 = 0 128 0x000000 = 1.0000000    1
 4.0000000 = 0x40800000 = 0 129 0x000000 = 1.0000000    2
 8.0000000 = 0x41000000 = 0 130 0x000000 = 1.0000000    3
 9.9999990 = 0x411FFFFF = 0 130 0x1FFFFF = 1.2499999    3
```
Column s shows the sign, exp shows the biased exponent in decimal and implicit shows the unsigned fraction in hexadecimal. Column fraction shows the decimal fraction and column 2^ shows the "true" exponent. Did you know that the IEEE754 format has a positive and a negative zero? And that you can express 510 different valid numbers that all have a fraction value of 0? These numbers are -0, -2^-126 to -2^127, +0 and 2^-126 to 2^127. Some examples are 0.125, 0.25, 0.5, 1.0, 2.0, 4.0 and the negative versions like -2.0, -1.0, -0.5.

The number 1.0 is a good example to explain details of the IEEE754 32bit format. The improper fraction part uses 24 bits for an unsigned Q1.23 number. That is one binary bit, the leading bit, before the binary point and 23 bits after the binary point. The leading bit of a "normalized" IEEE754 fraction always has the value 1. The leading bit is "implicit" or not stored in the IEEE754 format, you have to add it if you need. The "true" binary exponent of 1.0 is 2^0. IEEE754 uses biased exponent with a bias value of 127 for single format, not 2ers complement. You add 127 to convert from "true" binary exponent to biased exponent. Likewise, you subtract 127 to convert from biased exponent to "true" exponent. It is easier to compare fp numbers that use biased exponent and use the order sign, exponent, fraction. You can use signed 32bit integer compare, you need no fp compare.

There are "best practices" for an integer package, which I used in int.asm. With a fp package you have more alternatives. Let's start with radix conversion, that is the conversion between human readable ASCII string in decimal (Radix 10) system to and from computer internal binary (Radix 2) system. Donald Knuth writes in "The Art of Computer programming, Volume 2 Seminumerical Algorithms, chapter 4.4 Radix conversion" about "The four basic methods" for radix conversion. The subchapter "Converting fractions" tells us "No equally fast method of converting fractions manually is known. The best way seems to be Method 2a \[Multiplication by B using radix-b arithmetric\]". The subchapter "Floating point conversion" tells the complete story of radix conversion: "it is necessary to deal with both the exponent and the fraction parts simultaneously, since conversion of the exponent will affect the fraction part".

The fraction program can convert fp numbers between -10 and -1 and 1 to 10 from decimal to fp and vice versa without need "to deal with both the exponent and the fraction parts simultaneously". As you see above, the decimal number 1.0 is stored as IEEE754 number 1.0 * 2^0 and 9.999999 is stored as 1.249999 * 2^3. The ASCII (decimal) to improper fraction function produces a "denormalized" fraction. The radix 10 to radix 2 conversion constants are defined for an exponent of 2^-0 or 10^0. To store the denormalized fraction, we need at least 27 bits. We use 32 bits. The additional 5 bits are used as "rounding" bits. To "normalize" the fraction, we shift the fraction to the left, until the leading bit is 1. For every successful shift, we decrease the exponent by one.

The following table shows the quality of my decimal-to-fp function dcm2fpu() and fp-to-decimal function fpu2dcm(). The "to ASCII" column shows the result of a decimal to fp to decimal again conversion. The "IEEE754" column uses the conversion that is part of printf() as reference. The columns s, explicit, fraction and 2^ show the internal fp format with 2ers complement exponent and explicit leading bit fraction.
```
from ASCII= s explicit fraction 2^   =   to ASCII  =  IEEE754
1.        = 0 0x800000 1.000000    0 =  1.000000e0 =  1.000000
1.1       = 0 0x8CCCCD 1.100000    0 =  1.100000e0 =  1.100000
1.01      = 0 0x8147AF 1.010000    0 =  1.010000e0 =  1.010000
1.001     = 0 0x8020C5 1.001000    0 =  1.001000e0 =  1.001000
1.0001    = 0 0x800347 1.000100    0 =  1.000100e0 =  1.000100
1.00001   = 0 0x800054 1.000010    0 =  1.000010e0 =  1.000010
1.000001  = 0 0x800009 1.000001    0 =  1.000001e0 =  1.000001
5.        = 0 0xA00000 1.250000    2 =  5.000000e0 =  5.000000
5.5       = 0 0xB00000 1.375000    2 =  5.500000e0 =  5.500000
5.05      = 0 0xA19999 1.262500    2 =  5.050000e0 =  5.050000
5.005     = 0 0xA028F5 1.251250    2 =  5.005000e0 =  5.005000
5.0005    = 0 0xA00418 1.250125    2 =  5.000500e0 =  5.000500
5.00005   = 0 0xA00069 1.250013    2 =  5.000050e0 =  5.000050
5.000005  = 0 0xA0000A 1.250001    2 =  5.000005e0 =  5.000005
9.        = 0 0x900000 1.125000    3 =  0.900000e1 =  9.000000
9.9       = 0 0x9E6666 1.237500    3 =  0.990000e1 =  9.900000
9.09      = 0 0x9170A3 1.136250    3 =  0.909000e1 =  9.089999
9.009     = 0 0x9024DD 1.126125    3 =  0.900900e1 =  9.009000
9.0009    = 0 0x9003AF 1.125112    3 =  0.900090e1 =  9.000899
9.00009   = 0 0x90005E 1.125011    3 =  0.900009e1 =  9.000090
9.000009  = 0 0x900009 1.125001    3 =  0.900000e1 =  9.000009
9.999999  = 0 0x9FFFFF 1.250000    3 =  0.999999e1 =  9.999999
-1.000001 = 1 0x800009 1.000001    0 = -1.000001e0 = -1.000001
-5.000005 = 1 0xA0000A 1.250001    2 = -5.000005e0 = -5.000005
-9.000009 = 1 0x900009 1.125001    3 = -0.900000e1 = -9.000009
-9.999999 = 1 0x9FFFFF 1.250000    3 = -0.999999e1 = -9.999999
0.        = 0 0x000000 0.000000 -127 =          0. =  0.000000
-0.       = 1 0x000000 0.000000 -127 =         -0. = -0.000000
```
You find the fraction.cpp source code in folder fraction, ready to compile with Microsoft Visual Studio. But the source code should compile and run on Linux (very good OS) and MacOS (just another UNIX), too.

The following table looks into "deal with both the exponent and the fraction parts simultaneously". The first column shows the IEEE754 number as converted by printf(). The second column shows the IEEE754 parts sign, biased exponent and fraction with implicit leading bit. The third column shows the fraction as decimal fraction and the "true" exponent for radix 2 in the normal IEEE754 notation with 1.0 <= fraction < 2.0.
```
   IEEE754    = s exp implicit = fraction  2^
 1.000000e-05 = 0 110 0x27C5AC = 1.310720  -17
 1.000000e-04 = 0 113 0x51B717 = 1.638400  -14
 1.000000e-03 = 0 117 0x03126F = 1.024000  -10
 1.000000e-02 = 0 120 0x23D70A = 1.280000   -7
 1.000000e-01 = 0 123 0x4CCCCD = 1.600000   -4
 1.000000e+00 = 0 127 0x000000 = 1.000000    0
 1.000000e+01 = 0 130 0x200000 = 1.250000    3
 1.000000e+02 = 0 133 0x480000 = 1.562500    6
 1.000000e+03 = 0 136 0x7A0000 = 1.953125    9
 1.000000e+04 = 0 140 0x1C4000 = 1.220703   13
 1.000000e+05 = 0 143 0x435000 = 1.525879   16
```
The following table shows ASCII (decimal) conversion for fraction and exponent from radix 10 to radix 2. The fraction and exponent conversion constants are only correct for one exponent value. Therefore we have to do the "fraction correction" multiplication with at least 27 bits before we can do normalization from 27 bits to 24 bits. 
```
 from ASCII = 10^ ndx= explicit fraction 2^   =   IEEE754
1.e-5       =  -5  0 = 0xA7C5AD 1.310720  -17 = 1.000000e-05
1.e-4       =  -4  1 = 0xD1B718 1.638400  -14 = 1.000000e-04
1.e-3       =  -3  2 = 0x83126F 1.024000  -10 = 1.000000e-03
1.e-2       =  -2  3 = 0xA3D70B 1.280000   -7 = 1.000000e-02
1.e-1       =  -1  4 = 0xCCCCCE 1.600000   -4 = 1.000000e-01
1.e0        =   0  5 = 0x800000 1.000000    0 = 1.000000e+00
1.e1        =   1  6 = 0xA00001 1.250000    3 = 1.000000e+01
1.e2        =   2  7 = 0xC80001 1.562500    6 = 1.000000e+02
1.e3        =   3  8 = 0xFA0001 1.953125    9 = 1.000000e+03
1.e4        =   4  9 = 0x9C4001 1.220703   13 = 1.000000e+04
1.e5        =   5 10 = 0xC35001 1.525879   16 = 1.000000e+05
5.e-3       =  -3  2 = 0xA3D70A 1.280000   -8 = 5.000000e-03
5.5e-2      =  -2  3 = 0xE147AE 1.760000   -5 = 5.500000e-02
5.05e-1     =  -1  4 = 0x8147AE 1.010000   -1 = 5.050000e-01
5.005e0     =   0  5 = 0xA028F5 1.251250    2 = 5.005000e+00
5.0005e1    =   1  6 = 0xC8051F 1.562656    5 = 5.000500e+01
5.00005e2   =   2  7 = 0xFA00A4 1.953145    8 = 5.000050e+02
5.000005e3  =   3  8 = 0x9C400A 1.220704   12 = 5.000005e+03
```
The last table (for now) shows radix 2 to radix 10 conversion. The radix 2 exponent 2^3 can have a fraction from 1.0 to 1.999999. The radix 10 representation is 8.0 * 10^0 to 1.599999 * 10^1. To make conversion easy, 8.0 * 10^0 is written as 0.8e1. One radix 2 exponent value is mapped to one radix 10 exponent value.
```
   IEEE754    = fraction 2^   =  to ASCII
 7.812500e-03 = 1.000000   -7 = 0.781250e-2
 1.562500e-02 = 1.000000   -6 = 1.562500e-2
 3.125000e-02 = 1.000000   -5 = 3.125000e-2
 6.250000e-02 = 1.000000   -4 = 0.625000e-1
 1.250000e-01 = 1.000000   -3 = 1.250000e-1
 2.500000e-01 = 1.000000   -2 = 2.500000e-1
 5.000000e-01 = 1.000000   -1 = 5.000000e-1
 1.000000e+00 = 1.000000    0 = 1.000000e0
 2.000000e+00 = 1.000000    1 = 2.000000e0
 4.000000e+00 = 1.000000    2 = 4.000000e0
 8.000000e+00 = 1.000000    3 = 0.800000e1
 1.600000e+01 = 1.000000    4 = 1.600000e1
 3.200000e+01 = 1.000000    5 = 1.000000e1
 6.400000e+01 = 1.000000    6 = 0.640000e2
 1.280000e+02 = 1.000000    7 = 1.280000e2
```
The current program version can handle radix 10 fp in the range 10^-5 to 10^5 and radix 2 fp in the range 2^-7 to 2^7. You can enhance the exponent range by adding more "magic numbers" to the arrays c2, c3, c4, c5. The purpose of these arrays are:
```
c1: Q4.28 constants for 1.0, 0.1, ... 0.000001 for fraction conversion
c2: Q4.28 fraction constants to convert exponent 10^x to exponent 2^y
c3: integer exponent constants, radix 10 to radix 2, e.g. 10^-5 converts to 2^-17
c4: Q4.28 fraction constants to convert exponent 2^x to exponent 10^y
c5: integer exponent constants, radix 2 to radix 10, e.g. 2^-7 converts to 10^-2
```
