/*highly modified adafruit library to run uart spi
 * Copyright (c) 2018 KO <knockoutperformance@gmail.com>*/

 /***************************************************
  This is a library for the Adafruit 1.8" SPI display.

This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
The 1.8" TFT shield
  ----> https://www.adafruit.com/product/802
The 1.44" TFT breakout
  ----> https://www.adafruit.com/product/2088
as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
 
#include "GFX.h"
#include "glcdfont.c"
#ifdef __AVR__
  #include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
  #include <pgmspace.h>
#endif

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
 #define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
 #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

GFX::GFX(int16_t w, int16_t h):
WIDTH(w), HEIGHT(h)
{
    _width    = WIDTH;
    _height   = HEIGHT;
    rotation  = 0;
    cursor_y  = cursor_x    = 0;
    textsize  = 1;
    textcolor = textbgcolor = 0xFFFF;
    wrap      = true;
    _cp437    = false;
    gfxFont   = NULL;
}

// Bresenham's algorithm - thx wikpedia
void GFX::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
        uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            writePixel(y0, x0, color);
        } else {
            writePixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void GFX::startWrite(){

}

void GFX::writePixel(int16_t x, int16_t y, uint16_t color){
    drawPixel(x, y, color);
}

// (x,y) is topmost point; if unsure, calling function
// should sort endpoints or call writeLine() instead
void GFX::writeFastVLine(int16_t x, int16_t y,
        int16_t h, uint16_t color) {
    drawFastVLine(x, y, h, color);
}

// (x,y) is leftmost point; if unsure, calling function
// should sort endpoints or call writeLine() instead
void GFX::writeFastHLine(int16_t x, int16_t y,
        int16_t w, uint16_t color) {
    drawFastHLine(x, y, w, color);
}

void GFX::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
        uint16_t color) {
    fillRect(x,y,w,h,color);
}

void GFX::endWrite(){
}

// (x,y) is topmost point; if unsure, calling function
// should sort endpoints or call drawLine() instead
void GFX::drawFastVLine(int16_t x, int16_t y,
        int16_t h, uint16_t color) {
    startWrite();
    writeLine(x, y, x, y+h-1, color);
    endWrite();
}

// (x,y) is leftmost point; if unsure, calling function
// should sort endpoints or call drawLine() instead
void GFX::drawFastHLine(int16_t x, int16_t y,
        int16_t w, uint16_t color) {
    startWrite();
    writeLine(x, y, x+w-1, y, color);
    endWrite();
}

void GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
        uint16_t color) {
    startWrite();
    for (int16_t i=x; i<x+w; i++) {
        writeFastVLine(i, y, h, color);
    }
    endWrite();
}

void GFX::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

void GFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
        uint16_t color) {
    if(x0 == x1){
        if(y0 > y1) _swap_int16_t(y0, y1);
        drawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if(y0 == y1){
        if(x0 > x1) _swap_int16_t(x0, x1);
        drawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        startWrite();
        writeLine(x0, y0, x1, y1, color);
        endWrite();
    }
}

// Draw a circle outline
void GFX::drawCircle(int16_t x0, int16_t y0, int16_t r,
        uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    startWrite();
    writePixel(x0  , y0+r, color);
    writePixel(x0  , y0-r, color);
    writePixel(x0+r, y0  , color);
    writePixel(x0-r, y0  , color);

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        writePixel(x0 + x, y0 + y, color);
        writePixel(x0 - x, y0 + y, color);
        writePixel(x0 + x, y0 - y, color);
        writePixel(x0 - x, y0 - y, color);
        writePixel(x0 + y, y0 + x, color);
        writePixel(x0 - y, y0 + x, color);
        writePixel(x0 + y, y0 - x, color);
        writePixel(x0 - y, y0 - x, color);
    }
    endWrite();
}

void GFX::drawCircleHelper( int16_t x0, int16_t y0,
        int16_t r, uint8_t cornername, uint16_t color) {
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {
            writePixel(x0 + x, y0 + y, color);
            writePixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            writePixel(x0 + x, y0 - y, color);
            writePixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            writePixel(x0 - y, y0 + x, color);
            writePixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            writePixel(x0 - y, y0 - x, color);
            writePixel(x0 - x, y0 - y, color);
        }
    }
}

void GFX::fillCircle(int16_t x0, int16_t y0, int16_t r,
        uint16_t color) {
    startWrite();
    writeFastVLine(x0, y0-r, 2*r+1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
    endWrite();
}

// Used to do circles and roundrects
void GFX::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
        uint8_t cornername, int16_t delta, uint16_t color) {

    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x1) {
            writeFastVLine(x0+x, y0-y, 2*y+1+delta, color);
            writeFastVLine(x0+y, y0-x, 2*x+1+delta, color);
        }
        if (cornername & 0x2) {
            writeFastVLine(x0-x, y0-y, 2*y+1+delta, color);
            writeFastVLine(x0-y, y0-x, 2*x+1+delta, color);
        }
    }
}

// Draw a rectangle
void GFX::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
        uint16_t color) {
    startWrite();
    writeFastHLine(x, y, w, color);
    writeFastHLine(x, y+h-1, w, color);
    writeFastVLine(x, y, h, color);
    writeFastVLine(x+w-1, y, h, color);
    endWrite();
}

// Draw a rounded rectangle
void GFX::drawRoundRect(int16_t x, int16_t y, int16_t w,
        int16_t h, int16_t r, uint16_t color) {
    // smarter version
    startWrite();
    writeFastHLine(x+r  , y    , w-2*r, color); // Top
    writeFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    writeFastVLine(x    , y+r  , h-2*r, color); // Left
    writeFastVLine(x+w-1, y+r  , h-2*r, color); // Right
    // draw four corners
    drawCircleHelper(x+r    , y+r    , r, 1, color);
    drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
    endWrite();
}

// Fill a rounded rectangle
void GFX::fillRoundRect(int16_t x, int16_t y, int16_t w,
        int16_t h, int16_t r, uint16_t color) {
    startWrite();
    writeFillRect(x+r, y, w-2*r, h, color);

    fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
    endWrite();
}

// Draw a triangle
void GFX::drawTriangle(int16_t x0, int16_t y0,
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void GFX::fillTriangle(int16_t x0, int16_t y0,
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

    int16_t a, b, y, last;

    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }

    startWrite();
    if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        writeFastHLine(a, y0, b-a+1, color);
        endWrite();
        return;
    }

    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t
    sa   = 0,
    sb   = 0;

    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if(a > b) _swap_int16_t(a,b);
        writeFastHLine(a, y, b-a+1, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if(a > b) _swap_int16_t(a,b);
        writeFastHLine(a, y, b-a+1, color);
    }
    endWrite();
}

// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------
void GFX::drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if(byte & 0x80) writePixel(x+i, y, color);
        }
    }
    endWrite();
}

// Draw a PROGMEM-resident 1-bit image at the specified (x,y) position,
// using the specified foreground (for set bits) and background (unset
// bits) colors.
void GFX::drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h,
  uint16_t color, uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            writePixel(x+i, y, (byte & 0x80) ? color : bg);
        }
    }
    endWrite();
}

// Draw a RAM-resident 1-bit image at the specified (x,y) position,
// using the specified foreground color (unset bits are transparent).
void GFX::drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            if(byte & 0x80) writePixel(x+i, y, color);
        }
    }
    endWrite();
}

// Draw a RAM-resident 1-bit image at the specified (x,y) position,
// using the specified foreground (for set bits) and background (unset
// bits) colors.
void GFX::drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            writePixel(x+i, y, (byte & 0x80) ? color : bg);
        }
    }
    endWrite();
}


// Draw PROGMEM-resident XBitMap Files (*.xbm), exported from GIMP,
// Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
// C Array can be directly used with this function.
// There is no RAM-resident version of this function; if generating bitmaps
// in RAM, use the format defined by drawBitmap() and call that instead.
void GFX::drawXBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte >>= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            // Nearly identical to drawBitmap(), only the bit order
            // is reversed here (left-to-right = LSB to MSB):
            if(byte & 0x01) writePixel(x+i, y, color);
        }
    }
    endWrite();
}

// Draw a PROGMEM-resident 8-bit image (grayscale) at the specified (x,y)
// pos.  Specifically for 8-bit display devices such as IS31FL3731;
// no color reduction/expansion is performed.
void GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
        }
    }
    endWrite();
}

// Draw a RAM-resident 8-bit image (grayscale) at the specified (x,y)
// pos.  Specifically for 8-bit display devices such as IS31FL3731;
// no color reduction/expansion is performed.
void GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, bitmap[j * w + i]);
        }
    }
    endWrite();
}

// Draw a PROGMEM-resident 8-bit image (grayscale) with a 1-bit mask
// (set bits = opaque, unset bits = clear) at the specified (x,y) position.
// BOTH buffers (grayscale and mask) must be PROGMEM-resident.
// Specifically for 8-bit display devices such as IS31FL3731;
// no color reduction/expansion is performed.
void GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&mask[j * bw + i / 8]);
            if(byte & 0x80) {
                writePixel(x+i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
            }
        }
    }
    endWrite();
}

// Draw a RAM-resident 8-bit image (grayscale) with a 1-bit mask
// (set bits = opaque, unset bits = clear) at the specified (x,y) pos.
// BOTH buffers (grayscale and mask) must be RAM-resident, no mix-and-
// match.  Specifically for 8-bit display devices such as IS31FL3731;
// no color reduction/expansion is performed.
void GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = mask[j * bw + i / 8];
            if(byte & 0x80) {
                writePixel(x+i, y, bitmap[j * w + i]);
            }
        }
    }
    endWrite();
}

// Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) at the specified (x,y)
// position.  For 16-bit display devices; no color reduction performed.
void GFX::drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, pgm_read_word(&bitmap[j * w + i]));
        }
    }
    endWrite();
}

// Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y)
// position.  For 16-bit display devices; no color reduction performed.
void GFX::drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, bitmap[j * w + i]);
        }
    }
    endWrite();
}

// Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask
// (set bits = opaque, unset bits = clear) at the specified (x,y) position.
// BOTH buffers (color and mask) must be PROGMEM-resident.
// For 16-bit display devices; no color reduction performed.
void GFX::drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&mask[j * bw + i / 8]);
            if(byte & 0x80) {
                writePixel(x+i, y, pgm_read_word(&bitmap[j * w + i]));
            }
        }
    }
    endWrite();
}

// Draw a RAM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask
// (set bits = opaque, unset bits = clear) at the specified (x,y) pos.
// BOTH buffers (color and mask) must be RAM-resident, no mix-and-match.
// For 16-bit display devices; no color reduction performed.
void GFX::drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = mask[j * bw + i / 8];
            if(byte & 0x80) {
                writePixel(x+i, y, bitmap[j * w + i]);
            }
        }
    }
    endWrite();
}

// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

// Draw a character
void GFX::drawChar(int16_t x, int16_t y, unsigned char c,
  uint16_t color, uint16_t bg, uint8_t size) {

    if(!gfxFont) { // 'Classic' built-in font

        if((x >= _width)            || // Clip right
           (y >= _height)           || // Clip bottom
           ((x + 6 * size - 1) < 0) || // Clip left
           ((y + 8 * size - 1) < 0))   // Clip top
            return;

        if(!_cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

        startWrite();
        for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
            uint8_t line = pgm_read_byte(&font[c * 5 + i]);
            for(int8_t j=0; j<8; j++, line >>= 1) {
                if(line & 1) {
                    if(size == 1)
                        writePixel(x+i, y+j, color);
                    else
                        writeFillRect(x+i*size, y+j*size, size, size, color);
                } else if(bg != color) {
                    if(size == 1)
                        writePixel(x+i, y+j, bg);
                    else
                        writeFillRect(x+i*size, y+j*size, size, size, bg);
                }
            }
        }
        if(bg != color) { // If opaque, draw vertical line for last column
            if(size == 1) writeFastVLine(x+5, y, 8, bg);
            else          writeFillRect(x+5*size, y, size, 8*size, bg);
        }
        endWrite();

    } else { // Custom font

        // Character is assumed previously filtered by write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // drawChar() directly with 'bad' characters of font may cause mayhem!

        c -= (uint8_t)pgm_read_byte(&gfxFont->first);
        GFXglyph *glyph  = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
        uint8_t  *bitmap = (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);

        uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
        uint8_t  w  = pgm_read_byte(&glyph->width),
                 h  = pgm_read_byte(&glyph->height);
        int8_t   xo = pgm_read_byte(&glyph->xOffset),
                 yo = pgm_read_byte(&glyph->yOffset);
        uint8_t  xx, yy, bits = 0, bit = 0;
        int16_t  xo16 = 0, yo16 = 0;

        if(size > 1) {
            xo16 = xo;
            yo16 = yo;
        }

        startWrite();
        for(yy=0; yy<h; yy++) {
            for(xx=0; xx<w; xx++) {
                if(!(bit++ & 7)) {
                    bits = pgm_read_byte(&bitmap[bo++]);
                }
                if(bits & 0x80) {
                    if(size == 1) {
                        writePixel(x+xo+xx, y+yo+yy, color);
                    } else {
                        writeFillRect(x+(xo16+xx)*size, y+(yo16+yy)*size,
                          size, size, color);
                    }
                }
                bits <<= 1;
            }
        }
        endWrite();

    } // End classic vs custom font
}

#if ARDUINO >= 100
size_t GFX::write(uint8_t c) {
#else
void GFX::write(uint8_t c) {
#endif
    if(!gfxFont) { // 'Classic' built-in font

        if(c == '\n') {                        // Newline?
            cursor_x  = 0;                     // Reset x to zero,
            cursor_y += textsize * 8;          // advance y one line
        } else if(c != '\r') {                 // Ignore carriage returns
            if(wrap && ((cursor_x + textsize * 6) > _width)) { // Off right?
                cursor_x  = 0;                 // Reset x to zero,
                cursor_y += textsize * 8;      // advance y one line
            }
            drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
            cursor_x += textsize * 6;          // Advance x one char
        }

    } else { // Custom font

        if(c == '\n') {
            cursor_x  = 0;
            cursor_y += (int16_t)textsize *
                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if(c != '\r') {
            uint8_t first = pgm_read_byte(&gfxFont->first);
            if((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
                GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(
                  &gfxFont->glyph))[c - first]);
                uint8_t   w     = pgm_read_byte(&glyph->width),
                          h     = pgm_read_byte(&glyph->height);
                if((w > 0) && (h > 0)) { // Is there an associated bitmap?
                    int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
                    if(wrap && ((cursor_x + textsize * (xo + w)) > _width)) {
                        cursor_x  = 0;
                        cursor_y += (int16_t)textsize *
                          (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                    }
                    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
                }
                cursor_x += (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize;
            }
        }

    }
#if ARDUINO >= 100
    return 1;
#endif
}

void GFX::setCursor(int16_t x, int16_t y) {
    cursor_x = x;
    cursor_y = y;
}

int16_t GFX::getCursorX(void) const {
    return cursor_x;
}

int16_t GFX::getCursorY(void) const {
    return cursor_y;
}

void GFX::setTextSize(uint8_t s) {
    textsize = (s > 0) ? s : 1;
}

void GFX::setTextColor(uint16_t c) {
    // For 'transparent' background, we'll set the bg
    // to the same as fg instead of using a flag
    textcolor = textbgcolor = c;
}

void GFX::setTextColor(uint16_t c, uint16_t b) {
    textcolor   = c;
    textbgcolor = b;
}

void GFX::setTextWrap(boolean w) {
    wrap = w;
}

uint8_t GFX::getRotation(void) const {
    return rotation;
}

void GFX::setRotation(uint8_t x) {
    rotation = (x & 3);
    switch(rotation) {
        case 0:
        case 2:
            _width  = WIDTH;
            _height = HEIGHT;
            break;
        case 1:
        case 3:
            _width  = HEIGHT;
            _height = WIDTH;
            break;
    }
}


void GFX::cp437(boolean x) {
    _cp437 = x;
}

void GFX::setFont(const GFXfont *f) {
    if(f) {            // Font struct pointer passed in?
        if(!gfxFont) { // And no current font struct?
            // Switching from classic to new font behavior.
            // Move cursor pos down 6 pixels so it's on baseline.
            cursor_y += 6;
        }
    } else if(gfxFont) { // NULL passed.  Current font struct defined?
        // Switching from new to classic font behavior.
        // Move cursor pos up 6 pixels so it's at top-left of char.
        cursor_y -= 6;
    }
    gfxFont = (GFXfont *)f;
}

// Broke this out as it's used by both the PROGMEM- and RAM-resident
// getTextBounds() functions.
void GFX::charBounds(char c, int16_t *x, int16_t *y,
  int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {

    if(gfxFont) {

        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += textsize * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if(c != '\r') { // Not a carriage return; is normal char
            uint8_t first = pgm_read_byte(&gfxFont->first),
                    last  = pgm_read_byte(&gfxFont->last);
            if((c >= first) && (c <= last)) { // Char present in this font?
                GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(
                  &gfxFont->glyph))[c - first]);
                uint8_t gw = pgm_read_byte(&glyph->width),
                        gh = pgm_read_byte(&glyph->height),
                        xa = pgm_read_byte(&glyph->xAdvance);
                int8_t  xo = pgm_read_byte(&glyph->xOffset),
                        yo = pgm_read_byte(&glyph->yOffset);
                if(wrap && ((*x+(((int16_t)xo+gw)*textsize)) > _width)) {
                    *x  = 0; // Reset x to zero, advance y by one line
                    *y += textsize * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                }
                int16_t ts = (int16_t)textsize,
                        x1 = *x + xo * ts,
                        y1 = *y + yo * ts,
                        x2 = x1 + gw * ts - 1,
                        y2 = y1 + gh * ts - 1;
                if(x1 < *minx) *minx = x1;
                if(y1 < *miny) *miny = y1;
                if(x2 > *maxx) *maxx = x2;
                if(y2 > *maxy) *maxy = y2;
                *x += xa * ts;
            }
        }

    } else { // Default font

        if(c == '\n') {                     // Newline?
            *x  = 0;                        // Reset x to zero,
            *y += textsize * 8;             // advance y one line
            // min/max x/y unchaged -- that waits for next 'normal' character
        } else if(c != '\r') {  // Normal char; ignore carriage returns
            if(wrap && ((*x + textsize * 6) > _width)) { // Off right?
                *x  = 0;                    // Reset x to zero,
                *y += textsize * 8;         // advance y one line
            }
            int x2 = *x + textsize * 6 - 1, // Lower-right pixel of char
                y2 = *y + textsize * 8 - 1;
            if(x2 > *maxx) *maxx = x2;      // Track max x, y
            if(y2 > *maxy) *maxy = y2;
            if(*x < *minx) *minx = *x;      // Track min x, y
            if(*y < *miny) *miny = *y;
            *x += textsize * 6;             // Advance x one char
        }
    }
}

// Pass string and a cursor position, returns UL corner and W,H.
void GFX::getTextBounds(char *str, int16_t x, int16_t y,
        int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t c; // Current character

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while((c = *str++))
        charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

// Same as above, but for PROGMEM strings
void GFX::getTextBounds(const __FlashStringHelper *str,
        int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t *s = (uint8_t *)str, c;

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while((c = pgm_read_byte(s++)))
        charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

// Return the size of the display (per current rotation)
int16_t GFX::width(void) const {
    return _width;
}

int16_t GFX::height(void) const {
    return _height;
}

void GFX::invertDisplay(boolean i) {
    // Do nothing, must be subclassed if supported by hardware
}

/***************************************************************************/
// code for the GFX button UI element

