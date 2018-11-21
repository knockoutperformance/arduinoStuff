/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * Copyright (c) 2016 TMRh20 <tmrh20@gmail.com>
 * Copyright (c) 2018 KO <knockoutperformance@gmail.com>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef _SPI_UART_H_INCLUDED
#define _SPI_UART_H_INCLUDED

#include <stdio.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR


class SPIUARTClass {

public:

 SPIUARTClass(Usart* spiUart);
  
 inline byte transfer(byte _data);
 inline void transfer(void *_buf, size_t _count);
 inline byte fTransfer(byte _data);
  // SPI Configuration methods

 void attachInterrupt();
 void detachInterrupt(); // Default

 void begin(); // Default
 void end();

 static void setBitOrder(uint8_t);
 void setDataMode(uint8_t);
 void setClockDivider(uint8_t);
  
protected:
  Usart* SPIUart;
};


void SPIUARTClass::transfer(void *_buf, size_t _count){
  uint8_t *d = (uint8_t*)_buf;
  if(_count == 1){
      *d = transfer(*d);
  }
  SPIUart->US_RPR = SPIUart->US_TPR = (uint32_t)_buf;
  SPIUart->US_TCR = SPIUart->US_RCR = _count;
  while( SPIUart->US_RNCR > 0 || SPIUart->US_RCR > 0 ){;} 
}

byte SPIUARTClass::fTransfer(byte _data) {
  while( !(SPIUart->US_CSR & US_CSR_TXRDY) ){;} 
  SPIUart->US_THR = _data;
  while(  !(SPIUart->US_CSR & US_CSR_RXRDY) ){;} 
  return SPIUart->US_RHR;
}

byte SPIUARTClass::transfer(byte _data) {
  while( !(SPIUart->US_CSR & US_CSR_TXRDY) ){;} 
  SPIUart->US_THR = _data;
  while(  !(SPIUart->US_CSR & US_CSR_RXRDY) ){;} 
  return SPIUart->US_RHR;
}

#endif
