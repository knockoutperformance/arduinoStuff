/*8mhz ardunio mega2560 DUPLEX SPI COMMS master*/

#include <SPI.h>
#include <avr/interrupt.h>
#include "digitalWriteFast.h"

#define debug

#define indicator       LED_BUILTIN
#define SS                53                     // master slave select

//#define crc_in_checking                          // crc8 check on commands received
#define crc_out_checking                         // crc8 check on outgoing data, costs 4 micros, need to enable on the slave due
#define unrolled_loop                            // if unrolled 16-24 us
#define size_data         10
#define timer_rate        32                     // 1,2,4,8,16,32,64,128 = brink of stability enable timer for sending data flag
#define led_on          HIGH

uint8_t spi_buffer[size_data];
volatile bool do_it = false;                      //timer flag for transfering data

void enable_timer(bool on, uint16_t click_mode){
 if(on){  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 62500 / click_mode;                   // 1,2,4,8,16,32,64,128 hz
  TCCR1B |= bit(WGM12);                          // CTC mode
  TCCR1B |= bit(CS12);                           // 256 prescaler 
  TIMSK1 |= bit(OCIE1A);                         // enable timer compare interrupt
  TIFR1 |= bit(OCF1A);                           // clear the compare flag
 }else{
  TIMSK1 &= ~bit(OCIE1A);                        // disable timer compare interrupt 
  TIFR1 |= bit(OCF1A);                           // clear the compare flag
  }
 }

 /////crc8 checksum///////
#define  CRC1B(b)       ( (uint8_t)((b)<<1) ^ ((b)&0x80? 0x07 : 0) ) // MS first
#define  CRC(b)         CRC_##b                 // or CRC8B(b) //8+1 entry enum lookup table define

enum {
    CRC(0x01) = CRC1B(0x80),
    CRC(0x02) = CRC1B(CRC(0x01)),
    CRC(0x04) = CRC1B(CRC(0x02)),
    CRC(0x08) = CRC1B(CRC(0x04)),
    CRC(0x10) = CRC1B(CRC(0x08)),
    CRC(0x20) = CRC1B(CRC(0x10)),
    CRC(0x40) = CRC1B(CRC(0x20)),
    CRC(0x80) = CRC1B(CRC(0x40)),
    CRC(0x03) = CRC(0x02)^CRC(0x01)// Add 0x03 to optimise in CRCTAB1
    };

/* 
 * Build a 256 byte CRC constant lookup table
 */

#define  CRCTAB1(ex)    CRC(0x01)ex, CRC(0x02)ex,  CRC(0x03)ex,
#define  CRCTAB2(ex)    CRCTAB1(ex)  CRC(0x04)ex,  CRCTAB1(^CRC(0x04)ex)
#define  CRCTAB3(ex)    CRCTAB2(ex)  CRC(0x08)ex,  CRCTAB2(^CRC(0x08)ex)
#define  CRCTAB4(ex)    CRCTAB3(ex)  CRC(0x10)ex,  CRCTAB3(^CRC(0x10)ex)
#define  CRCTAB5(ex)    CRCTAB4(ex)  CRC(0x20)ex,  CRCTAB4(^CRC(0x20)ex)
#define  CRCTAB6(ex)    CRCTAB5(ex)  CRC(0x40)ex,  CRCTAB5(^CRC(0x40)ex)

static const uint8_t crc_table[256] = { 0, CRCTAB6() CRC(0x80), CRCTAB6(^CRC(0x80)) };//This is the final lookup table. It is rough on the compiler, but generates the table at compile time                                                                                   // required lookup table automagically at compile time.

uint8_t crc85(uint8_t *p){ // unrolled to make faster
    uint8_t crc = 0x0;
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p];
    return crc;
  }

void print_buffer(){    
    for(uint8_t t = 0; t < sizeof(spi_buffer); t++){
    Serial.print(spi_buffer[t]);
    if(t < (sizeof(spi_buffer) - 1)){
     Serial.print(F(", "));
    }else{
     Serial.println(F(""));
     }
    }
}

void assemSpiTransfTen(void *spi_buffer) { // fast as possible
  #ifdef crc_out_checking
    uint8_t* p = (uint8_t*) spi_buffer;
    p[9] = 0; // 10th bit clear the last crc value
    uint8_t crc = 0x0;
    crc = crc_table[crc ^ *p++]; // 0 then ++
    crc = crc_table[crc ^ *p++]; // 1 then ++
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];    
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    crc = crc_table[crc ^ *p++];
    *p = crc; // 10th bit is crc value
   #endif
  
  //PORTB &= ~_BV(PB0);                           //low pin 53 slave select to arduino due
  digitalWriteFast(SS,LOW);
 
  uint8_t sreg = SREG;                          // save registers
  cli();                                        // turn interrupts off //noInterrupts();
