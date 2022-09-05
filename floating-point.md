In my Z80 dayes, I wrote a 32bit integer package and a 32bit [CORDIC package](http://www.andreadrian.de/oldcpu/Z80_number_cruncher.html). Now, I write a IEEE754 compatible floating point (fp) package for the 6502. There are some 6502 fp packages available. First the Steve Wozniak/Roy Rankin package that was published in Dr.Dobb's Journal in August 1976. Second the fp package in Microsoft BASIC for Apple 2, Commodore PET and others.

Since 1985 IEEE754, the "IEEE Standard for Binary Floating-Point Arithmetic" is used for floating point. The Intel 8087 co-processor was one of the first implementations of IEEE754 in hardware. For me, IEEE754 is a good fp format for the 6502. Every IEEE754 fp number has three fields:
- sign bit s
- biased exponent e
- unsigned improper fraction f

An improper fraction has a numerator that is greater than the denominator, like 3/2. The [Q notation](https://en.wikipedia.org/wiki/Q_(number_format)) or binary fixed point is helpful to understand improper binary fractions. The IEEE754 single (32bit) format has an unsigned Q1.23 improper fraction, that is one integer bit before the binary dot and 23 proper fraction bits after the binary bit. An unsigned Q1.23 number can store a decimal number in the range from 1.0 to 2.0 - 2^-22 or 1.9999997615... The right most bit in the Q1.23 number has the value 2^-22 or 0.0000002384...
You can write a Q number as a decimal fraction or as an integer. The decimal fraction 1.0 is integer 1.0 * 2^23 = 8388608 = 0x800000. The decimal fraction 1.5 is 1.5 * 2^23 = 12582912 = 0xC00000, and 1.9999997615... is 16777215 = 0xFFFFFF. For decimal fraction to integer conversion, you multiply with 2^number_of_fraction_bits. For integer to decimal fraction conversion, you divide by 2^number_of_fraction_bits.

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
 3.9999990 = 0x407FFFFC = 0 128 0x7FFFFC = 1.9999995    1
 4.0000000 = 0x40800000 = 0 129 0x000000 = 1.0000000    2
 8.0000000 = 0x41000000 = 0 130 0x000000 = 1.0000000    3
 9.9999990 = 0x411FFFFF = 0 130 0x1FFFFF = 1.2499999    3
```
Column s shows the sign, exp shows the biased exponent in decimal and implicit shows the unsigned fraction in hexadecimal. Column fraction shows the decimal fraction and column 2^ shows the "true" exponent. Did you know that the IEEE754 format has a positive and a negative zero? And that you can express 510 different valid numbers that all have a fraction value of 0? These numbers are -0, -2^-126 to -2^127, +0 and 2^-126 to 2^127. Some examples are 0.125, 0.25, 0.5, 1.0, 2.0, 4.0 and the negative numbers like -2.0, -1.0, -0.5.

The number 1.0 is a good example to explain details of the IEEE754 32bit format. The improper fraction part uses 24 bits for an unsigned Q1.23 number. That is one binary bit, the leading bit, before the binary point and 23 bits after the binary point. The leading bit of a "normalized" IEEE754 fraction always has the value 1. The leading bit is "implicit" or not stored in the IEEE754 format, you have to add it if you need. The "true" binary exponent of 1.0 is 2^0. IEEE754 uses biased exponent with a bias value of 127 for single format, not 2ers complement. You add 127 to convert from "true" binary exponent to biased exponent. Likewise, you subtract 127 to convert from biased exponent to "true" exponent. It is easier to compare fp numbers that use biased exponent and use the order sign, exponent, fraction. You can use signed 32bit integer compare, you need no fp compare. The 23 bits fraction can hold 2^23 = 8.388.608 different values. But 7 decimal digits make 10^7 = 10.000.000 different values. Therefore, adjacent numbers in radix 10 can map to the same radix 2 number. The ASCII string "5.05", after conversion to floating point and back, will become the ASCII string "5.049999".

There are "best practices" for an integer package, which I used in int.asm. With a fp package you have more alternatives. Let's start with radix conversion, that is the conversion between human readable ASCII string in decimal (Radix 10) system to and from computer internal binary (Radix 2) system. Donald Knuth writes in "The Art of Computer programming, Volume 2 Seminumerical Algorithms, chapter 4.4 Radix conversion" about "The four basic methods" for radix conversion. The subchapter "Converting fractions" tells us "No equally fast method of converting fractions manually is known. The best way seems to be Method 2a \[Multiplication by B using radix-b arithmetric\]". The subchapter "Floating point conversion" tells the complete story of radix conversion: "it is necessary to deal with both the exponent and the fraction parts simultaneously, since conversion of the exponent will affect the fraction part".

The fraction program can convert fp numbers between -10 and -1 and 1 to 10 from decimal to fp and vice versa without need "to deal with both the exponent and the fraction parts simultaneously". As you see above, the decimal number 1.0 is stored as IEEE754 number 1.0 * 2^0 and 9.999999 is stored as 1.249999 * 2^3. The ASCII (decimal) to improper fraction function produces a "denormalized" fraction. The radix 10 to radix 2 conversion constants are defined for an exponent of 2^0 or 10^0. To store the denormalized fraction, we need at least 27 bits. We use 32 bits. The additional 5 bits are used as "rounding" bits. To "normalize" the fraction, we shift the fraction to the right or to the left, until the integer part is 1. For every successful right shift, we increase the exponent by one.

The following table shows the quality of my decimal-to-fp function dcm2fpu() and fp-to-decimal function fpu2dcm(). The "to ASCII" column shows the result of a decimal to fp to decimal again conversion. The "IEEE754" column uses the conversion that is part of printf() as reference. The columns s, explicit, fraction and 2^ show the internal fp format with 2ers complement exponent and explicit leading bit fraction. The floating point zero need to be written as 0.e0, because my ASCII parser is primitive.
```
from ASCII= s explicit fraction 2^   =   to ASCII   =  IEEE754
1.        = 0 0x800000 1.000000    0 =     1.000000 =  1.000000
1.1       = 0 0x8CCCCD 1.100000    0 =     1.100000 =  1.100000
1.01      = 0 0x8147AE 1.010000    0 =     1.010000 =  1.010000
1.001     = 0 0x8020C5 1.001000    0 =     1.001000 =  1.001000
1.0001    = 0 0x800347 1.000100    0 =     1.000100 =  1.000100
1.00001   = 0 0x800054 1.000010    0 =     1.000010 =  1.000010
1.000001  = 0 0x800008 1.000001    0 =     1.000001 =  1.000001
5.        = 0 0xA00000 1.250000    2 =     5.000000 =  5.000000
5.5       = 0 0xB00000 1.375000    2 =     5.500000 =  5.500000
5.05      = 0 0xA19999 1.262500    2 =     5.049999 =  5.050000
5.005     = 0 0xA028F5 1.251250    2 =     5.004999 =  5.005000
5.0005    = 0 0xA00418 1.250125    2 =     5.000499 =  5.000500
5.00005   = 0 0xA00068 1.250012    2 =     5.000049 =  5.000050
5.000005  = 0 0xA0000A 1.250001    2 =     5.000004 =  5.000005
9.        = 0 0x900000 1.125000    3 =     9.000000 =  9.000000
9.9       = 0 0x9E6666 1.237500    3 =     9.899999 =  9.900000
9.09      = 0 0x9170A3 1.136250    3 =     9.089999 =  9.089999
9.009     = 0 0x9024DD 1.126125    3 =     9.008999 =  9.009000
9.0009    = 0 0x9003AF 1.125112    3 =     9.000899 =  9.000899
9.00009   = 0 0x90005E 1.125011    3 =     9.000089 =  9.000090
9.000009  = 0 0x900009 1.125001    3 =     9.000008 =  9.000009
9.999999  = 0 0x9FFFFF 1.250000    3 =     9.999999 =  9.999999
-1.000001 = 1 0x800008 1.000001    0 =    -1.000001 = -1.000001
-5.000005 = 1 0xA0000A 1.250001    2 =    -5.000004 = -5.000005
-9.000009 = 1 0x900009 1.125001    3 =    -9.000008 = -9.000009
-9.999999 = 1 0x9FFFFF 1.250000    3 =    -9.999999 = -9.999999
0.e0      = 0 0x000000 0.000000 -127 =         0.e0 =  0.000000
-0.e0     = 1 0x000000 0.000000 -127 =        -0.e0 = -0.000000
```
You find the fraction.cpp source code in folder fraction, ready to compile with Microsoft Visual Studio. But the source code should compile and run on Linux (very good OS) and MacOS (just another UNIX), too. The program fpconsts.cpp creates the constants that fraction.cpp needs. The c2 constants array has a size of 256Bytes - the size of a 6502 index register - and helps to convert radix 10 numbers in the range from 10^-31 to 9.999999 * 10^32 into radix 2 numbers.

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
The following table shows ASCII (decimal) conversion for fraction and exponent from radix 10 to radix 2. The fraction and exponent conversion constants are only correct for one exponent value. Therefore we have to do the "fraction correction" multiplication with at least 27 bits before we can do normalization from 27 bits to 24 bits. The notation 0.8e1 has the same precision than the notation 8.0e0. Both notations represent decimal 8. Note: 0.8e1 is no official ASCII representation of a floating point number.
```
 from ASCII = 10^ ndx= explicit fraction 2^   =   IEEE754
1.e-31      = -31  0 = 0x81CEB3 1.014120 -103 = 1.000000e-31
1.e-5       =  -5 26 = 0xA7C5AC 1.310720  -17 = 1.000000e-05
1.e-4       =  -4 27 = 0xD1B718 1.638400  -14 = 1.000000e-04
1.e-3       =  -3 28 = 0x83126F 1.024000  -10 = 1.000000e-03
1.e-2       =  -2 29 = 0xA3D70A 1.280000   -7 = 1.000000e-02
1.e-1       =  -1 30 = 0xCCCCCD 1.600000   -4 = 1.000000e-01
1.e0        =   0 31 = 0x800000 1.000000    0 = 1.000000e+00
8.e0        =   0 31 = 0x800000 1.000000    3 = 8.000000e+00
0.8e1       =   1 32 = 0x800000 1.000000    3 = 8.000000e+00
1.e1        =   1 32 = 0xA00000 1.250000    3 = 1.000000e+01
1.e2        =   2 33 = 0xC80001 1.562500    6 = 1.000000e+02
1.e3        =   3 34 = 0xFA0001 1.953125    9 = 1.000000e+03
1.e4        =   4 35 = 0x9C4000 1.220703   13 = 1.000000e+04
1.e5        =   5 36 = 0xC35001 1.525879   16 = 1.000000e+05
1.e32       =  32 63 = 0x9DC5AE 1.232595  106 = 1.000000e+32
5.e-3       =  -3 28 = 0xA3D70A 1.280000   -8 = 5.000000e-03
5.5e-2      =  -2 29 = 0xE147AE 1.760000   -5 = 5.500000e-02
5.05e-1     =  -1 30 = 0x8147AE 1.010000   -1 = 5.050000e-01
5.005e0     =   0 31 = 0xA028F5 1.251250    2 = 5.005000e+00
5.0005e1    =   1 32 = 0xC8051E 1.562656    5 = 5.000500e+01
5.00005e2   =   2 33 = 0xFA00A4 1.953145    8 = 5.000050e+02
5.000005e3  =   3 34 = 0x9C400A 1.220704   12 = 5.000005e+03
```
The next table shows radix 2 to radix 10 conversion. The radix 2 exponent 2^3 can go with a fraction range from 1.0 to 1.999999. The radix 10 representation of this range is 8.0 * 10^0 to 1.599999 * 10^1. There is some tricky source code in function fpu2dcm() that performs a multiplication by 10 or a division by 10 on the ASCII representation to avoid ASCII representations 00.XXXXXXX (two leading zeros) or XX.XXXXX (two integer digits).
```
   IEEE754    = fraction  2^   =   to ASCII
 1.000000e-31 = 1.0141205 -103 = 1.000000e-31
 7.812500e-03 = 1.0000000   -7 = 7.812500e-3
 1.562500e-02 = 1.0000000   -6 = 1.562500e-2
 3.125000e-02 = 1.0000000   -5 = 3.125000e-2
 6.250000e-02 = 1.0000000   -4 = 6.250000e-2
 1.250000e-01 = 1.0000000   -3 = 1.250000e-1
 1.000000e-01 = 1.6000000   -4 = 1.000000e-1
 2.500000e-01 = 1.0000000   -2 = 2.500000e-1
 5.000000e-01 = 1.0000000   -1 = 5.000000e-1
 1.000000e+00 = 1.0000000    0 = 1.000000
 2.000000e+00 = 1.0000000    1 = 2.000000
 4.000000e+00 = 1.0000000    2 = 4.000000
 8.000000e+00 = 1.0000000    3 = 8.000000
 1.600000e+01 = 1.0000000    4 = 1.600000e1
 3.200000e+01 = 1.0000000    5 = 3.200000e1
 6.400000e+01 = 1.0000000    6 = 6.400000e1
 1.280000e+02 = 1.0000000    7 = 1.280000e2
 1.000000e+32 = 1.2325952  106 = 1.000000e32
 9.999999e+32 = 1.5407438  109 = 9.999999e32
```
Another tricky detail is the "pseudo radix 16" constants array c4. There are only entries for exponents 2^(4\*x), like 2^-3, 2^0, 2^4. The Q4.28 conversion variable allows the necessary shift of the Q1.23 fraction to match the floating point radix 2 exponent to the available entries in the constants array. Note: The IBM 360 had radix 16 exponents. This solution resulted in up to 3Bits loss of fraction precision. My pseudo radix 16 solution has not this disadvantage!

The purpose of the constants arrays c1 to c5 are:
```
c1: Q constants 10.0, 1.0, ... 0.0000001 for fraction conversion
c2: Q fraction constants to convert exponent 10^x to exponent 2^y
c3: integer exponent constants, radix 10 to radix 2, e.g. 10^0 converts to 2^0 .. 2^3
c4: Q fraction constants to convert exponent 2^x to exponent 10^y
c5: integer exponent constants, radix 2 to radix 10, e.g. 2^3 converts to 10^0 .. 10^1
```
In the last table (for now) you can see that a given radix 2 exponent can have two different radix 10 exponents. A given radix 10 exponent can have up to four different radix 2 exponents. Furthermore you see the "Matula condition" in action. We need 8 decimal digits to convert an IEEE754 single format number to ASCII and back to IEEE754 without loss. See CACM, January 1968, David W. Matula, In-and-Out Conversions. The IEEE754 fraction precision is only 6,9236.. decimal digits.
```
 IEEE754   = fraction  2^   =   to ASCII   =  IEEE754
0x3D000000 = 1.0000000   -5 = 3.1250000e-2 = 0x3D000000
0x3D7FFFFF = 1.9999999   -5 = 6.2499996e-2 = 0x3D7FFFFF
0x3D800000 = 1.0000000   -4 = 6.2500003e-2 = 0x3D800000
0x3DFFFFFF = 1.9999999   -4 = 1.2499999e-1 = 0x3DFFFFFF
0x3E000000 = 1.0000000   -3 = 1.2500000e-1 = 0x3E000000
0x3E7FFFFF = 1.9999999   -3 = 2.4999998e-1 = 0x3E7FFFFF
0x3E800000 = 1.0000000   -2 = 2.5000000e-1 = 0x3E800000
0x3EFFFFFF = 1.9999999   -2 = 4.9999997e-1 = 0x3EFFFFFF
0x3F000000 = 1.0000000   -1 = 5.0000000e-1 = 0x3F000000
0x3F7FFFFF = 1.9999999   -1 = 9.9999994e-1 = 0x3F7FFFFF
0x3F800000 = 1.0000000    0 = 1.0000000    = 0x3F800000
0x3FFFFFFF = 1.9999999    0 = 1.9999999    = 0x3FFFFFFF
0x40000000 = 1.0000000    1 = 2.0000000    = 0x40000000
0x407FFFFF = 1.9999999    1 = 3.9999998    = 0x407FFFFF
0x40800000 = 1.0000000    2 = 4.0000000    = 0x40800000
0x40FFFFFF = 1.9999999    2 = 7.9999995    = 0x40FFFFFF
0x41000000 = 1.0000000    3 = 8.0000000    = 0x41000000
0x417FFFFF = 1.9999999    3 = 1.5999999e1  = 0x417FFFFF
0x41800000 = 1.0000000    4 = 1.6000000e1  = 0x41800000
0x41FFFFFF = 1.9999999    4 = 3.1999999e1  = 0x41FFFFFF
```
My fp package does not implement the full exponent range from 10^-38 to 10^38, neither IEEE754 NaN (not a number), infinite and denormalized numbers (numbers between 0 and 2^-126). As Donald Knuth told us, radix conversion is tricky. I seldom need "The Art of computer programming" for my daily work. The actual fp calculations need care, too. But this is another story.

Right now, there is only a C++ implementation of the radix conversion algorithms. As always it is good to make the work on the highest possible level of abstraction. I wrote "6502" compatible C++. I use subroutines for 32Bits addition, subtraction and multiplication. I replaced necessary divisions by repeated subtractions. Transcribing the C++ source code into 6502 assembler should be easy.
