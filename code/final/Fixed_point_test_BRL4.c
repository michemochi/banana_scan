/*
 * File:        Fixed point_test_BRL4.c
 * Author:      Bruce Land
 * For use with Sean Carroll's Big Board
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// fixed point built-ins
// see
// http://ww1.microchip.com/downloads/en/DeviceDoc/50001686J.pdf
// Chapter 10
#include <stdfix.h>
#include <stdlib.h>

////////////////////////////////////


/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
// === My 16:16 fixed point macros ========================================
typedef signed int fix16;
#define multfix16(a,b) ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define float2fix16(a) ((fix16)((a)*65536.0)) // 2^16
#define fix2float16(a) ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b) ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a) (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a) abs(a)

// === print utilities ========================================
// print a line on the TFT
// string buffer
char buffer[60];

void printLine(int line_number, char* print_buffer, short text_color, short back_color) {
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color); // x,y,w,h,radius,color
    tft_setTextColor(text_color);
    tft_setCursor(0, v_pos);
    tft_setTextSize(1);
    tft_writeString(print_buffer);
}

void printLine2(int line_number, char* print_buffer, short text_color, short back_color) {
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 20;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 319, 16, 1, back_color); // x,y,w,h,radius,color
    tft_setTextColor(text_color);
    tft_setCursor(0, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_fix;

// system 1 second interval tick
int sys_time_seconds;

// === Timer Thread =================================================
// update a 1 second tick counter

static PT_THREAD(protothread_timer(struct pt *pt)) {
    PT_BEGIN(pt);
    
    // set up LED to blink
    mPORTASetBits(BIT_0); //Clear bits to ensure light is off.
    mPORTASetPinsDigitalOut(BIT_0); //Set port as output
    while (1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000);
        sys_time_seconds++;
        // toggle the LED on the big board
        mPORTAToggleBits(BIT_0);
        // draw sys_time
        sprintf(buffer, "%d", sys_time_seconds);
        printLine2(0, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // timer thread

// === fixed point Thread =================================================
// test fixed point speeds
// The XC32 Users Guide
// http://ww1.microchip.com/downloads/en/DeviceDoc/50001686J.pdf
// introduces some new variable types for fixed point
// The _Accum datatype is s16.15 format.
// Sign bit, 16-bit integer and 15-bit fraction

_Accum accum1, accum2, accum3;
fix16 fix1, fix2, fix3;
float float1, float2, float3, i;
int time1, time2;

// Use a suffix in a fixed-point literal constant:
// k or K for _Accum
// e.g. accum1 = 0.1K ;

static PT_THREAD(protothread_fix(struct pt *pt)) {
    PT_BEGIN(pt);
    i = 0;
    while (1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(500);

        float1 = i + 0.1;
        float2 = i + 0.2;
        i = i + 0.1;

        accum1 = (_Accum) float1;
        accum2 = (_Accum) float2;
        
        fix1 = float2fix16(float1);
        fix2 = float2fix16(float2);
        

        time1 = ReadTimer2();
        accum3 = accum1 + accum2*accum1;
        // null time 3 cycles
        time2 = ReadTimer2() - time1;

        sprintf(buffer, "_Accum %f %d", (float) accum3, time2 - 3);
        printLine2(2, buffer, ILI9340_WHITE, ILI9340_BLACK);

        time1 = ReadTimer2();
        //fix3 = fix3 = multfix16(fix1, fix2);
        fix3 = fix1 + multfix16(fix2,fix1);
        // null time 3 cycles
        time2 = ReadTimer2() - time1;

        sprintf(buffer, "fix16  %f %d", fix2float16(fix3), time2 - 3);
        printLine2(4, buffer, ILI9340_WHITE, ILI9340_BLACK);

        time1 = ReadTimer2();
        float3 = float1 + float2*float1;
        // null time 3 cycles
        time2 = ReadTimer2() - time1;

        sprintf(buffer, "float  %f %d", float3, time2 - 3);
        printLine2(6, buffer, ILI9340_WHITE, ILI9340_BLACK);
        
        fix1 = ((rand() & 0x2ffff)) - float2fix16(1.5) ; //- 0x10000
        sprintf(buffer, "rand %f", fix2float16(fix1));
        printLine2(8, buffer, ILI9340_WHITE, ILI9340_BLACK);
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // fix thread

// === Main  ======================================================

void main(void) {
    //SYSTEMConfigPerformance(PBCLK);

    ANSELA = 0;
    ANSELB = 0;

    // timer for fix operations
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 0xffff);

    // === config threads ==========
    // turns OFF UART support and debugger pin, unless defines are set
    PT_setup();

    // === setup system wide interrupts  ========
    INTEnableSystemMultiVectoredInt();

    // init the threads
    PT_INIT(&pt_timer);
    PT_INIT(&pt_fix);


    // init the display
    // NOTE that this init assumes SPI channel 1 connections
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    //240x320 vertical display
    tft_setRotation(1); // Use tft_setRotation(1) for 320x240

    // seed random color
    srand(1);

    // round-robin scheduler for threads
    while (1) {
        PT_SCHEDULE(protothread_timer(&pt_timer));
        PT_SCHEDULE(protothread_fix(&pt_fix));
    }
} // main

// === end  ======================================================

