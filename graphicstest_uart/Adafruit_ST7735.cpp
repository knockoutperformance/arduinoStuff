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
 
#include "Adafruit_ST7735.h"
#include <limits.h>
#include "pins_arduino.h"
#include "wiring_private.h"

#define sck_divisor 3 // 4 =21mhz

#define UART_SPI //MOSI TX2 MISO RX2 SCK A0 CS&CE User selected

#ifdef UART_SPI
  #include <SPI_UART.h>
  SPIUARTClass UART_SPI1(USART1);
#else
  #include "SPI.h"
#endif

extern void wait_for(uint32_t  wait);

inline uint16_t swapcolor(uint16_t x) { 
  return (x << 11) | (x & 0x07E0) | (x >> 11);
}

#if defined (SPI_HAS_TRANSACTION)
  static SPISettings mySPISettings;
#elif defined (__AVR__) || defined(CORE_TEENSY)
  static uint8_t SPCRbackup;
  static uint8_t mySPCR;
#endif


// Constructor when using software SPI.  All output pins are configurable.
Adafruit_ST7735::Adafruit_ST7735(int8_t cs, int8_t dc, int8_t sid, int8_t sclk, int8_t rst) 
  : GFX(ST7735_TFTWIDTH_128, ST7735_TFTHEIGHT_160){
  _cs   = cs;
  _dc   = dc;
  _sid  = sid;
  _sclk = sclk;
  _rst  = rst;
  hwSPI = false;
}

// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
Adafruit_ST7735::Adafruit_ST7735(int8_t cs, int8_t dc, int8_t rst) 
  : GFX(ST7735_TFTWIDTH_128, ST7735_TFTHEIGHT_160) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  hwSPI = true;
  _sid  = _sclk = -1;
}


inline void Adafruit_ST7735::spiBeginEndwrite(bool start_write) {
  if(start_write){
 //   UART_SPI1.setClockDivider(sck_divisor); //42mhz
    *dcport |= dcpinmask; // DC_HIGH():
    *csport &= ~cspinmask; // CS_LOW():   
  }else{
    *csport |= cspinmask; // CS_HIGH():    
  }
}

inline void Adafruit_ST7735::spiBeginEndcommand(bool start_write) {
  if(start_write){
 //   UART_SPI1.setClockDivider(sck_divisor); //42mhz
    *dcport &= ~dcpinmask; // DC_LOW():
    *csport &= ~cspinmask; // CS_LOW():    
  }else{
    *csport |= cspinmask; // CS_HIGH():    
  }
}

inline void Adafruit_ST7735::spiwrite(uint8_t c) {
#ifdef UART_SPI
    UART_SPI1.transfer(c);
#else
    // Fast SPI bitbang swiped from LPD8806 library
    for(uint8_t bit = 0x80; bit; bit >>= 1) {
      if(c & bit) *dataport |=  datapinmask;
      else        *dataport &= ~datapinmask;
      *clkport |=  clkpinmask;
      *clkport &= ~clkpinmask;
    }
#endif
}



inline void Adafruit_ST7735::writecommand(uint8_t c) {
  spiBeginEndcommand(true);
  spiwrite(c);
  spiBeginEndcommand(false);
}


inline void Adafruit_ST7735::writedata(uint8_t d) {
  spiBeginEndwrite(true);  
  spiwrite(d);
  spiBeginEndwrite(false);
}

