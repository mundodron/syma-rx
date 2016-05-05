/* symax_protocol.cpp -- Handle the symax protocol.
 *
 * Copyright (C) 2014 Alexandre Clienti
 * Copyright (C) 2016 Suxsem
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "symax_protocol.h"

const uint8_t bind_rx_tx_addr[] = {0xab, 0xac, 0xad, 0xae, 0xaf}; // bind addr
const uint8_t chans_bind[] = {0x4b, 0x30, 0x40, 0x20}; // bind chan

symaxProtocol::symaxProtocol()
{
    mState = NO_BIND;
    mLastSignalTime = 0;
    mRfChNum = 0;
}

symaxProtocol::~symaxProtocol()
{
    /*EMPTY*/
}

uint8_t symaxProtocol::checksum(uint8_t *data)
{
    uint8_t sum = data[0];
    
    for (int i=1; i < PSIZE-1; i++)
    sum ^= data[i];
    
    return sum + 0x55;
}

void symaxProtocol::init(nrf24l01p *wireless)
{
    mWireless = wireless;
    mWireless->init(PSIZE);
    delayMicroseconds(100);
    mWireless->rxMode();
    mWireless->setAddress(bind_rx_tx_addr, 5);
    mWireless->switchFreq(chans_bind[0]);
    mLastSignalTime = millis();
}

// loop function, can be factorized (for later)
uint8_t symaxProtocol::run( rx_values_t *rx_value )
{
    uint8_t returnValue = UNKNOWN;
    
    switch(mState)
    {
        case BOUND:
        {
            unsigned long newTime = millis();
            returnValue = BOUND_NO_VALUES;
            if( !mWireless->rxFlag() )
            {
                // Signal lost
                if((newTime - mLastSignalTime) > 4000)
                {
                    mWireless->setAddress(bind_rx_tx_addr, 5);
                    mWireless->switchFreq(chans_bind[0]);
                    mState = NO_BIND;
                    mLastSignalTime = newTime; 
                }
            }
            else
            {
                bool incrementChannel = false;
                mWireless->resetRxFlag();
                while ( !mWireless->rxEmpty() )
                {
                    mWireless->readPayload(mFrame, PSIZE);
                    if( checksum(mFrame) == mFrame[PSIZE-1] )
                    {
                        // a valid frame has been received
                        incrementChannel = true;

                        // Discard bind frame
                        if( mFrame[5] != 0xAA && mFrame[6] != 0xAA )
                        {
                            // Extract values
                            returnValue = BOUND_NEW_VALUES;
                            rx_value->throttle = mFrame[0];
                            
                            rx_value->yaw = mFrame[2];
                            if (rx_value->yaw < 0)
                              rx_value->yaw = 128 - rx_value->yaw;
                              
                            rx_value->pitch = mFrame[1];
                            if (rx_value->pitch < 0)
                              rx_value->pitch = 128 - rx_value->pitch;
                              
                            rx_value->roll = mFrame[3];
                            if (rx_value->roll < 0)
                              rx_value->roll = 128 - rx_value->roll;
                              
                            rx_value->trim_yaw = mFrame[6] & 0x3F;
                            if (rx_value->trim_yaw >= 32)
                              rx_value->trim_yaw = 32 - rx_value->trim_yaw;

                            rx_value->trim_pitch = mFrame[5] & 0x3F;
                            if (rx_value->trim_pitch >= 32)
                              rx_value->trim_pitch = 32 - rx_value->trim_pitch;

                            rx_value->trim_roll = mFrame[7] & 0x3F;
                            if (rx_value->trim_roll >= 32)
                              rx_value->trim_roll = 32 - rx_value->trim_roll;

                            rx_value->video = mFrame[4] & 0x80;
                            rx_value->picture = mFrame[4] & 0x40;
                            rx_value->highspeed = mFrame[5] & 0x80;
                            rx_value->flip = mFrame[6] & 0x40;

                            mLastSignalTime = newTime;
                        }
                    }
                }
                if(incrementChannel)
                {
                    mRfChNum++;
                    if( mRfChNum >= FSIZE)
                    mRfChNum = 0;
                    mWireless->switchFreq(mRFChanBufs[mRfChNum]);
                }
            }
            
        }
        break;
        // Initial state
        case NO_BIND:
        {
            returnValue = NOT_BOUND;
            unsigned long newTime = millis();
            
            if( !mWireless->rxFlag() )
            {
                // Wait 128ms before switching the frequency
                // 128ms is the time to a TX to emits on all this frequency
                if((newTime - mLastSignalTime) > 128) //TODO it's 128?
                {
                    mRfChNum++;
                    if(mRfChNum >= FSIZE)
                    mRfChNum = 0;
                    mWireless->switchFreq(chans_bind[mRfChNum]);
                    mLastSignalTime = newTime;
                }
            }
            else
            {
                
                mWireless->resetRxFlag();
                bool bFrameOk = false;
                while ( !mWireless->rxEmpty() )
                {
                    mWireless->readPayload(mFrame, PSIZE);
                    if(checksum(mFrame) == mFrame[PSIZE-1] && mFrame[5] == 0xAA && mFrame[6] == 0xAA)
                    {
                        
                        // Bind frame is OK
                        
                        uint8_t txAddr[5];
                        for (int k=0; k<5; k++)
                        txAddr[k] = mFrame[4-k];
                        
                        // Create TX frequency array
                        
                        mWireless->setAddress(txAddr, 5);
                        
                        setRFChannel(txAddr[0]);
                        mRfChNum = 0;
                        mWireless->switchFreq(mRFChanBufs[mRfChNum]);
                        
                        mLastSignalTime = newTime;
                        mState = WAIT_FIRST_SYNCHRO;
                        mWireless->flushRx();
                        returnValue = BIND_IN_PROGRESS;
                        
                        break;
                    }
                }
            }
        }
        break;
        
        // Wait on the first frequency of TX
        case WAIT_FIRST_SYNCHRO:
        {
            
            unsigned long newTime = millis();
            returnValue = BIND_IN_PROGRESS;
            if( mWireless->rxFlag() )
            {
                mWireless->resetRxFlag();
                bool incrementChannel = false;
                while ( !mWireless->rxEmpty() )
                {
                    mWireless->readPayload(mFrame, PSIZE);
                    
                    if( checksum(mFrame) == mFrame[PSIZE-1] )
                    {
                        incrementChannel = true;
                        mState = BOUND;
                        mLastSignalTime = newTime;
                    }
                }
                
                if(incrementChannel)
                {
                    // switch channel
                    mRfChNum++;
                    if( mRfChNum >= FSIZE)
                    mRfChNum = 0;
                    mWireless->switchFreq(mRFChanBufs[mRfChNum]);
                }
            }
            
            break;
        }
        
        default:
        break;
    }
    
    return returnValue;
}

