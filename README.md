# 65C02-ICP
65C02 single board computer with in-circuit-programming

The 65C02-ICP is my little 65C02 computer. I have 32kByte RAM, 8kByte EEPROM, a (buggy) WDC 65C51N ACIA RS232 interface and 7.3728MHz clock. Like [Ben Eater](https://eater.net/6502) I use a single 74HC00 for glue logic. And I have in-circuit-programming. This is the big difference to other 6502 single board computers. I need no EEPROM programmer. I have two CP2102 USB-to-UART bridges. One I use to program the EEPROM in-circuit, the other I use to communicate between my MS-Windows computer via [TeraTerm](https://ttssh2.osdn.jp/index.html.en) terminal emulator program and my 65C02 with 38400bps.

My monitor program - mon7.asm - uses RTS/CTS hardware handshake, interrupt driven receive and a 256 Byte receive buffer to work around the 65C51N TDRE (transmit data register empty) bug. I assume my monitor is compatible to other 6551. The monitor has three commands. Download bytes into memory with the : command, examine memory with the p command and execute a program with the r command. If the program executes a BRK opcode, the monitor prints a trace. That is the contents of the Y, X, A, P registers and the PC plus two more bytes on the stack. The monitor machine code length is 320Bytes. The mon7 api is:
```
monbyte = $0    ; 1 byte used by getbyte/putbyte
monword = $1    ; 2 bytes used by getword/putword
monwrptr= $3    ; 2 bytes buf write pointer used by getc
monrdptr= $5    ; 2 bytes buf read pointer used by getc
monbfcnt= $7    ; 1 byte buf count used by getc
montmpy = $8    ; 1 byte tmp used by getc
nmihook = $7EFD ; 3 bytes used by mon
monbuf  = $7F00 ; 256 bytes buf used by getc
getc    = $FF7E ; jsr console (65C51) input, out: a=char
echoc   = $FF81 ; jsr console input with echo, out: a=char
putc    = $FF84 ; jsr console output, in: a=char
asc2hex = $FF96 ; jsr convert ASCII to hex, in: a=ascii, out: a=hex
hex2asc = $FFA6 ; jsr convert hex to ASCII, in: a=hex, return a=ascii
getbyte = $FFB0 ; jsr input 2 Hex digits in ASCII, out: a=byte
putbyte = $FFC5 ; jsr output 2 Hex digits as ASCII, in: a=byte
getword = $FFDB ; jsr input 4 Hex digits in ASCII, out: monword=word
putword = $FFE6 ; jsr output 4 Hex digits as ASCII, in: monword=word
putcrlf = $FFF0 ; jsr output Carrige Return, Line Feed
```
The second program is int.asm. This is a 16Bit integer package with some basic ASCIIZ string subroutines. For subroutine arguments (parameters) I use register A=low byte, X=high byte. Or you can place the parameters into "well known" zero page variables and call the subroutines. The 16Bit integer package is fast. I use the fastest multiplication and division subroutines from the net and I use my own fast integer to decimal conversation subroutine. The same int2dec idea I used in the 1980s for my Z80 32Bit integer package. The int api is:
```
intsign = $9   ; 1 byte used by dec2int, intdiv, i32div
intarg  = $A   ; 2 bytes 16bit argument
intacc  = $C   ; 4 bytes 32bit accumulator (mul, div need 32bit)
getsa   = $FCC0 ; in: console, out: x=high, a=low ASCII pointer len<256
gets    = $FCC4 ; in: console, out: strptr ASCIIZ pointer len<256
putsa   = $FCD5 ; in: x=high, a=low ASCIIZ pointer len<256, out: console
puts    = $FCD9 ; in: strptr ASCIIZ pointer len<256, out: console
dec2inta= $FCE6 ; signed/unsigned decimal ASCII to 16bit signed/unsigned in: x=high, a=low ASCII ptr len<256, out: intacc
dec2int = $FCEA ; in: strptr ASCIIZ pointer len<256, out: intacc
int2deca= $FD2B ; signed 16bit to signed decimal ASCII in: intacc, out: x=high, a=low ASCIIZ ptr len<256
int2dec = $FD2F ; in: intacc, out: strptr ASCIIZ pointer len<256
uns2deca= $FD42 ; unsigned 16bit to unsigned decimal ASCII in: intacc, out: x=high, a=low ASCIIZ ptr len<256
uns2dec = $FD46 ; in: intacc out: strptr ASCIIZ pointer len<256
intmula = $FDEB ; signed multiplication in: x=high, a=low value, intarg, out: 32bit (intacc, resulth)
unsmula = $FDEB ; unsigned multiplication in: x=high, a=low value, intarg, out: 32bit (intacc, resulth)
intmul  = $FDEF ; in: intacc, intarg out: 32bit (intacc, resulth)
unsmul  = $FDEF ; in: intacc, intarg out: 32bit (intacc, resulth)
intdiva = $FE49 ; signed division 16bit/16bit in: x=high, a=low value, intarg, out: intacc=acc/arg, remaindr=acc%arg
intdiv  = $FE4D ; in: intacc, intarg, out: intacc=acc/arg, remaindr=acc%arg
i32div  = $FE6C ; signed division 32bit/16bit in: 32bit (intacc, remaindr), intarg, out: intacc=acc/arg, remaindr=acc%arg
unsdiva = $FE86 ; unsigned division 16bit/16bit in: x=high, a=low value, intarg, out: intacc=acc/arg, remaindr=acc%arg
unsdiv  = $FE8A ; in: intacc, intarg, out: intacc=acc/arg, remaindr=acc%arg
u32div  = $FE90 ; unsigned division 32bit/16bit in: 32bit (intacc, remaindr), intarg, out: intacc=acc/arg, remaindr=acc%arg
```
The int machine code length is 512Bytes. There are no addition or subtraction subroutines. You write these better
as inline. My int package can do scaling operation. That is a multiplication followed by a division
with a 32Bit intermediate result. The Forth programming language has the scaling command */.

Last but not least is int_t.asm, a test program for the int package. You normally execute int_t in RAM. Just drop the assembler output with ALT-V into the TeraTerm window and mon7 will store the bytes into RAM. A typical output of the int_t program is:
```
\0800: A9 0C A2 0A 20 D5 FC A9
\0808: 04 A2 02 20 C0 FC A9 AD
... many more lines
\0BA8: 65 6C 20 52 3A 20 52 65
\0BB0: 73 75 6C 74 3A 20 00
\0800r

puts: int version 20220823
65C02 wisdom of the day:
RAM needs the phi0 clock,
65C51 needs the phi2 clock
and the EEPROM does not care.
gets: Enter text: Hello 65C02-ICP
gets: Your text: Hello 65C02-ICP
int2dec: $8000 -32768
int2dec: $0000 0
int2dec: $7530 30000
int2dec: $FFFF -1
uns2dec: $8000 32768
uns2dec: $FFFF 65535
dec2int: Enter number: -123
int2dec: Your number: -123
dec2int: Enter number: -123
uns2dec: Your number: 65413
intmul: Enter number: 300
intmul: Enter number: -100
intmul: Result: $012B8AD0 -30000
intdiv: Enter number: 30000
intdiv: Enter number: 17
intdiv: Result: 1764 12
R parallel R: Enter number: 32767
R parallel R: Enter number: 32768
R parallel R: Result: 16383

! 05 F9 0A 31 9F 09 FF 7F
```
The first lines show the machine code download. I use the hex format that the [virtual 6502 assembler](https://www.masswerk.at/6502/assembler.html) produces. The 0800r command starts program execution at RAM memory location $0800 (800 hexadecimal). The program ends in a BRK. The trace output starts with a !.

The R parallel R test section uses the scaling operation. Without it, the maximum numbers are 255 and 256.

I draw the 65C02-ICP schematics with KiCad. At the moment there is only a breadboard build. The main components are:
- 7.3728MHz or 1.8432MHz clock oscillator
- W65C02 14MHz CPU
- W65C51N 14 MHz ACIA
- 28C64 8KByte EEPROM
- 62C256 32KByte static RAM
- 74HC00 glue logic
- 74HCT04, 74HC123, 3 x 74HC595 for in-circuit-programming
- 2 x CP2102 modules for USB connection

The in-circuit-programming part needs some hardware on the target computer - the 65C02 - and some software on the MS-Windows development computer. The EEPROM programmer software - named usb.exe - is in directory usb. There are the minimum files to recreate a Visual Studio project. Before you start Visual Studio, you have to get some files from Silabs, the CP2102 manufacturer. You need files SiUSBXp.h and SiUSBXp.lib in directoty usb/usb and file SiUSBXp.dll in directory usb/x64/Release. You find these files in the [USBXpress SDK v6.7.4 11/24/2021](https://www.silabs.com/developers/direct-access-drivers). You have to compile "Release" version, not "Debug" version, because the Silabs files need this. Again I use the machine code or object code format of virtual 6502 assembler. I copy the object code from virtual 6502 assembler to a hex file, e.g. mon7.hex for the monitor program. Then I call the EEPROM programmer program:
```
C:\src\usb\usb\x64\Release>usb mon7.hex
CP2102 EEPROMer
usage: usb [[filename] singlestep]
USBX Devices found = 1
USBX Device = 0 name = 0001
prog time = 5360 ms
```
The programmer needs 5 seconds to program 320bytes. This is slow, but hey, what to you expect from my low-cost hardware solution? How does the programmer work? I use only two RS232 symbols. One symbol is all zeros, the other is all ones. The 74HC123 monoflop produces out of the RS232 start bit a rising edge at data bit 3 location. At that time the RS232 signal is either 0 or 1. I use this rising edge as SPI clock signal SCK and the RS232 TxD signal from the CP2102 USB-to-UART bridge as SPI MOSI signal to shift 24 bits into 3 74HC595 shift registers. After the bits are "loaded", the USB-to-UART bridge uses a /DTR pulse to operate the EEPROM /WE pin. Because real-time performance of MS-Windows is poor, this pulse is 3ms (milliseconds) long - even if the CPU is 4GHz fast. The 24 bits set address bus A0 to A15 and data bus D0 to D7. While the in-circuit-programming hardware is active, the CPU is not active and vice versa. A simple on-off switch does the trick.

By the way, for EEPROM programming, the CP2102 has to work as USBXpress device. I use [CP210x Legacy Programming Utilities](https://community.silabs.com/s/article/cp210x-legacy-programming-utilities?language=en_US) program CP210xSetIDs to change the USB ProductID (PID) to 0xEA61. See [AN169 USBXpress Programmer's Guide](https://www.silabs.com/documents/public/application-notes/AN169.pdf) for details.