GFX_Button::GFX_Button(void) {
  _gfx = 0;
}

// Classic initButton() function: pass center & size
void GFX_Button::initButton(
 GFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  // Tweak arguments and pass to the newer initButtonUL() function...
  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill,
    textcolor, label, textsize);
}

// Newer function instead accepts upper-left corner & size
void GFX_Button::initButtonUL(
 GFX *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  _x1           = x1;
  _y1           = y1;
  _w            = w;
  _h            = h;
  _outlinecolor = outline;
  _fillcolor    = fill;
  _textcolor    = textcolor;
  _textsize     = textsize;
  _gfx          = gfx;
  strncpy(_label, label, 9);
}

void GFX_Button::drawButton(boolean inverted) {
  uint16_t fill, outline, text;

  if(!inverted) {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
  } else {
    fill    = _textcolor;
    outline = _outlinecolor;
    text    = _fillcolor;
  }

  uint8_t r = min(_w, _h) / 4; // Corner radius
  _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
  _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

  _gfx->setCursor(_x1 + (_w/2) - (strlen(_label) * 3 * _textsize),
    _y1 + (_h/2) - (4 * _textsize));
  _gfx->setTextColor(text);
  _gfx->setTextSize(_textsize);
  _gfx->print(_label);
}

boolean GFX_Button::contains(int16_t x, int16_t y) {
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _h)));
}