inline void Adafruit_ST7735::writedata16(uint16_t d) {
  spiBeginEndwrite(true);  
  uint8_t buf[] = {d >> 8,d};
  UART_SPI1.transfer(buf, sizeof(buf));
  spiBeginEndwrite(false);
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const uint8_t PROGMEM
  Bcmd[] = {                  // Initialization commands for 7735B screens
    18,                       // 18 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, no args, w/delay
      50,                     //     50 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, no args, w/delay
      255,                    //     255 = 500 ms delay
    ST7735_COLMOD , 1+DELAY,  //  3: Set color mode, 1 arg + delay:
      0x05,                   //     16-bit color
      10,                     //     10 ms delay
    ST7735_FRMCTR1, 3+DELAY,  //  4: Frame rate control, 3 args + delay:
      0x00,                   //     fastest refresh
      0x06,                   //     6 lines front porch
      0x03,                   //     3 lines back porch
      10,                     //     10 ms delay
    ST7735_MADCTL , 1      ,  //  5: Memory access ctrl (directions), 1 arg:
      0x08,                   //     Row addr/col addr, bottom to top refresh
    ST7735_DISSET5, 2      ,  //  6: Display settings #5, 2 args, no delay:
      0x15,                   //     1 clk cycle nonoverlap, 2 cycle gate
                              //     rise, 3 cycle osc equalize
      0x02,                   //     Fix on VTL
    ST7735_INVCTR , 1      ,  //  7: Display inversion control, 1 arg:
      0x0,                    //     Line inversion
    ST7735_PWCTR1 , 2+DELAY,  //  8: Power control, 2 args + delay:
      0x02,                   //     GVDD = 4.7V
      0x70,                   //     1.0uA
      10,                     //     10 ms delay
    ST7735_PWCTR2 , 1      ,  //  9: Power control, 1 arg, no delay:
      0x05,                   //     VGH = 14.7V, VGL = -7.35V
    ST7735_PWCTR3 , 2      ,  // 10: Power control, 2 args, no delay:
      0x01,                   //     Opamp current small
      0x02,                   //     Boost frequency
    ST7735_VMCTR1 , 2+DELAY,  // 11: Power control, 2 args + delay:
      0x3C,                   //     VCOMH = 4V
      0x38,                   //     VCOML = -1.1V
      10,                     //     10 ms delay
    ST7735_PWCTR6 , 2      ,  // 12: Power control, 2 args, no delay:
      0x11, 0x15,
    ST7735_GMCTRP1,16      ,  // 13: Magical unicorn dust, 16 args, no delay:
      0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
      0x21, 0x1B, 0x13, 0x19, //      these config values represent)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    ST7735_GMCTRN1,16+DELAY,  // 14: Sparkles and rainbows, 16 args + delay:
      0x0B, 0x14, 0x08, 0x1E, //     (ditto)
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                     //     10 ms delay
    ST7735_CASET  , 4      ,  // 15: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 2
      0x00, 0x81,             //     XEND = 129
    ST7735_RASET  , 4      ,  // 16: Row addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 1
      0x00, 0x81,             //     XEND = 160
    ST7735_NORON  ,   DELAY,  // 17: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,   DELAY,  // 18: Main screen turn on, no args, w/delay
      255 },                  //     255 = 500 ms delay

  Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

  Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 0
      0x00, 0x7F+0x02,        //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,             //     XSTART = 0
      0x00, 0x9F+0x01 },      //     XEND = 159

  Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159

  Rcmd2green144[] = {              // Init for 7735R, part 2 (green 1.44 tab)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127

  Rcmd2green160x80[] = {              // Init for 7735R, part 2 (mini 160x80)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159


  Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay
 
	  
// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void Adafruit_ST7735::commandList(const uint8_t *addr) {
  uint8_t  numCommands, numArgs;
  uint16_t ms;

  numCommands = pgm_read_byte(addr++);   // Number of commands to follow
  while(numCommands--) {                 // For each command...
    writecommand(pgm_read_byte(addr++)); //   Read, issue command
    numArgs  = pgm_read_byte(addr++);    //   Number of args to follow
    ms       = numArgs & DELAY;          //   If hibit set, delay follows args
    numArgs &= ~DELAY;                   //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      writedata(pgm_read_byte(addr++));  //     Read, issue argument
    }

    if(ms) {
      ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      wait_for(ms);
    }
  }
}


