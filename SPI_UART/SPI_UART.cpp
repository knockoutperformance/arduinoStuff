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

#include "pins_arduino.h"
#include "SPI_UART.h"

SPIUARTClass::SPIUARTClass(Usart *spiUart){
  SPIUart = spiUart;
}

void SPIUARTClass::begin() {
  if(SPIUart == USART0){
    //Arduino Due Pins: 18: MOSI, 19: MISO, SDA1: SCK, CS: User Selected, CE: User Selected
    PIOA->PIO_ABSR |= (1u << 17);   // SCK: Assign A16 I/O to the Peripheral B function
    PIOA->PIO_PDR |= (1u << 17);    // SCK: Disable PIO control, enable peripheral control
    PIOA->PIO_ABSR |= (0u << 10);   // MOSI: Assign PA13 I/O to the Peripheral A function
    PIOA->PIO_PDR |= (1u << 10);    // MOSI: Disable PIO control, enable peripheral control
    PIOA->PIO_ABSR |= (0u << 11);   // MISO: Assign A12 I/O to the Peripheral A function
    PIOA->PIO_PDR |= (1u << 11);    // MISO: Disable PIO control, enable peripheral control
    pmc_enable_periph_clk(ID_USART0);
  }else if(SPIUart == USART1){
    //Arduino Due Pins: 16 tx2: MOSI, 17 rx2: MISO, A0: SCK, CS: User Selected, CE: User Selected
    PIOA->PIO_ABSR |= (0u << 16);   // SCK: Assign A16 I/O to the Peripheral A function
    PIOA->PIO_PDR |= (1u << 16);    // SCK: Disable PIO control, enable peripheral control
    PIOA->PIO_ABSR |= (0u << 13);   // MOSI: Assign PA13 I/O to the Peripheral A function
    PIOA->PIO_PDR |= (1u << 13);    // MOSI: Disable PIO control, enable peripheral control
    PIOA->PIO_ABSR |= (0u << 12);   // MISO: Assign A12 I/O to the Peripheral A function
    PIOA->PIO_PDR |= (1u << 12);    // MISO: Disable PIO control, enable peripheral control    
    pmc_enable_periph_clk(ID_USART1);
    }  
  //Set USART to Master mode
  SPIUart->US_MR = 0x409CE;
  SPIUart->US_BRGR = 4; // 21mhz
  SPIUart->US_CR = US_CR_RSTRX | US_CR_RSTTX;
  SPIUart->US_CR = US_CR_RXEN;
  SPIUart->US_CR = US_CR_TXEN;
  SPIUart->US_PTCR = US_PTCR_RXTEN;
  SPIUart->US_PTCR = US_PTCR_TXTEN;
}

void SPIUARTClass::end() {
  SPIUart->US_CR = US_CR_RXDIS;
  SPIUart->US_CR = US_CR_TXDIS;
  if(SPIUart == USART0){
    PIOA->PIO_PDR &= ~(1u << 11);
    PIOA->PIO_PDR &= ~(1u << 10);
    PIOA->PIO_PDR &= ~(1u << 17);
  }else if(SPIUart == USART1){  
    PIOA->PIO_PDR &= ~(1u << 13);
    PIOA->PIO_PDR &= ~(1u << 12);
    PIOA->PIO_PDR &= ~(1u << 16);
  }
}


void SPIUARTClass::attachInterrupt() {

}

void SPIUARTClass::detachInterrupt() {

}

void SPIUARTClass::setBitOrder(uint8_t bitOrder){

}


void SPIUARTClass::setDataMode(uint8_t mode){
  if(mode == 0){
    SPIUart->US_MR |= 1 << 8;
  }else
  if(mode == 4){
    SPIUart->US_MR &= ~(1 << 8);  
  }
}

void SPIUARTClass::setClockDivider(uint8_t rate){
  SPIUart->US_BRGR = rate;
}