static const PROGMEM u8 START_CHANS_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
static const PROGMEM u8 START_CHANS_2[] = {0x2a, 0x0a, 0x42, 0x22};
static const PROGMEM u8 START_CHANS_3[] = {0x1a, 0x3a, 0x12, 0x32};

// channels determined by last byte of tx address
void symaxProtocol::setRFChannel(uint8_t address)
{
    uint8_t laddress = address & 0x1f;
    uint8_t i;
    uint32_t *pchans = (uint32_t *)mRFChanBufs;  // avoid compiler warning
    
    if (laddress < 0x10) {
        if (laddress == 6)
        laddress = 7;
        for(i=0; i < FSIZE; i++) {
            mRFChanBufs[i] = pgm_read_byte(START_CHANS_1 + i) + laddress;
        }
        } else if (laddress < 0x18) {
        for(i=0; i < FSIZE; i++) {
            mRFChanBufs[i] = pgm_read_byte(START_CHANS_2 + i) + (laddress & 0x07);
        }
        if (laddress == 0x16) {
            mRFChanBufs[0] += 1;
            mRFChanBufs[1] += 1;
        }
        } else if (laddress < 0x1e) {
        for(i=0; i < FSIZE; i++) {
            mRFChanBufs[i] = pgm_read_byte(START_CHANS_3 + i) + (laddress & 0x07);
        }
        } else if (laddress == 0x1e) {
        *pchans = 0x38184121;
        } else {
        *pchans = 0x39194121;
    }
}