// Initialization code common to both 'B' and 'R' type displays
void Adafruit_ST7735::commonInit(const uint8_t *cmdList) {
  ystart = xstart = colstart  = rowstart = 0; // May be overridden in init func

  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  csport    = portOutputRegister(digitalPinToPort(_cs));
  dcport    = portOutputRegister(digitalPinToPort(_dc));
  cspinmask = digitalPinToBitMask(_cs);
  dcpinmask = digitalPinToBitMask(_dc);

#ifdef UART_SPI
    UART_SPI1.begin();
    UART_SPI1.setClockDivider(sck_divisor); 
    // UART_SPI1.setDataMode(0x00);
    // UART_SPI1.setBitOrder(MSBFIRST);
#else
    pinMode(_sclk, OUTPUT);
    pinMode(_sid , OUTPUT);
    digitalWrite(_sclk, LOW);
    digitalWrite(_sid, LOW);
    clkport     = portOutputRegister(digitalPinToPort(_sclk));
    dataport    = portOutputRegister(digitalPinToPort(_sid));
    clkpinmask  = digitalPinToBitMask(_sclk);
    datapinmask = digitalPinToBitMask(_sid);
#endif
 
  // toggle RST low to reset; CS low so it'll listen to us
  *csport &= ~cspinmask; // CS_LOW():
  if (_rst != -1) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    wait_for(400);
    digitalWrite(_rst, LOW);
    wait_for(400);
    digitalWrite(_rst, HIGH);
    wait_for(400);
  }

  if(cmdList) commandList(cmdList);
}

// Initialization for ST7735B screens
void Adafruit_ST7735::initB(void) {
  commonInit(Bcmd);
  setRotation(0);
}

// Initialization for ST7735R screens (green or red tabs)
void Adafruit_ST7735::initR(uint8_t options) {
  commonInit(Rcmd1);
  if(options == INITR_GREENTAB) {
    commandList(Rcmd2green);
    colstart = 2;
    rowstart = 1;
  } else if(options == INITR_144GREENTAB) {
    _height = ST7735_TFTHEIGHT_128;
    _width = ST7735_TFTWIDTH_128;
    commandList(Rcmd2green144);
    colstart = 2;
    rowstart = 3;
  } else if(options == INITR_MINI160x80) {
    _height = ST7735_TFTHEIGHT_160;
    _width = ST7735_TFTWIDTH_80;
    commandList(Rcmd2green160x80);
    colstart = 24;
    rowstart = 0;
  } else {
    // colstart, rowstart left at default '0' values
    commandList(Rcmd2red);
  }
  commandList(Rcmd3);

  // if black, change MADCTL color filter
  if ((options == INITR_BLACKTAB) || (options == INITR_MINI160x80)) {
    writecommand(ST7735_MADCTL);
    writedata(0xC0);
  }

  tabcolor = options;

  setRotation(0);
}

void Adafruit_ST7735::setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  uint8_t buf[] = {0x00  , x0+xstart  , 0x00  , x1+xstart};
  uint8_t buf2[] = {0x00  , y0+ystart  , 0x00  , y1+ystart};

  writecommand(ST7735_CASET); // Column addr set
  
  spiBeginEndwrite(true);
  UART_SPI1.transfer(buf , sizeof(buf)) ; 
  spiBeginEndwrite(false);
  
  writecommand(ST7735_RASET); // Row addr set
  
  spiBeginEndwrite(true);
  UART_SPI1.transfer(buf2 , sizeof(buf2)) ; 
  spiBeginEndwrite(false);  

  writecommand(ST7735_RAMWR); // write to RAM
}


void Adafruit_ST7735::pushColor(uint16_t color) {
  writedata16(color);
}

void Adafruit_ST7735::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

  setAddrWindow(x,y,x+1,y+1);
  
  writedata16(color);
}


