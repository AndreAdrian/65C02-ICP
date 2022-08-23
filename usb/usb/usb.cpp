/* usb.cpp
 * (in-circuit) EEPROM programmer with CP2102, version 2022-08-06
 * (C) 2022 Andre Adrian, DL1ADR
 * MS-Windows version using Silicon Labs USBXpress library, because it is faster then VCP
 *
 * 2022-08-06 adr: first version
 * 2022-08-11 adr: main() sscanf_s() better assert message (for now)
 * 
 * The CP2102 USB-UART bridge pins TxD and /DTR are used to create SPI SCLK, SPI MOSI and 
 * EEPROM write strobe signals for the EEPROM programmer.
 * A RS232 symbol has one startbit, 8 databits and one stopbit. As databits values we only
 * use 0x00 and 0xff. Without a RS232 symbol, the CP2102 TxD line is logic 1 (high). 
 * A 0x00 value gives a long logic 0 pulse, a 0xff value gives a short logic 0 pulse.
 * A 74HC123 monoflop differentiates the RS232 symbols into MOSI bits 0 or 1.
 * The falling edge on the /TxD line triggers the monoflop and value sampling happens
 * after 5 bit-times, that is after 43 microseconds for 115200 bps (baud).
 * The /Q monoflop output is used as SPI SCLK. The TxD line is used as SPI MOSI.
 * Three 74HC595 serial-in, parallel-out shift registers provide 24 parallel outputs
 * for the EEPROM address and data pins. The CP2102 /DTR connects to the EEPROM /WE
 * pin. For programming, the EEPROM /CE pin is 0 and the /OE pin is 1.
 * 
 * The /DTR strobe is 3 milliseconds. The EEPROM datasheet specifies a minimum write
 * pulse width of 100 nanoseconds. The length of /DTR strobe is the weak point of this
 * EEPROM programmer.
 * 
 * The CP2102 pins TxD and /RxD are connected. The host computer can read back the
 * transmitted RS232 symbols. This is necessary because SI_Write() is asynchrounous.
 * The call returns before the bytes are transfered "on the wire".
 * 
 * The EEPROM programmer hardware is low-cost: CP2102, 74HCT04, 74HC123, 74HC595. It brings
 * in-circuit-programming to 65C02 or Z80 systems. These CPUs have tri-state outputs
 * and the 74HC595 has tri-state outputs, too. A simple switch selects between run
 * and program mode.
 * 
 * Note: Use the Silabs program CP210xSetIDs.exe to set CP2102 product ID from 0xEA60 to 0xEA61
 * to make it a USBXpress device.
 * see https://community.silabs.com/s/article/cp210x-legacy-programming-utilities?language=en_US
 */

#include <stdio.h>
#undef NDEBUG           // Release needs this for active assert
#include <assert.h>
#include <Windows.h>
#include "SiUSBXp.h"


enum {
    SI_BAUD_921600 = 921600,
    SI_BAUD_460800 = 460800,
    SI_BAUD_230400 = 230400,
    SI_BAUD_115200 = 115200,
    SI_BAUD_76800 = 76800,
    SI_BAUD_57600 = 57600,
    SI_BAUD_38400 = 38400,
    SI_BAUD_19200 = 19200,
    SI_BAUD_9600 = 9600,
    SI_DATABITS_5 = 5,
    SI_DATABITS_6 = 6,
    SI_DATABITS_7 = 7,
    SI_DATABITS_8 = 8,
    SI_STOPBITS_1 = 0,
    SI_STOPBITS_2 = 2,
    SI_PARITY_NONE = 0, 
    SI_PARITY_ODD = 1,
    SI_PARITY_EVEN = 2,
    SI_PARITY_MARK = 3,
    SI_PARITY_SPACE = 4,
    L = 0,
    H = 0xff,

    // program configuration
    SI_BAUD = SI_BAUD_115200,
    SI_STOPBITS = SI_STOPBITS_1,
    SI_DATABITS = SI_DATABITS_8,
    SI_PARITY = SI_PARITY_NONE,
};

void prog(HANDLE handle, char* buf, int len, int singlestep)
{
    // write SPI bits as long/short RS232 symbols
    DWORD bytes;
    SI_STATUS status = SI_Write(handle, buf, len, &bytes, NULL);
    assert(SI_SUCCESS == status && len == bytes && "SI_Write");

    // use read back to ensure that bits are correctly transfered
    char rdbuf[24];
    DWORD rdbytes;
    status = SI_Read(handle, rdbuf, len, &rdbytes, NULL);
    assert(SI_SUCCESS == status && "SI_Read1");
    assert(len == rdbytes && "SI_Read2");
    assert(0 == memcmp(buf, rdbuf, sizeof rdbuf) && "SI_Read3");

    // toggle /WE = /DTR low and high again.
    // /DTR is low for 3 milliseconds.
    status = SI_SetFlowControl(handle, SI_STATUS_INPUT, SI_HELD_INACTIVE, SI_HELD_ACTIVE,
        SI_STATUS_INPUT, SI_STATUS_INPUT, 0);
    assert(SI_SUCCESS == status && "SI_SetFlowControl");
    status = SI_SetFlowControl(handle, SI_STATUS_INPUT, SI_HELD_INACTIVE, SI_HELD_INACTIVE,
        SI_STATUS_INPUT, SI_STATUS_INPUT, 0);
    assert(SI_SUCCESS == status && "SI_SetFlowControl");

    // after /WE the EEPROM needs "Write Cycle Time" of 1ms for internal programming
    Sleep(1);

    if (singlestep) {
        for (int j = 0; j < len; ++j) {
            if (buf[j]) putchar('1');
            else putchar('0');
            if (7 == j || 15 == j) putchar(' ');
        }
        putchar('\n');
        fflush(stdout);
        getchar();
    }
}

