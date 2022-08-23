; int_t.asm
; 6502 16bit integer package tests
; (C) 2022 Andre Adrian
; Assembler:
; https://www.masswerk.at/6502/assembler.html

; 2022-08-22 first version
; 2022-08-23 no gap between mon zp and int zp variables

; import mon7 9600bps/38400bps, RTS/CTS, 256bytes rx buf
  monbyte = $0    ; 1 byte used by getbyte/putbyte
  monword = $1    ; 2 bytes used by getword/putword
  monwrptr= $3    ; 2 bytes buf write pointer used by getc
  monrdptr= $5    ; 2 bytes buf read pointer used by getc
  monbfcnt= $7    ; 1 byte buf count used by getc
  montmpy = $8    ; 1 byte tmp used by getc
  nmihook = $7EFD ; 3 bytes used by mon
  monbuf  = $7F00 ; 256 bytes buf used by getc
  getc    = $FF7E ; jsr acia input, return a=char
  echoc   = $FF81 ; jsr acia input with echo, return a=char
  putc    = $FF84 ; jsr acia output, a=char
  asc2hex = $FF96 ; jsr convert ASCII to hex, a=ascii, return a=hex
  hex2asc = $FFA6 ; jsr convert hex to ASCII, a=hex, return a=ascii
  getbyte = $FFB0 ; jsr input 2 Hex digits in ASCII, return a=byte
  putbyte = $FFC5 ; jsr output 2 Hex digits as ASCII, a=byte
  getword = $FFDB ; jsr input 4 Hex digits in ASCII, return monword=word
  putword = $FFE6 ; jsr output 4 Hex digits as ASCII, monword=word
  putcrlf = $FFF0 ; jsr output Carrige Return, Line Feed

; int
  intsign = $9    ; 1 byte used by dec2int, intdiv, i32div
  intarg  = $A    ; 2 bytes 16bit argument
  intacc  = $C    ; 4 bytes 32bit accumulator (mul, div need 32bit)
  getsa   = $FCC0 ; in: console, out: x=high, a=low ASCII pointer len<256
  gets    = $FCC4 ; in: console, out: strptr ASCIIZ pointer len<256
  putsa   = $FCD5 ; in: x=high, a=low ASCIIZ pointer len<256, out: console
  puts    = $FCD9 ; in: strptr ASCIIZ pointer len<256, out: console
  ; signed/unsigned decimal ASCII to 16bit signed/unsigned
  dec2inta= $FCE6 ; in: x=high, a=low ASCII pointer len<256, out: intacc
  dec2int = $FCEA ; in: strptr ASCIIZ pointer len<256, out: intacc
  ; signed 16bit to signed decimal ASCII
  int2deca= $FD2B ; in: intacc, out: x=high, a=low ASCIIZ pointer len<256
  int2dec = $FD2F ; in: intacc, out: strptr ASCIIZ pointer len<256
  ; unsigned 16bit to unsigned decimal ASCII
  uns2deca= $FD42 ; in: intacc, out: x=high, a=low ASCIIZ pointer len<256
  uns2dec = $FD46 ; in: intacc out: strptr ASCIIZ pointer len<256
  intmula = $FDEB ; in: x=high, a=low value, intarg, out: 32bit (intacc, resulth)
  unsmula = $FDEB ; in: x=high, a=low value, intarg, out: 32bit (intacc, resulth)
  intmul  = $FDEF ; in: intacc, intarg out: 32bit (intacc, resulth)
  unsmul  = $FDEF ; in: intacc, intarg out: 32bit (intacc, resulth)
  intdiva = $FE49 ; in: x=high, a=low value, intarg, out: intacc=acc/arg, remaindr=acc%arg
  intdiv  = $FE4D ; in: intacc, intarg, out: intacc=acc/arg, remaindr=acc%arg
  i32div  = $FE6C ; in: 32bit (intacc, remaindr), intarg, out: intacc=acc/arg, remaindr=acc%arg
  unsdiva = $FE86 ; in: x=high, a=low value, intarg, out: intacc=acc/arg, remaindr=acc%arg
  unsdiv  = $FE8A ; in: intacc, intarg, out: intacc=acc/arg, remaindr=acc%arg
  u32div  = $FE90 ; in: 32bit (intacc, remaindr), intarg, out: intacc=acc/arg, remaindr=acc%arg