void GFX_Button::press(boolean p) {
  laststate = currstate;
  currstate = p;
}

boolean GFX_Button::isPressed() { return currstate; }
boolean GFX_Button::justPressed() { return (currstate && !laststate); }
boolean GFX_Button::justReleased() { return (!currstate && laststate); }

// -------------------------------------------------------------------------

GFXcanvas1::GFXcanvas1(uint16_t w, uint16_t h) : GFX(w, h) {
    uint16_t bytes = ((w + 7) / 8) * h;
    if((buffer = (uint8_t *)malloc(bytes))) {
        memset(buffer, 0, bytes);
    }
}

GFXcanvas1::~GFXcanvas1(void) {
    if(buffer) free(buffer);
}

uint8_t* GFXcanvas1::getBuffer(void) {
    return buffer;
}

void GFXcanvas1::drawPixel(int16_t x, int16_t y, uint16_t color) {
#ifdef __AVR__
    // Bitmask tables of 0x80>>X and ~(0x80>>X), because X>>Y is slow on AVR
    static const uint8_t PROGMEM
        GFXsetBit[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 },
        GFXclrBit[] = { 0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE };
#endif

    if(buffer) {
        if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;

        int16_t t;
        switch(rotation) {
            case 1:
                t = x;
                x = WIDTH  - 1 - y;
                y = t;
                break;
            case 2:
                x = WIDTH  - 1 - x;
                y = HEIGHT - 1 - y;
                break;
            case 3:
                t = x;
                x = y;
                y = HEIGHT - 1 - t;
                break;
        }

        uint8_t   *ptr  = &buffer[(x / 8) + y * ((WIDTH + 7) / 8)];
#ifdef __AVR__
        if(color) *ptr |= pgm_read_byte(&GFXsetBit[x & 7]);
        else      *ptr &= pgm_read_byte(&GFXclrBit[x & 7]);
#else
        if(color) *ptr |=   0x80 >> (x & 7);
        else      *ptr &= ~(0x80 >> (x & 7));
#endif
    }
}

