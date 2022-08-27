In my Z80 dayes, I wrote a 32bit integer package and a 32bit [CORDIC package](http://www.andreadrian.de/oldcpu/Z80_number_cruncher.html). Now, I want to write a IEEE754 compatible floating point (fp) package for the 6502. There are some 6502 fp packages. First the Steve Wozniak/Roy Rankin package that was published in Dr.Dobb's Journal in August 1976. Second the fp package in Microsoft BASIC for Apple 2, Commodore PET and others.

Since 1985 IEEE754, the "IEEE Standard for Binary Floating-Point Arithmetic" is used for floating point. The Intel 8087 co-processor was one of the first implementation of IEEE754 in hardware. For me, IEEE754 is a very compatible way to exchange fp numbers between different computers. My fp package uses next to IEEE754 format an internal fp format. Every IEEE754 fp number has thre parts:
- a one bit sign
- a biased exponent
- a unsigned fraction

Here are some IEEE754 single (32bit) numbers as printed by fraction.cpp:
```
 IEEE754    = as bits    = s exp fraction
 0.00000000 = 0x00000000 = 0   0      0
-0.00000000 = 0x80000000 = 1   0      0
 0.06250000 = 0x3D800000 = 0 123      0
 0.12500000 = 0x3E000000 = 0 124      0
 0.25000000 = 0x3E800000 = 0 125      0
 0.50000000 = 0x3F000000 = 0 126      0
 0.75000000 = 0x3F400000 = 0 126 400000
 0.99999994 = 0x3F7FFFFF = 0 126 7FFFFF
 1.00000000 = 0x3F800000 = 0 127      0
-1.00000000 = 0xBF800000 = 1 127      0
 1.50000000 = 0x3FC00000 = 0 127 400000
 1.99999988 = 0x3FFFFFFF = 0 127 7FFFFF
```
Column s shows the sign, exp shows the biased exponent in decimal and fraction shows the unsigned fraction in hexadecimal. Did you know that the IEEE754 format has a positive and a negative zero? And that you can express 510 different valid numbers that all have a fraction value of 0? These numbers are -0, -2^-126 to -2^127, +0 and 2^-126 to 2^127. Some examples are 0.0625, 0.125, 0.25, 0.5, 1 and -1.0.

The number 1.0 is a good example to explain details of the IEEE754 32bit format. The "fraction" part uses 24 bits for an unsigned q 1.23 number. That is one binary bit, the leading bit, before the binary point and 23 bits after the binary point. The leading bit of a "normalized" IEEE754 fraction always has the value 1. The leading bit is "implicit" or not written down in the IEEE754 format, you have to add it if you need. The "true" exponent of 1.0 is 2^0 or 10^0, whatever you like. IEEE754 uses biased exponent with a bias value of 127, not 2ers complement. It is easier to compare fp numbers that use biased exponent and use the order sign, exponent, fraction. You can use integer compare, you need no fp compare.

There are "best practices" for an integer package, which I used in int.asm. With a fp package you have more alternatives. Let's start with radix conversion, that is the conversion between human readable ASCII string in decimal (Radix 10) system to and from computer internal binary (Radix 2) system. Donald Knuth writes in "The Art of Computer programming, Volume 2 Seminumerical Algorithms, chapter 4.4 Radix conversion" about "The four basic methods" for radix conversion. The subchapter "Converting fractions" tells us "No equally fast method of converting fractions manually is known. The best way seems to be Method 2a \[Multiplication by B using radix-b arithmetric\]". The subchapter "Floating point conversion" tells the complete story of radix conversion: "it is necessary to deal with both the exponent and the fraction parts simultaneously, since conversion of the exponent will affect the fraction part".

The fraction program can only convert fp numbers between -1 and -0.1 and +0.1 to 1 from decimal to fp and vice versa. That is, I avoid the "correction multiplications". Example: The exponent 2^10 is 1.024 * 10^3. The exponent 10^3 is 0.9765625 * 2^10. The fraction "correction" constants are 0.9765625 to convert from decimal to binary and 1.024 to convert from binary to decimal.

The following table shows the quality of my decimal-to-fp function dcm2fpu() and fp-to-decimal function fpu2dcm(). The "to string" column shows the result of a decimal to fp to decimal again conversion. The "IEEE754" column uses the conversion that is part of printf() as reference. The columns s, exp, fraction show the internal fp format with 2ers complement exponent and explicit leading bit fraction.
```
from string = s exp fraction = to string  =  IEEE754
0.1         = 0   -4 CCCCD0 =  0.10000002 =  0.10000002
0.11        = 0   -4 E147B2 =  0.11000003 =  0.11000003
0.101       = 0   -4 CED91A =  0.10100002 =  0.10100003
0.1001      = 0   -4 CD013E =  0.10010002 =  0.10010003
0.10001     = 0   -4 CCD20F =  0.10001003 =  0.10001003
0.100001    = 0   -4 CCCD57 =  0.10000103 =  0.10000103
0.1000001   = 0   -4 CCCCDE =  0.10000013 =  0.10000013
0.10000001  = 0   -4 CCCCD2 =  0.10000004 =  0.10000004
0.5         = 0   -1 800000 =  0.50000002 =  0.50000000
0.55        = 0   -1 8CCCCD =  0.55000004 =  0.55000001
0.505       = 0   -1 8147AE =  0.50500002 =  0.50500000
0.5005      = 0   -1 8020C5 =  0.50050005 =  0.50050002
0.50005     = 0   -1 800347 =  0.50005003 =  0.50005001
0.500005    = 0   -1 800054 =  0.50000503 =  0.50000501
0.5000005   = 0   -1 800008 =  0.50000050 =  0.50000048
0.50000005  = 0   -1 800001 =  0.50000008 =  0.50000006
0.9         = 0   -1 E66666 =  0.90000000 =  0.89999998
0.99        = 0   -1 FD70A4 =  0.99000003 =  0.99000001
0.909       = 0   -1 E8B439 =  0.90900000 =  0.90899998
0.9009      = 0   -1 E6A162 =  0.90090003 =  0.90090001
0.90009     = 0   -1 E66C4C =  0.90009000 =  0.90008998
0.900009    = 0   -1 E666FD =  0.90000900 =  0.90000898
0.9000009   = 0   -1 E66675 =  0.90000090 =  0.90000087
0.90000009  = 0   -1 E66668 =  0.90000012 =  0.90000010
0.99999995  = 0   -1 FFFFFF =  0.99999996 =  0.99999994
-0.10000001 = 1   -4 CCCCD2 = -0.10000004 = -0.10000004
-0.50000005 = 1   -1 800001 = -0.50000008 = -0.50000006
-0.90000009 = 1   -1 E66668 = -0.90000012 = -0.90000010
-0.99999995 = 1   -1 FFFFFF = -0.99999996 = -0.99999994
```
You find the fraction.cpp source code in folder fraction, ready to compile with Microsoft Visual Studio. But the source code should compile and run on Linux (very good OS) and MacOS (just another UNIX), too.

The following table looks into the "correction" constants. I use the engineering scaling of exponents, that is steps of 3, like 10^-12, 10^-9, 10^-3, 10^0, 10^3, 10^6, 10^9, 10^12. The first column shows the IEEE754 number as converted by printf(). The second column shows the IEEE754 parts sign, biased exponent and fraction with implicit leading bit. The third column shows the fraction as decimal fraction and the "true" exponent for base (radix) 2 in the normal IEEE754 notation with 1.0 <= fraction < 2.0. The last column shows the fraction with 0.5 <= fraction < 1.0. I use this interpretation of IEEE754, because it makes floating point conversion from ASCII to binary easier.
```
    IEEE754   = s exp fract. = fraction  2^   = fraction  2^
 1.000000e-12 = 0  87  CBCCC = 1.0995116  -40 = 0.5497558  -39
 1.000000e-09 = 0  97  9705F = 1.0737418  -30 = 0.5368709  -29
 1.000000e-06 = 0 107  637BD = 1.0485760  -20 = 0.5242880  -19
 1.000000e-03 = 0 117  3126F = 1.0240000  -10 = 0.5120000   -9
 1.000000e+00 = 0 127      0 = 1.0000000    0 = 0.5000000    1
 1.000000e+03 = 0 136 7A0000 = 1.9531250    9 = 0.9765625   10
 1.000000e+06 = 0 146 742400 = 1.9073486   19 = 0.9536743   20
 1.000000e+09 = 0 156 6E6B28 = 1.8626451   29 = 0.9313226   30
 1.000000e+12 = 0 166 68D4A5 = 1.8189894   39 = 0.9094947   40
```
To be continued ...