void Adafruit_ST7735::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((y+h-1) >= _height) h = _height-y;
    
  setAddrWindow(x, y, x, y+h-1);

  while (h--) {
  writedata16(color);
  }
}


void Adafruit_ST7735::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((x+w-1) >= _width)  w = _width-x;

  setAddrWindow(x, y, x+w-1, y);

  while (w--) {
   writedata16(color);  // writedata(color >> 8);
  }
}

void Adafruit_ST7735::fillScreen(uint16_t color) {
  fillRect(0, 0,  _width, _height, color);
}

// fill a rectangle
void Adafruit_ST7735::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  uint32_t totalPixels = (uint32_t)w*(uint32_t)h;
  uint16_t scanline[totalPixels];

  for (uint16_t i = 0; i < totalPixels; i++){
     scanline[i] = color;
  }
  
  setAddrWindow(x, y, x+w-1, y+h-1);
  
  spiBeginEndwrite(true);
  UART_SPI1.transfer(scanline , sizeof(scanline)) ; 
  spiBeginEndwrite(false);

}


// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t Adafruit_ST7735::Color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

void Adafruit_ST7735::setRotation(uint8_t m) {

  writecommand(ST7735_MADCTL);
  rotation = m % 4; // can't be higher than 3
  switch (rotation) {
   case 0:
     if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
       writedata(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
     } else {
       writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
     }

     if (tabcolor == INITR_144GREENTAB) {
       _height = ST7735_TFTHEIGHT_128;
       _width  = ST7735_TFTWIDTH_128;
     } else if (tabcolor == INITR_MINI160x80)  {
       _height = ST7735_TFTHEIGHT_160;
       _width = ST7735_TFTWIDTH_80;
     } else {
       _height = ST7735_TFTHEIGHT_160;
       _width  = ST7735_TFTWIDTH_128;
     }
     xstart = colstart;
     ystart = rowstart;
     break;
   case 1:
     if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
       writedata(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
     } else {
       writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
     }

     if (tabcolor == INITR_144GREENTAB)  {
       _width = ST7735_TFTHEIGHT_128;
       _height = ST7735_TFTWIDTH_128;
     } else if (tabcolor == INITR_MINI160x80)  {
       _width = ST7735_TFTHEIGHT_160;
       _height = ST7735_TFTWIDTH_80;
     } else {
       _width = ST7735_TFTHEIGHT_160;
       _height = ST7735_TFTWIDTH_128;
     }
     ystart = colstart;
     xstart = rowstart;
     break;
  case 2:
     if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
       writedata(MADCTL_RGB);
     } else {
       writedata(MADCTL_BGR);
     }

     if (tabcolor == INITR_144GREENTAB) {
       _height = ST7735_TFTHEIGHT_128;
       _width  = ST7735_TFTWIDTH_128;
     } else if (tabcolor == INITR_MINI160x80)  {
       _height = ST7735_TFTHEIGHT_160;
       _width = ST7735_TFTWIDTH_80;
     } else {
       _height = ST7735_TFTHEIGHT_160;
       _width  = ST7735_TFTWIDTH_128;
     }
     xstart = colstart;
     ystart = rowstart;
     break;
   case 3:
     if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
       writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
     } else {
       writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
     }

     if (tabcolor == INITR_144GREENTAB)  {
       _width = ST7735_TFTHEIGHT_128;
       _height = ST7735_TFTWIDTH_128;
     } else if (tabcolor == INITR_MINI160x80)  {
       _width = ST7735_TFTHEIGHT_160;
       _height = ST7735_TFTWIDTH_80;
     } else {
       _width = ST7735_TFTHEIGHT_160;
       _height = ST7735_TFTWIDTH_128;
     }
     ystart = colstart;
     xstart = rowstart;
     break;
  }
}


void Adafruit_ST7735::invertDisplay(boolean i) {
  writecommand(i ? ST7735_INVON : ST7735_INVOFF);
}