int main(int argc, char *argv[])
{
    printf("CP2102 EEPROMer\n");
    printf("usage: usb [[filename] singlestep]\n");
    fflush(stdout);

    FILE* fp = NULL;
    if (argc >= 2) {
        errno_t rv = fopen_s(&fp, argv[1], "rb");
        assert(0 == rv && "fopen_s");
    }
    int singlestep = 0;
    if (argc >= 3) {
        singlestep = 1;
    }

    DWORD devices;
    SI_STATUS status = SI_GetNumDevices(&devices);
    assert (SI_SUCCESS == status && "SI_GetNumDevices");

    printf("USBX Devices found = %ld\n", devices);

    for (DWORD i = 0; i < devices; ++i) {
        SI_DEVICE_STRING    name;
        status = SI_GetProductString(i, name, SI_RETURN_SERIAL_NUMBER);
        assert(SI_SUCCESS == status && "SI_GetProductString");

        printf("USBX Device = %ld name = %s\n", i, name);
    }
    fflush(stdout);

    HANDLE handle;
    status = SI_Open(0, &handle);
    assert(SI_SUCCESS == status && "SI_Open");

    status = SI_SetBaudRate(handle, SI_BAUD);
    assert(SI_SUCCESS == status && "SI_SetBaudRate");

    status = SI_SetLineControl(handle, SI_STOPBITS | (SI_PARITY << 4) | (SI_DATABITS << 8));
    assert(SI_SUCCESS == status && "SI_SetLineControl");

    // write strobe = /DTR to high
    status = SI_SetFlowControl(handle, SI_STATUS_INPUT, SI_HELD_INACTIVE, SI_HELD_INACTIVE,
        SI_STATUS_INPUT, SI_STATUS_INPUT, 0);
    assert(SI_SUCCESS == status && "SI_SetFlowControl");


    if (argc >= 2) {
        ULONGLONG t0 = GetTickCount64();
        char buf[128];
        while (fgets(buf, sizeof buf, fp)) {
#if 0
            printf("%s", buf);
            fflush(stdout);
#endif
            int a, d[8];
            int rv2 = sscanf_s(buf, "%x: %x %x %x %x %x %x %x %x", &a, &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7]);
            assert(9 == rv2 && "sscanf needs 8 data bytes");

            //printf("%04x: %02X %02X %02X %02X %02X %02X %02X %02X\n", a, d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
            //fflush(stdout);

            for (int i = 0; i < 8; ++i, ++a) {
                char buf[24];
                memset(buf, 0, sizeof buf);     // init auf L
                for (int j = 0; j < 16; ++j) {
                    int mask = 1 << (15 - j);
                    if (a & mask) buf[j] = H;
                }
                for (int j = 0; j < 8; ++j) {
                    int mask = 1 << (7 - j);
                    if (d[i] & mask) buf[16 + j] = H;
                }
                prog(handle, buf, sizeof buf, singlestep);
            }
            // putchar('\n');
        }
        ULONGLONG t1 = GetTickCount64();
        printf("prog time = %llu ms\n", t1 - t0);
        fflush(stdout);
    }
    else {
        char buf[24] = {
        //  A15/A7               A8/A0
        //  D0/Qa                D7/Qh
        //  15 1  2  3  4  5  6  7
            L, H, L, H, L, H, L, H,     // U10
            L, L, H, H, L, L, H, H,     // U9
            L, L, L, L, H, H, H, H};    // U8
        for (;;) {
            prog(handle, buf, sizeof buf, 0);
        }
#if 0
        char buf2[24] = {
        //  D0/Qa                D7/Qh
        //  15 1  2  3  4  5  6  7
            H, L, H, L, H, L, H, L,     // U10
            H, H, L, L, H, H, L, L,     // U9
            H, H, H, H, L, L, L, L};    // U8
        for (;;) {
            prog(handle, buf2, sizeof buf2, 0);
        }
#endif
    }

    status = SI_Close(handle);
    assert(SI_SUCCESS == status && "SI_Close");

    if (argc >= 2) {
        fclose(fp);
    }
    return 0;
}