//  uint8_t clockDiv = 0;                         // 8000000 mhz
//  clockDiv ^= 0x1;                              // Invert the SPI2X bit
                                                // Pack into the SPISettings class
//  SPCR = _BV(SPE) | _BV(MSTR) | ((MSBFIRST == LSBFIRST) ? _BV(DORD) : 0) | (SPI_MODE0 & SPI_MODE_MASK) | ((clockDiv >> 1) & SPI_CLOCK_MASK);
//  SPSR = clockDiv & SPI_2XCLOCK_MASK;

#ifdef unrolled_loop
  asm volatile ( // go to an unrolled pure assembly... that was rough

          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 1
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 2
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 3
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 4
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 5
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 6
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 7
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 8
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 9
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte
                                                //     21   - total cpu cycles for duplex spi 10
                                                //     210  - cpu cycles to complete 10 byte duplex transfer

         :                                      // Outputs:
          [buf]   "+e" (spi_buffer)             // pointer to buffer 
          
         :                                      // Inputs:
          "[buf]" (spi_buffer),                 // pointer to buffer   
          [spdr]  "I" (_SFR_IO_ADDR(SPDR))    
         
         :                                      // Clobbers
          "cc"                                  // special name that indicates that flags may have been clobbered
          
    );

#else
  uint16_t len = size_data;
  asm volatile ( 
        "LOOP_LEN_%=:           \n\t"           // Cycles
                                                // ======
          "ld __tmp_reg__,%a[buf]  \n\t"        //     2    - load next byte 
          "out %[spdr],__tmp_reg__  \n\t"       //     1    - transmit byte start
          "rjmp   .+0               \n\t"       //     2    - cpu count to 17
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2 
          "rjmp   .+0               \n\t"       //     2
          "nop                      \n\t"       //     1
          "in __tmp_reg__,%[spdr]   \n\t"       //     2    - receive next byte 17 required cpu cycles
          "st %a[buf]+,__tmp_reg__  \n\t"       //     1    - save byte               
          "sbiw %[len], 1           \n\t"       //     2 
          "brne LOOP_LEN_%=         \n\t"       //     2
          
        :                                       // Outputs
         [buf]   "+e" (spi_buffer),             // pointer to buffer 
         [len]   "+w" (len)                     // length of buffer
        
        :                                       // Inputs
         "[buf]" (spi_buffer),                  // pointer to buffer
         "[len]" (len),                         // length of buffer
         [spdr]  "I" (_SFR_IO_ADDR(SPDR))       // SPI data register
        
        :                                       // Clobbers
         "cc"                                   // special name that indicates that flags may have been clobbered 
        
    );
#endif

 //PORTB |= _BV(PB0);                             // high pin 53 slave select
 digitalWriteFast(SS,HIGH);
 
 SREG = sreg;                                   // reload registers
 sei();                                         // turn interrupts back on
}

ISR(TIMER1_COMPA_vect) {                        // timer compare interrupt service routine
  digitalWriteFast(indicator , led_on);
  led_on != led_on;  
  do_it = true;
 }
 
void setup() {
 #ifdef debug
  Serial.begin(500000);
 #endif
  pinModeFast(SS, OUTPUT);                         // needed right before 
  //pinModeFast(53, OUTPUT);                         // if using no standerd pin 53 for ss
  SPI.begin ();                                    // put here for cold start issues
  uint8_t clockDiv = 0;                            // 8000000 mhz
  clockDiv ^= 0x1;                                 // Invert the SPI2X bit                                           // Pack into the SPISettings class
  SPCR = _BV(SPE) | _BV(MSTR) | ((MSBFIRST == LSBFIRST) ? _BV(DORD) : 0) | (SPI_MODE0 & SPI_MODE_MASK) | ((clockDiv >> 1) & SPI_CLOCK_MASK);
  SPSR = clockDiv & SPI_2XCLOCK_MASK;
  
  enable_timer(true,timer_rate);                   // enable timer for sending data flag 
}

void loop() {
  if(do_it){                                // spi send flag
   do_it = false;                           // clear it
   
   enable_timer(false,timer_rate);          // turn timer back off
   
  #ifdef debug      
   Serial.print(F("tx:"));
   print_buffer();
  #endif
  
   for(int8_t bob = 0; bob < sizeof(spi_buffer); bob++){ // load buffer before calling
    spi_buffer[bob] = bob;
   }
   
   assemSpiTransfTen(spi_buffer);            // send it flying

  #ifdef crc_in_checking
   uint8_t crc_rx = spi_buffer[5];           // read buffer after done, check for good data
   if(crc_rx != crc85(spi_buffer)){ 
    //fail
   }else{
    //good
   }
  #endif

  #ifdef debug      
   Serial.print(F("rx:"));
   print_buffer();
  #endif
   
  enable_timer(true,timer_rate);             // turn timer back on
  }
}