; mon aliases
  inttmpy = monbyte  ; zp 1 byte used by uns2dec
  strptr  = monword  ; zp 2 bytes used by puts, dec2int, int2dec, uns2dec

; int aliases
  plier   = intacc   ; in
  plicand = intarg   ; in
  dividend= intacc   ; in (denominator)
  divisor = intarg   ; in (numerator)
  result  = intacc   ; out low(plier * plicand) = dividend / divisor
  resulth = intacc+2 ; out high(plier * plicand)
  remaindr= intacc+2 ; out dividend % divisor

; int_t
  RparaRra= $200      ; 2 bytes
  RparaRrb= $202      ; 2 bytes
  strbuf  = $204      ; ram min. 7 bytes ASCIIZ (1 sign, 5 digits, 1 NUL)

; const
  NUL = 0     ; ASCIIZ terminal char
  LF  = $0A   ; ASCII line feed
  CR  = $0D   ; ASCII carrige return
  SPC = $20   ; ASCII space


; Program im RAM
  .org $0800

; ************************************
; tests

main:
;  jmp here
; puts
  lda #<putsm
  ldx #>putsm
  jsr putsa

; gets
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  lda #<getsm
  ldx #>getsm
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr putsa

; int2dec
  lda #<int2decm
  ldx #>int2decm
  jsr putsa
  INT2DCC1 = -32768
  lda #<INT2DCC1
  ldx #>INT2DCC1
  jsr doint2dc

  lda #<int2decm
  ldx #>int2decm
  jsr putsa
  INT2DCC2 = 0
  lda #<INT2DCC2
  ldx #>INT2DCC2
  jsr doint2dc

  lda #<int2decm
  ldx #>int2decm
  jsr putsa
  INT2DCC3 = 30000
  lda #<INT2DCC3
  ldx #>INT2DCC3
  jsr doint2dc

  lda #<int2decm
  ldx #>int2decm
  jsr putsa
  INT2DCC4 = 65535
  lda #<INT2DCC4
  ldx #>INT2DCC4
  jsr doint2dc

; uns2dec
  lda #<uns2decm
  ldx #>uns2decm
  jsr putsa
  UNS2DCC1 = 32768
  lda #<UNS2DCC1
  ldx #>UNS2DCC1
  jsr douns2dc

  lda #<uns2decm
  ldx #>uns2decm
  jsr putsa
  UNS2DCC2 = 65535
  lda #<UNS2DCC2
  ldx #>UNS2DCC2
  jsr douns2dc

; dec2int, int2dec
  lda #<dec2intm
  ldx #>dec2intm
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int
  lda #<dec2inm2
  ldx #>dec2inm2
  jsr putsa
  jsr putint

; dec2int, uns2dec
  lda #<dec2intm
  ldx #>dec2intm
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int
  lda #<dec2inm3
  ldx #>dec2inm3
  jsr putsa
  jsr putuns

; intmul
  lda #<intmulm1
  ldx #>intmulm1
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int
  lda intacc
  ldx intacc+1
  lda intacc+1      ; save first number to stack
  pha
  lda intacc
  pha
  lda #<intmulm1
  ldx #>intmulm1
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int       ; second number to acc
  pla               ; restore first to arg
  sta intarg
  pla
  sta intarg+1
  jsr intmul
  lda #<intmulm2    ; print result
  ldx #>intmulm2
  jsr putsa
  jsr putlong       ; does not change intacc
  jsr putint

; intdiv
  lda #<intdivm1
  ldx #>intdivm1
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int
  lda intacc+1      ; save first number to stack
  pha
  lda intacc
  pha
  lda #<intdivm1
  ldx #>intdivm1
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int       ; second number to acc
  lda intacc+1      ; save second number to stack
  pha
  lda intacc
;  pha
;  pla               ; restore second to arg
  sta intarg
  pla
  sta intarg+1
  pla               ; restore first to acc
  sta intacc
  pla
  sta intacc+1
  jsr intdiv
  lda remaindr+1    ; save remainder
  pha
  lda remaindr
  pha
  lda #<intdivm2    ; print result
  ldx #>intdivm2
  jsr putsa
  jsr putint

  lda #SPC
  jsr putc
  pla               ; restore remainder
  sta intacc
  pla
  sta intacc+1
  jsr putint

