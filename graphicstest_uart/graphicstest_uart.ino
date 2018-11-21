 /* Copyright (c) 2018 KO <knockoutperformance@gmail.com>
  *  run adafruit 1.8 tft through uart spi 1 or 2
 */
#include <Arduino.h>
#include "GFX.h"    // Core graphics library
#include "Adafruit_ST7735.h" // Hardware-specific library
#include <SPI_UART.h>

#define SYS_BOARD_PLLAR (CKGR_PLLAR_ONE | CKGR_PLLAR_MULA(18UL) | CKGR_PLLAR_PLLACOUNT(0x3fUL) | CKGR_PLLAR_DIVA(1UL))
#define SYS_BOARD_MCKR ( PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)

#define TFT_CS    15
#define TFT_RST    0
#define TFT_DC     8
#define BTN_UP     2
#define BTN_DOWN   3
#define BTN_SEL    5
#define BTN_LEFT   6
#define BTN_RIGHT  7
#define SD_CS      4

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); // hardware uart spi

volatile bool upi = false;
volatile bool downi = false;
volatile bool lefti = false;
volatile bool righti = false;
volatile bool enteri = false;


void wait_for(uint32_t  wait) {
  uint64_t  myMillis = wait * 9500;  
  for (uint64_t  Counting = 0; Counting <= myMillis; Counting++) {
    __asm__("nop\n\t");
  }
}

void wait_for_micro(uint32_t  wait) {
  uint64_t  myMillis = wait * 12;   
  for (uint64_t  Counting = 0; Counting <= myMillis; Counting++) {
    __asm__("nop\n\t");
  }
}
  volatile static unsigned long last_interrupt_time = 0;

void up(){
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50UL){
    upi = true;
  }
  last_interrupt_time = interrupt_time;
}

void down(){
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50UL){
    downi = true;
  }
  last_interrupt_time = interrupt_time;
}

void left(){
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50UL){
    lefti = true;
  }
  last_interrupt_time = interrupt_time;
}

void right(){
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50UL){
    righti = true;
  }
  last_interrupt_time = interrupt_time;
}

void enter(){
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50UL){
    enteri = true;
  }
  last_interrupt_time = interrupt_time;
}




float p = 3.1415926;

#define pin_state LOW



void setup(void) {
    //Set FWS according to SYS_BOARD_MCKR configuration
    EFC0->EEFC_FMR = EEFC_FMR_FWS(4); //4 waitstate flash access
    EFC1->EEFC_FMR = EEFC_FMR_FWS(4);
    // Initialize PLLA to new overclock rate, was 114MHz
    PMC->CKGR_PLLAR = SYS_BOARD_PLLAR;
    while (!(PMC->PMC_SR & PMC_SR_LOCKA)) {}
    PMC->PMC_MCKR = SYS_BOARD_MCKR;
    while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {}
    // Re-initialize some stuff with the new speed
    SystemCoreClockUpdate();
    Serial.begin(511000);

  pinMode(BTN_UP,INPUT_PULLUP);
  pinMode(BTN_DOWN,INPUT_PULLUP);
  pinMode(BTN_LEFT,INPUT_PULLUP);
  pinMode(BTN_RIGHT,INPUT_PULLUP);
  pinMode(BTN_SEL,INPUT_PULLUP);
  
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS , OUTPUT);
  digitalWrite(SD_CS, HIGH);
  display.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  //Serial.println("Initialized");
  display.fillScreen(ST7735_BLACK);
/*  me_testing();

  attachInterrupt(BTN_LEFT, *left, pin_state);
  lefti = false;// needed to prevent interrupt
  attachInterrupt(BTN_RIGHT, *right, pin_state);
  righti = false;// needed to prevent interrupt
  attachInterrupt(BTN_SEL, *enter, pin_state);
  enteri = false;// needed to prevent interrupt
  attachInterrupt(BTN_UP, *up, pin_state);
  upi = false;// needed to prevent interrupt
  attachInterrupt(BTN_DOWN, *down, pin_state);
  downi = false;// needed to prevent interrupt*/
}


uint8_t stored_value[] = {0,0,0,0,0,0,0,0}; // 0 - 127 values same as x position on bar grap[h
const uint16_t rainbow2[] ={31,95,159,223,287,351,415,479,543,607,671,735,799,863,927,991,1055,1119,1183,1247,1311,1375,1439,1503,1567,1631,1695,1759,1823,1887,1951,2015,2047,2046,2045,2044,2043,2042,2041,2040,2039,2038,2037,2036,2035,2034,2033,2032,2031,2030,2029,2028,2027,2026,2025,2024,2023,2022,2021,2020,2019,2018,2017,2016,2016,4064,6112,8160,10208,12256,14304,16352,18400,20448,22496,24544,26592,28640,30688,32736,34784,36832,38880,40928,42976,45024,47072,49120,51168,53216,55264,57312,59360,61408,63456,65504,65504,65440,65376,65312,65248,65184,65120,65056,64992,64928,64864,64800,64736,64672,64608,64544,64480,64416,64352,64288,64224,64160,64096,64032,63968,63904,63840,63776,63712,63648,63584,63520};
const uint16_t y_posistions[] = {66,76,78,88,90,100,102,112,114,124,126,136,138,148,150,160}; 

