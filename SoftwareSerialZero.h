/*
  SoftwareSerial.h (formerly NewSoftSerial.h) -
  Multi-instance software serial library for Arduino/Wiring
  -- Interrupt-driven receive and other improvements by ladyada
   (http://ladyada.net)
  -- Tuning, circular buffer, derivation from class Print/Stream,
   multi-instance support, porting to 8MHz processors,
   various optimizations, PROGMEM delay tables, inverse logic and
   direct port writing by Mikal Hart (http://www.arduiniana.org)
  -- Pin change interrupt macros by Paul Stoffregen (http://www.pjrc.com)
  -- 20MHz processor support by Garrett Mace (http://www.macetech.com)
  -- ATmega1280/2560 support by Brett Hagman (http://www.roguerobotics.com/)
  -- SAMD21 support by Ben Rose (http://www.tinyCricuits.com/)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The latest version of this library can always be found at
  http://arduiniana.org.
*/
#if defined(ARDUINO_ARCH_SAMD)
#ifndef SoftwareSerial_h
#define SoftwareSerial_h

#include <inttypes.h>
#include <Stream.h>

/******************************************************************************
  Definitions
******************************************************************************/

#define _SS_MAX_RX_BUFF 256 // RX buffer size

class SoftwareSerial : public Stream
{
  private:
    // per object data
    uint8_t _receivePin;
    uint32_t _receiveBitMask;
    volatile uint32_t *_receivePortRegister;
    uint32_t _transmitBitMask;
    volatile uint32_t *_transmitPortRegister;

    // Expressed as 4-cycle delays (must never be 0!)
    uint16_t _rx_delay_centering;
    uint16_t _rx_delay_intrabit;
    uint16_t _rx_delay_stopbit;
    uint16_t _tx_delay;

    uint16_t _buffer_overflow: 1;
    uint16_t _inverse_logic: 1;

    // static data
    static uint8_t _receive_buffer[_SS_MAX_RX_BUFF];
    static volatile uint8_t _receive_buffer_tail;
    static volatile uint8_t _receive_buffer_head;
    static SoftwareSerial *active_object;

    // private methods
    inline void recv() __attribute__((__always_inline__));
    uint32_t rx_pin_read();
    void setTX(uint8_t transmitPin);
    void setRX(uint8_t receivePin);
    inline void setRxIntMsk(bool enable) __attribute__((__always_inline__));

    // Return num - sub, or 1 if the result would be < 1
    static uint16_t subtract_cap(uint32_t num, uint32_t sub);

    // private static method for timing
    static inline void tunedDelay(volatile uint32_t delay);

  public:
    // public methods
    SoftwareSerial(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic = false);
    ~SoftwareSerial();
    void begin(long speed);
    bool listen();
    void end();
    bool isListening() {
      return this == active_object;
    }
    bool stopListening();
    bool overflow() {
      bool ret = _buffer_overflow;
      if (ret) _buffer_overflow = false;
      return ret;
    }
    int peek();

    virtual size_t write(uint8_t byte);
    virtual int read();
    virtual int available();
    virtual void flush();
    operator bool() {
      return true;
    }

    using Print::write;

    // public only for easy access by interrupt handlers
    static inline void handle_interrupt() __attribute__((__always_inline__));
};

#endif
#endif