; Parallelschaltung Rp = Ra * Rb / (Ra + Rb)
here:
  lda #<RparaRm1
  ldx #>RparaRm1
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int
  lda intacc      ; save first number to mem
  ldx intacc+1
  sta RparaRra
  stx RparaRra+1
  lda #<RparaRm1
  ldx #>RparaRm1
  jsr putsa
  lda #<strbuf
  ldx #>strbuf
  jsr getsa
  jsr dec2int
  lda intacc      ; save second number to mem
  ldx intacc+1
;  sta RparaRrb
;  stx RparaRrb+1
;  lda RparaRrb    ; result = Ra * Rb / (Ra + Rb)
;  ldx RparaRrb+1
  sta intarg      ; arg = Rb
  stx intarg+1
  lda RparaRra
  ldx RparaRra+1
  jsr intmula     ; acc = Ra * Rb
  clc             ; arg = Ra + Rb
  lda RparaRra
  adc intarg
  sta intarg
  lda RparaRra+1
  adc intarg+1
  sta intarg+1
  jsr u32div      ; acc = acc / arg = Ra * Rb / (Ra + Rb)
  lda #<RparaRm2
  ldx #>RparaRm2
  jsr putsa
  jsr putuns

  jsr putcrlf
  brk

doint2dc:
  sta monword
  sta intacc
  stx intacc+1
  stx monword+1
  jsr putword       ; print as hex
  lda #SPC
  jsr putc
  jsr putint
  rts

douns2dc:
  sta monword
  sta intacc
  stx intacc+1
  stx monword+1
  jsr putword       ; print as hex
  lda #SPC
  jsr putc
  lda #<strbuf
  ldx #>strbuf
  jsr uns2deca      ; print as unsigned decimal
  lda #<strbuf
  ldx #>strbuf
  jsr putsa
  rts

putint:
  lda #<strbuf
  ldx #>strbuf
  jsr int2deca      ; attention: - changes strptr
  lda #<strbuf
  ldx #>strbuf
  jsr putsa
  rts

putuns:
  lda #<strbuf
  ldx #>strbuf
  jsr uns2deca
  jsr puts
  rts

putlong:
  lda #'$
  jsr putc
  lda resulth
  ldx resulth+1
  sta monword
  stx monword+1
  jsr putword
  lda intacc
  ldx intacc+1
  sta monword
  stx monword+1
  jsr putword
  lda #SPC
  jsr putc
  rts

putsm:
  .byte CR,LF
  .byte "puts: int version 20220823"
  .byte CR,LF
  .byte "65C02 wisdom of the day:"
  .byte CR,LF
  .byte "RAM needs the phi0 clock,"
  .byte CR,LF
  .byte "65C51 needs the phi2 clock"
  .byte CR,LF
  .byte "and the EEPROM does not care."
  .byte CR,LF
  .byte "gets: Enter text: "
  .byte 0

getsm:
  .byte CR,LF
  .byte "gets: Your text: "
  .byte 0

int2decm:
  .byte CR,LF
  .byte "int2dec: $"
  .byte 0

uns2decm:
  .byte CR,LF
  .byte "uns2dec: $"
  .byte 0

dec2intm:
  .byte CR,LF
  .byte "dec2int: Enter number: "
  .byte 0

dec2inm2:
  .byte CR,LF
  .byte "int2dec: Your number: "
  .byte 0

dec2inm3:
  .byte CR,LF
  .byte "uns2dec: Your number: "
  .byte 0

intmulm1:
  .byte CR,LF
  .byte "intmul: Enter number: "
  .byte 0

intmulm2:
  .byte CR,LF
  .byte "intmul: Result: "
  .byte 0

intdivm1:
  .byte CR,LF
  .byte "intdiv: Enter number: "
  .byte 0

intdivm2:
  .byte CR,LF
  .byte "intdiv: Result: "
  .byte 0

RparaRm1:
  .byte CR,LF
  .byte "R parallel R: Enter number: "
  .byte 0

RparaRm2:
  .byte CR,LF
  .byte "R parallel R: Result: "
  .byte 0

  .end