uint8_t draw_bar(uint8_t value, uint8_t posis){
    value = value >> 1;
    if(stored_value[posis] <= value){
      for(uint8_t i = stored_value[posis]; i < value; i++){
        display.drawFastVLine(i, y_posistions[posis*2], y_posistions[(posis*2)+1] - y_posistions[posis*2], rainbow2[i]);
        }  
    } else if(stored_value[posis] > value){
      for(uint8_t i = value; i < stored_value[posis]; i++){
        display.drawFastVLine(i, y_posistions[posis*2], y_posistions[(posis*2)+1] - y_posistions[posis*2], ST7735_BLACK);
        }  
      }
  return value;
  }

void fast_draw_bars(uint8_t *p){
  for(uint8_t j = 0; j < 8; j++){
    if(j < 7){
      stored_value[j] = draw_bar(*p++, j);
    }else{
      stored_value[j] = draw_bar(*p, j);
    }
  }
}

struct {
  uint8_t amps;
  uint8_t amps1;
  uint8_t amps2;
  uint8_t amps3;
  uint8_t amps4;
  uint8_t amps5;
  uint8_t amps6;
  uint8_t amps7;      
} send_me;


uint16_t i = 0;

void loop() {

   //me_testing();

   if(i>255){
    i = 0;
   }

   send_me.amps = random(i-20, i);
   send_me.amps1 = random(i-20, i);
   send_me.amps2 = random(i-20, i);
   send_me.amps3 = random(i-20, i);
   send_me.amps4 = random(i-20, i);
   send_me.amps5 = random(i-20, i);
   send_me.amps6 = random(i-20, i);
   send_me.amps7 = random(i-20, i);


   fast_draw_bars((uint8_t*)&send_me); //awesome!

 /*  stored_value[0] = draw_bar(send_me.amps, 0);
   stored_value[1] = draw_bar(send_me.amps1, 1);
   stored_value[2] = draw_bar(send_me.amps2, 2);
   stored_value[3] = draw_bar(send_me.amps3, 3);
   stored_value[4] = draw_bar(send_me.amps4, 4);
   stored_value[5] = draw_bar(send_me.amps5, 5);
   stored_value[6] = draw_bar(send_me.amps6, 6);
   stored_value[7] = draw_bar(send_me.amps7, 7);*/
   
   i++;
   wait_for(50);
  

  /*
if(upi){
  Serial.println("up");
  display.fillTriangle(42, 60, 68, 20, 90, 60, ST7735_RED); // up
  //wait_for(250);
  display.fillScreen(ST7735_BLACK);
  upi = false;
}

if(downi){
  Serial.println("down");
  display.fillTriangle(42, 20, 90, 20, 68, 60, ST7735_RED); // down
   // wait_for(250);
  display.fillScreen(ST7735_BLACK);
  downi = false;
}

if(lefti){
  Serial.println("left");
  display.fillTriangle(42, 40, 90, 20, 90, 60, ST7735_RED); // left
  //  wait_for(250);
  display.fillScreen(ST7735_BLACK);
  lefti = false;
}

if(righti){
  Serial.println("right");
  display.fillTriangle(42, 20, 42, 60, 90, 40, ST7735_RED); // right
   // wait_for(250);
  display.fillScreen(ST7735_BLACK);
  righti = false;
}

if(enteri){
  Serial.println("enter");
  display.fillRoundRect(39, 98, 20, 45, 5, ST7735_GREEN);
  display.fillRoundRect(69, 98, 20, 45, 5, ST7735_GREEN);
  display.fillScreen(ST7735_BLACK);
  enteri = false;
}
*/

}

void me_testing(){
  // large block of text
  display.fillScreen(ST7735_BLACK);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST7735_WHITE);
  wait_for(1000);

  // tft print function!
  tftPrintTest();
 // wait_for(4000);

  // a single pixel
 // display.drawPixel(display.width()/2, display.height()/2, ST7735_GREEN);
 // wait_for(500);

  // line draw test
  testlines(ST7735_YELLOW);
 // wait_for(500);

  // optimized lines
  testfastlines(ST7735_RED, ST7735_BLUE);
  //wait_for(500);

  testdrawrects(ST7735_GREEN);
//  wait_for(500);

  testfillrects(ST7735_YELLOW, ST7735_MAGENTA);
//  wait_for(500);

  display.fillScreen(ST7735_BLACK);
  testfillcircles(10, ST7735_BLUE);
  testdrawcircles(10, ST7735_WHITE);
 // wait_for(500);

  testroundrects();
 // wait_for(500);

  testtriangles();
 // wait_for(500);

  mediabuttons();
 // wait_for(500);
  display.fillScreen(ST7735_BLACK);
  display.setTextSize(3);
  testdrawtext("DONE",ST7735_YELLOW);
  wait_for(500);
  display.fillScreen(ST7735_BLACK);
}