void GFXcanvas1::fillScreen(uint16_t color) {
    if(buffer) {
        uint16_t bytes = ((WIDTH + 7) / 8) * HEIGHT;
        memset(buffer, color ? 0xFF : 0x00, bytes);
    }
}

GFXcanvas8::GFXcanvas8(uint16_t w, uint16_t h) : GFX(w, h) {
    uint32_t bytes = w * h;
    if((buffer = (uint8_t *)malloc(bytes))) {
        memset(buffer, 0, bytes);
    }
}

GFXcanvas8::~GFXcanvas8(void) {
    if(buffer) free(buffer);
}

uint8_t* GFXcanvas8::getBuffer(void) {
    return buffer;
}

void GFXcanvas8::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if(buffer) {
        if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;

        int16_t t;
        switch(rotation) {
            case 1:
                t = x;
                x = WIDTH  - 1 - y;
                y = t;
                break;
            case 2:
                x = WIDTH  - 1 - x;
                y = HEIGHT - 1 - y;
                break;
            case 3:
                t = x;
                x = y;
                y = HEIGHT - 1 - t;
                break;
        }

        buffer[x + y * WIDTH] = color;
    }
}

void GFXcanvas8::fillScreen(uint16_t color) {
    if(buffer) {
        memset(buffer, color, WIDTH * HEIGHT);
    }
}

