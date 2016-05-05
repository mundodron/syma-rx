/* symax_protocol.h -- Handle the symax protocol.
 *
 * Copyright (C) 2014 Alexandre Clienti
 * Copyright (C) 2016 Suxsem
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef SYMAX_PROTOCOL_H_
#define SYMAX_PROTOCOL_H_

#include "Arduino.h"
#include "nrf24l01p.h"

#define PSIZE 10 // packet size
#define FSIZE 4 // frequency hop count

typedef struct __attribute__((__packed__)) {
  uint8_t throttle; // 0...255
  int8_t yaw; // 127...-127
  int8_t pitch; // 127...-127
  int8_t roll; // 127...-127
  int8_t trim_yaw; // 31...-31
  int8_t trim_pitch; // 31...-31
  int8_t trim_roll; // 31...-31
  bool video;
  bool picture;
  bool highspeed;
  bool flip;  
} rx_values_t;

enum rxState
{
  NO_BIND = 0,
  WAIT_FIRST_SYNCHRO,
  BOUND,
};


enum rxReturn
{
  BOUND_NEW_VALUES = 0,   // Bound state, frame received with new TX values
  BOUND_NO_VALUES,        // Bound state, no new frame received
  NOT_BOUND,              // Not bound, initial state
  BIND_IN_PROGRESS,       // Bind in progress, first frame has been received with TX id, wait no bind frame
  UNKNOWN                 // ???, not used for moment
};

class symaxProtocol
{
public:
  symaxProtocol();
  ~symaxProtocol();

  void init(nrf24l01p *wireless);
  uint8_t run(rx_values_t *rx_value );
  
protected:
  uint8_t checksum(uint8_t *data);
  void setRFChannel(uint8_t address);
  
  nrf24l01p *mWireless;
  uint8_t mRfChNum;
  uint8_t mFrame[PSIZE];
  uint8_t mRFChanBufs[FSIZE];
  uint8_t mState;
  unsigned long mLastSignalTime;

};
#endif /* SYMAX_PROTOCOL_H_ */