void testlines(uint16_t color) {
  display.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < display.width(); x+=6) {
    display.drawLine(0, 0, x, display.height()-1, color);
  }
  for (int16_t y=0; y < display.height(); y+=6) {
    display.drawLine(0, 0, display.width()-1, y, color);
  }

  display.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < display.width(); x+=6) {
    display.drawLine(display.width()-1, 0, x, display.height()-1, color);
  }
  for (int16_t y=0; y < display.height(); y+=6) {
    display.drawLine(display.width()-1, 0, 0, y, color);
  }

  display.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < display.width(); x+=6) {
    display.drawLine(0, display.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < display.height(); y+=6) {
    display.drawLine(0, display.height()-1, display.width()-1, y, color);
  }

  display.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < display.width(); x+=6) {
    display.drawLine(display.width()-1, display.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < display.height(); y+=6) {
    display.drawLine(display.width()-1, display.height()-1, 0, y, color);
  }
}

void testdrawtext(char *text, uint16_t color) {
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  display.fillScreen(ST7735_BLACK);
  for (int16_t y=0; y < display.height(); y+=5) {
    display.drawFastHLine(0, y, display.width(), color1);
  }
  for (int16_t x=0; x < display.width(); x+=5) {
    display.drawFastVLine(x, 0, display.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  display.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < display.width(); x+=6) {
    display.drawRect(display.width()/2 -x/2, display.height()/2 -x/2 , x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  display.fillScreen(ST7735_BLACK);
  for (int16_t x=display.width()-1; x > 6; x-=6) {
    display.fillRect(display.width()/2 -x/2, display.height()/2 -x/2 , x, x, color1);
    display.drawRect(display.width()/2 -x/2, display.height()/2 -x/2 , x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < display.width(); x+=radius*2) {
    for (int16_t y=radius; y < display.height(); y+=radius*2) {
      display.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < display.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < display.height()+radius; y+=radius*2) {
      display.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  display.fillScreen(ST7735_BLACK);
  int color = 0xF800;
  int t;
  int w = display.width()/2;
  int x = display.height()-1;
  int y = 0;
  int z = display.width();
  for(t = 0 ; t <= 15; t++) {
    display.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  display.fillScreen(ST7735_BLACK);
  int color = 100;
  int i;
  int t;
  for(t = 0 ; t <= 4; t+=1) {
    int x = 0;
    int y = 0;
    int w = display.width()-2;
    int h = display.height()-2;
    for(i = 0 ; i <= 16; i+=1) {
      display.drawRoundRect(x, y, w, h, 5, color);
      x+=2;
      y+=3;
      w-=4;
      h-=6;
      color+=1100;
    }
    color+=100;
  }
}

void tftPrintTest() {
  display.setTextWrap(false);
  display.fillScreen(ST7735_BLACK);
  display.setCursor(0, 30);
  display.setTextColor(ST7735_RED);
  display.setTextSize(1);
  display.println("Hello World!");
  display.setTextColor(ST7735_YELLOW);
  display.setTextSize(2);
  display.println("Hello World!");
  display.setTextColor(ST7735_GREEN);
  display.setTextSize(3);
  display.println("Hello World!");
  display.setTextColor(ST7735_BLUE);
  display.setTextSize(4);
  display.print(1234.567);
  wait_for(1500);
  display.setCursor(0, 0);
  display.fillScreen(ST7735_BLACK);
  display.setTextColor(ST7735_WHITE);
  display.setTextSize(0);
  display.println("Hello World!");
  display.setTextSize(1);
  display.setTextColor(ST7735_GREEN);
  display.print(p, 6);
  display.println(" Want pi?");
  display.println(" ");
  display.print(8675309, HEX); // print 8,675,309 out in HEX!
  display.println(" Print HEX!");
  display.println(" ");
  display.setTextColor(ST7735_WHITE);
  display.println("Sketch has been");
  display.println("running for: ");
  display.setTextColor(ST7735_MAGENTA);
  display.print(millis() / 1000);
  display.setTextColor(ST7735_WHITE);
  display.print(" seconds.");
  wait_for(500);
}

void mediabuttons() {
  // play
  display.fillScreen(ST7735_BLACK);
  display.fillRoundRect(25, 10, 78, 60, 8, ST7735_WHITE);
  display.fillTriangle(42, 20, 42, 60, 90, 40, ST7735_RED); // right
  wait_for(500);
  // pause
  display.fillRoundRect(25, 90, 78, 60, 8, ST7735_WHITE);
  display.fillRoundRect(39, 98, 20, 45, 5, ST7735_GREEN);
  display.fillRoundRect(69, 98, 20, 45, 5, ST7735_GREEN);
  wait_for(500);
  // play color
  display.fillTriangle(42, 20, 42, 60, 90, 40, ST7735_BLUE);
  wait_for(50);
  // pause color
  display.fillRoundRect(39, 98, 20, 45, 5, ST7735_RED);
  display.fillRoundRect(69, 98, 20, 45, 5, ST7735_RED);
  // play color
  display.fillTriangle(42, 20, 42, 60, 90, 40, ST7735_GREEN);
}
