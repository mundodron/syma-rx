/* nrf24l01p.h -- Handle communication with the nrf24l01+ chip.
 *
 * Copyright (C) 2014 Alexandre Clienti
 * Copyright (C) 2016 Suxsem
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef NRF24_L01P_H_
#define NRF24_L01P_H_

#include "Arduino.h"
#include <SPI.h> 	//must be included in the ino file (where the main is)

/* Memory Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define FIFO_STATUS 0x17

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define RX_DR       6
#define TX_DS       5
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

//      Raw            w 20dBm PA
// 0 : -18dBm  (16uW)   2dBm (1.6mW)
// 1 : -12dBm  (60uW)   8dBm   (6mW)
// 2 :  -6dBm (250uW)  14dBm  (25mW)
// 3 :   0dBm   (1mW)  20dBm (100mW)
enum rfPower
{
  PWRLOW = 0, //
  PWRMEDIUM,
  PWRHIGH,
  PWRMAX
};

// Bitrates
enum {
    NRF24L01_BR_1M = 0,
    NRF24L01_BR_2M,
    NRF24L01_BR_250K,
    NRF24L01_BR_RSVD
};

class nrf24l01p
{
public :
  nrf24l01p();
  ~nrf24l01p();
  void setPins(uint8_t cePin, uint8_t csPin);
  void setPwr(uint8_t power = PWRLOW);
  void init(uint8_t payloadSize);
  inline void setCeLow() {digitalWrite(mCePin,LOW);} 
  inline void setCeHigh() {digitalWrite(mCePin,HIGH);} 
  inline void setCsLow() {digitalWrite(mCsPin,LOW);} 
  inline void setCsHigh() {digitalWrite(mCsPin,HIGH);}
  inline void switchFreq(uint8_t freq){writeRegister(RF_CH, freq);}
  inline bool rxFlag(){return (readRegister(STATUS) & _BV(RX_DR));} 
  inline void resetRxFlag(){writeRegister(STATUS, _BV(RX_DR));}
  inline bool rxEmpty(){return (readRegister(FIFO_STATUS) & _BV(RX_EMPTY));} 
  uint8_t readPayload(void* buf, uint8_t len);
  void rxMode();
  void txMode();
  uint8_t flushRx();
  uint8_t flushTx();
  uint8_t setAddress(const uint8_t* buf, uint8_t len);
  
protected:  
  uint8_t readRegister(uint8_t reg, uint8_t* buf, uint8_t len);
  uint8_t readRegister(uint8_t reg);
  uint8_t writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len);
  uint8_t writeRegister(uint8_t reg, uint8_t value);
  uint8_t setBitrate(uint8_t bitrate);
  uint8_t setPower(uint8_t power);

  uint8_t mPower;
  uint8_t mCePin;
  uint8_t mCsPin;
  uint8_t mPayloadSize;
};

#endif /* NRF24_L01P_H_ */