void GFXcanvas8::writeFastHLine(int16_t x, int16_t y,
  int16_t w, uint16_t color) {

    if((x >= _width) || (y < 0) || (y >= _height)) return;
    int16_t x2 = x + w - 1;
    if(x2 < 0) return;

    // Clip left/right
    if(x < 0) {
        x = 0;
        w = x2 + 1;
    }
    if(x2 >= _width) w = _width - x;

    int16_t t;
    switch(rotation) {
        case 1:
            t = x;
            x = WIDTH  - 1 - y;
            y = t;
            break;
        case 2:
            x = WIDTH  - 1 - x;
            y = HEIGHT - 1 - y;
            break;
        case 3:
            t = x;
            x = y;
            y = HEIGHT - 1 - t;
            break;
    }

    memset(buffer + y * WIDTH + x, color, w);
}

GFXcanvas16::GFXcanvas16(uint16_t w, uint16_t h) : GFX(w, h) {
    uint32_t bytes = w * h * 2;
    if((buffer = (uint16_t *)malloc(bytes))) {
        memset(buffer, 0, bytes);
    }
}

GFXcanvas16::~GFXcanvas16(void) {
    if(buffer) free(buffer);
}

uint16_t* GFXcanvas16::getBuffer(void) {
    return buffer;
}

void GFXcanvas16::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if(buffer) {
        if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;

        int16_t t;
        switch(rotation) {
            case 1:
                t = x;
                x = WIDTH  - 1 - y;
                y = t;
                break;
            case 2:
                x = WIDTH  - 1 - x;
                y = HEIGHT - 1 - y;
                break;
            case 3:
                t = x;
                x = y;
                y = HEIGHT - 1 - t;
                break;
        }

        buffer[x + y * WIDTH] = color;
    }
}

void GFXcanvas16::fillScreen(uint16_t color) {
    if(buffer) {
        uint8_t hi = color >> 8, lo = color & 0xFF;
        if(hi == lo) {
            memset(buffer, lo, WIDTH * HEIGHT * 2);
        } else {
            uint32_t i, pixels = WIDTH * HEIGHT;
            for(i=0; i<pixels; i++) buffer[i] = color;
        }
    }
}
