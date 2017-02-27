/* nrf24l01p.cpp -- Handle communication with the nrf24l01+ chip.
 *
 * Copyright (C) 2014 Alexandre Clienti
 * Copyright (C) 2016 Suxsem
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "nrf24l01p.h"

static uint8_t rf_setup;

nrf24l01p::nrf24l01p()
{
  rf_setup = 0x0F;
}

nrf24l01p::~nrf24l01p()
{
  /* EMPTY */
}

void nrf24l01p::setPins(uint8_t cePin, uint8_t csPin)
{
  mCePin = cePin;
  mCsPin = csPin;
  pinMode(mCePin,OUTPUT);
  pinMode(mCsPin,OUTPUT);
  setCeLow();
  setCsHigh();
}

void nrf24l01p::setPwr(uint8_t power)
{
  mPower = power;
}
  
void nrf24l01p::init(uint8_t payloadSize)
{
  // Initialize SPI
  SPI.begin();
  mPayloadSize = payloadSize;
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}


void nrf24l01p::rxMode()
{
  setCeLow();
  writeRegister(CONFIG, _BV(EN_CRC) | _BV(CRCO));	// Enable CRC (2bytes)
  delayMicroseconds(100);
  writeRegister(EN_AA, 0x00);		// Disable auto acknowledgment
  writeRegister(EN_RXADDR, 0x01);	// Enable first data pipe
  writeRegister(SETUP_AW, 0x03);	// 5 bytes address
  writeRegister(SETUP_RETR, 0xFF);	// 15 retransmit, 4000us pause
  writeRegister(RF_CH, 0x00);		// channel 8
  setBitrate(NRF24L01_BR_250K);
  setPower(mPower);
  writeRegister(STATUS, 0x70);		// Clear status register
  writeRegister(RX_PW_P0, 0x0A);	// RX payload of 10 bytes
  writeRegister(FIFO_STATUS, 0x00);	// Nothing useful for write command
  delay(50);
  flushTx();
  flushRx();
  delayMicroseconds(100);
  writeRegister(CONFIG, _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP)  );
  delayMicroseconds(100);
  writeRegister(CONFIG, _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP) | _BV(PRIM_RX) );
  delayMicroseconds(130);
  setCeHigh();
  delayMicroseconds(100);
}

void nrf24l01p::txMode()
{
	setCeLow();
	writeRegister(STATUS, 0x70);	// Clear status register
	// Swith to TX mode
	writeRegister(CONFIG, (1 << EN_CRC) | (1 << CRCO) | (1 << PWR_UP));
	delayMicroseconds(130);
	setCeHigh();
	delayMicroseconds(100);
}

uint8_t nrf24l01p::readRegister(uint8_t reg)
{
  setCsLow();
  SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  uint8_t result = SPI.transfer(0xff);

  setCsHigh();
  return result;
}

uint8_t nrf24l01p::writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  setCsLow();
  uint8_t result = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    SPI.transfer(*buf++);

  setCsHigh();

  return result;
}

uint8_t nrf24l01p::writeRegister(uint8_t reg, uint8_t value)
{
  setCsLow();
  uint8_t result = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  SPI.transfer(value);
  setCsHigh();

  return result;
}

uint8_t nrf24l01p::setAddress(const uint8_t* buf, uint8_t len)
{
  return writeRegister(RX_ADDR_P0, buf, len);
}

uint8_t nrf24l01p::readPayload(void* buf, uint8_t len)
{
  uint8_t result;
  uint8_t* current = reinterpret_cast<uint8_t*>(buf);

  uint8_t data_len = min(len,mPayloadSize);
  uint8_t blank_len = mPayloadSize - data_len;
  
  setCsLow();
  result = SPI.transfer( R_RX_PAYLOAD );
  while ( data_len-- )
    *current++ = SPI.transfer(0xff);
  while ( blank_len-- )
    SPI.transfer(0xff);
  setCsHigh();

  return result;
}

uint8_t nrf24l01p::flushRx(void)
{
  setCsLow();
  uint8_t result = SPI.transfer( FLUSH_RX );
  setCsHigh();

  return result;
}

uint8_t nrf24l01p::flushTx(void)
{
  setCsLow();
  uint8_t result = SPI.transfer( FLUSH_TX );
  setCsHigh();

  return result;
}

uint8_t nrf24l01p::setBitrate(uint8_t bitrate) {
    // Note that bitrate 250kbps (and bit RF_DR_LOW) is valid only
    // for nRF24L01+. There is no way to programmatically tell it from
    // older version, nRF24L01, but the older is practically phased out
    // by Nordic, so we assume that we deal with with modern version.

    // Bit 0 goes to RF_DR_HIGH, bit 1 - to RF_DR_LOW
    rf_setup = (rf_setup & 0xD7) | ((bitrate & 0x02) << 4) | ((bitrate & 0x01) << 3);
    return writeRegister(RF_SETUP, rf_setup);
}

uint8_t nrf24l01p::setPower(uint8_t power) {
 uint8_t nrf_power = 0;
    switch(power) {
        case PWRLOW: nrf_power = 0; break;
        case PWRMEDIUM:   nrf_power = 1; break;
        case PWRHIGH:  nrf_power = 2; break;
        case PWRMAX: nrf_power = 3; break;
        default:            nrf_power = 0; break;
    };
    // Power is in range 0..3 for nRF24L01
    rf_setup = (rf_setup & 0xF9) | ((nrf_power & 0x03) << 1);
    return writeRegister(RF_SETUP, rf_setup);
}

