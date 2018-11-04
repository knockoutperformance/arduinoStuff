/*arduino due in slave mode at 8mhz
 * i struggled on this one for a long time to get it right and stable
 */

#include <SPI.h>
#define SS 10
#define BUFF_SIZE 10
#define debug

uint8_t rx_buffer[BUFF_SIZE];
uint8_t tx_buffer[BUFF_SIZE];
volatile bool done = false;

/** Use extra Bus Matrix arbitration fix if nonzero */
#define USE_SAM3X_BUS_MATRIX_FIX 1
/** chip select register number */
//#define SPI_CHIP_SEL 0
/** DMAC receive channel */
#define SPI_DMAC_RX_CH  1
/** DMAC transmit channel */
#define SPI_DMAC_TX_CH  0
/** DMAC Channel HW Interface Number for SPI TX. */
#define SPI_TX_IDX  1
/** DMAC Channel HW Interface Number for SPI RX. */
#define SPI_RX_IDX  2

//------------------------------------------------------------------------------
static void dmac_disable() {/** Disable DMA Controller. */
  DMAC->DMAC_EN &= (~DMAC_EN_ENABLE);
}
static void dmac_enable() {/** Enable DMA Controller. */
  DMAC->DMAC_EN = DMAC_EN_ENABLE;
}
static void dmac_channel_disable(uint32_t ul_num) {/** Disable DMA Channel. */
  DMAC->DMAC_CHDR = DMAC_CHDR_DIS0 << ul_num;
}
static void dmac_channel_enable(uint32_t ul_num) {/** Enable DMA Channel. */
  DMAC->DMAC_CHER = DMAC_CHER_ENA0 << ul_num;
}
uint32_t dmac_get_status(){
  return DMAC->DMAC_EBCISR;
}

// start RXTX DMA
void spiDmaRXTX(const uint8_t* spi_buf, const uint8_t* spi_buf2, uint16_t count) {
  dmac_channel_disable(SPI_DMAC_RX_CH);
  dmac_channel_disable(SPI_DMAC_TX_CH);

  DMAC->DMAC_CH_NUM[SPI_DMAC_TX_CH].DMAC_SADDR = (uint32_t)spi_buf;
  DMAC->DMAC_CH_NUM[SPI_DMAC_TX_CH].DMAC_DADDR = (uint32_t)&SPI0->SPI_TDR;
  DMAC->DMAC_CH_NUM[SPI_DMAC_TX_CH].DMAC_DSCR =  0;
  DMAC->DMAC_CH_NUM[SPI_DMAC_TX_CH].DMAC_CTRLA = count | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE;
  DMAC->DMAC_CH_NUM[SPI_DMAC_TX_CH].DMAC_CTRLB =  DMAC_CTRLB_SRC_DSCR | DMAC_CTRLB_DST_DSCR | DMAC_CTRLB_FC_MEM2PER_DMA_FC | DMAC_CTRLB_SRC_INCR_INCREMENTING | DMAC_CTRLB_DST_INCR_FIXED;

  DMAC->DMAC_CH_NUM[SPI_DMAC_RX_CH].DMAC_SADDR = (uint32_t)&SPI0->SPI_RDR;
  DMAC->DMAC_CH_NUM[SPI_DMAC_RX_CH].DMAC_DADDR = (uint32_t)spi_buf2;
  DMAC->DMAC_CH_NUM[SPI_DMAC_RX_CH].DMAC_DSCR =  0;
  DMAC->DMAC_CH_NUM[SPI_DMAC_RX_CH].DMAC_CTRLA = count | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE;
  DMAC->DMAC_CH_NUM[SPI_DMAC_RX_CH].DMAC_CTRLB = DMAC_CTRLB_SRC_DSCR | DMAC_CTRLB_DST_DSCR | DMAC_CTRLB_FC_PER2MEM_DMA_FC | DMAC_CTRLB_SRC_INCR_FIXED | DMAC_CTRLB_DST_INCR_INCREMENTING;// | DMA_CFG_IE | DMA_CCR_PL_VERY_HIGH;

  dmac_channel_enable(SPI_DMAC_RX_CH);
  dmac_channel_enable(SPI_DMAC_TX_CH);
}

void enable_dma(){
  pmc_enable_periph_clk(ID_DMAC);
  dmac_disable();
  DMAC->DMAC_GCFG = DMAC_GCFG_ARB_CFG_FIXED;
  //DMAC->DMAC_GCFG = DMAC_PRIORITY_ROUND_ROBIN;
  dmac_enable();
#if USE_SAM3X_BUS_MATRIX_FIX
  MATRIX->MATRIX_WPMR = 0x4d415400;
  MATRIX->MATRIX_MCFG[1] = 1;
  MATRIX->MATRIX_MCFG[2] = 1;
  MATRIX->MATRIX_SCFG[0] = 0x01000010;
  MATRIX->MATRIX_SCFG[1] = 0x01000010;
  MATRIX->MATRIX_SCFG[7] = 0x01000010;
#endif  // USE_SAM3X_BUS_MATRIX_FIX

  DMAC->DMAC_CH_NUM[SPI_DMAC_RX_CH].DMAC_CFG = DMAC_CFG_SRC_PER(SPI_RX_IDX) | DMAC_CFG_SRC_H2SEL | DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ALAP_CFG;  
  DMAC->DMAC_CH_NUM[SPI_DMAC_TX_CH].DMAC_CFG = DMAC_CFG_DST_PER(SPI_TX_IDX) | DMAC_CFG_DST_H2SEL | DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ALAP_CFG;

  NVIC_EnableIRQ(DMAC_IRQn);

  DMAC->DMAC_EBCIER = (1 << SPI_DMAC_RX_CH);
  spiDmaRXTX(tx_buffer,rx_buffer,BUFF_SIZE); // initial loading to get things rolling
  }

void slaveDMABegin(uint8_t _pin) {  
  pinMode(_pin,INPUT);                                            // for slave mode
  pinMode(SCK,INPUT);                                           // for slave mode

  SPI.begin(_pin);
  //SPI.beginTransaction(_pin,SPISettings(0, MSBFIRST, SPI_MODE0));
  REG_SPI0_CR = SPI_CR_SWRST;     // reset SPI
  REG_SPI0_CR = SPI_CR_SPIEN;     // enable SPI
  REG_SPI0_MR = 0x02;     // slave and no modefault
  REG_SPI0_CSR = 0x00;    // DLYBCT=0, DLYBS=0, SCBR=0, 8 bit transfer
}



void DMAC_Handler(){
  static uint32_t ul_status;
  ul_status = dmac_get_status();
  
  if (ul_status & (1 << SPI_DMAC_RX_CH)) {
#ifdef debug // only print when debuginng
    String strIn;              
    for(int iIdx=0; iIdx<sizeof(rx_buffer); iIdx++){
      strIn += (String(rx_buffer[iIdx]) + ",");
    }
    Serial.println("Received data");
    Serial.println(strIn);
#endif
    done = true;
    spiDmaRXTX(tx_buffer,rx_buffer,BUFF_SIZE); // reload for next time around
  }
}


void setup() {
  Serial.begin(457900);
  Serial.println("Starting setup");   
  slaveDMABegin(SS);
  enable_dma();
}

void loop() {
}
