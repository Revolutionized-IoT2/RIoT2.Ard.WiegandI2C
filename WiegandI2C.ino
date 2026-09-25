 #include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "TinyWireS.h"                  

/**
 * Wiegand I/O
 * These are parameterized should we want to support other MCUs
 */
#define WGD_DIR_REG             DDRB
#define WGD_OUT_REG             PORTB
#define WGD_IN_REG              PINB
#define WGD_D0                  PB3
#define WGD_D1                  PB4
#define WGD_IRQ                 PB1
#define WGD_PCINT_D0            PCINT3
#define WGD_PCINT_D1            PCINT4
#define I2C_SLAVE_ADDR          0x26 

/** Timeouts to end listening for a Wiegand transmission */
#define T1_OFFSET                0
#define T1_PRESCALE             ((1 << CS13) | (1 << CS12) | (1 << CS11))

/** Just like _BV() but for 64-bit integers */
#define _BV_ULL(b)              (1ULL<<(b))

volatile uint8_t counter;
volatile uint64_t buffer;
volatile bool hasNewData;
volatile uint32_t completedCode;
volatile uint32_t droppedFrameCount;

uint8_t countBits(uint64_t value, uint8_t firstBit, uint8_t bitCount) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < bitCount; ++i) {
        if (value & _BV_ULL(firstBit - i)) ++count;
    }
    return count;
}

bool hasValidWiegand26Parity(uint64_t frame) {
    // Wiegand26 layout: P_even, 24 data bits, P_odd. The leading parity bit
    // makes bits 25..13 even; the trailing parity bit makes bits 12..0 odd.
    return (countBits(frame, 25, 13) % 2 == 0) && (countBits(frame, 12, 13) % 2 == 1);
}

void setup() {
  //i2c setup
    TinyWireS.begin(I2C_SLAVE_ADDR); 
    TinyWireS.onRequest(requestEvent);
  
  // Wiegand Setup
    WGD_DIR_REG |= _BV(WGD_IRQ);                 // Outputs
    WGD_DIR_REG &= ~(_BV(WGD_D0) | _BV(WGD_D1)); // Inputs
    WGD_OUT_REG &= ~_BV(WGD_IRQ);              // Pull-ups

    PCMSK |= _BV(WGD_PCINT_D0) | _BV(WGD_PCINT_D1); // PCINT0 enable
    GIMSK |= _BV(PCIE); // Pin Change Interrupt Enable
    TIMSK |= _BV(TOIE1); // Timer0 Overflow Interrupt Enable

    TCCR1 = 0; // initialize timer, stopped
    
    // Initialize our state
    counter = 0;
    hasNewData = false;
    buffer = 0ULL;
    completedCode = 0;
    droppedFrameCount = 0;
    sei();
}

void loop() {
  //Do nothing in loop...
}

/**
 * Pin Change0 Interrupt
 *
 * Register incomming bits from the Wiegand reader
 */
ISR(PCINT0_vect)
{
  
  TCNT1 = T1_OFFSET;
  TCCR1 = T1_PRESCALE;

  //D0 falling edge while D1 high
  if (!(PINB & (1<<WGD_D0)) && (PINB & (1<<WGD_D1))) {
    buffer <<= 1;
    if (counter < 27) counter++;
  }

  //D1 falling edge while D0 high
  if (!(PINB & (1<<WGD_D1)) && (PINB & (1<<WGD_D0))) {
    buffer <<= 1;
    buffer++;
    if (counter < 27) counter++;
  }

/* This does not work for some reason
  if(counter == 26) { //26 bit received -> we can stop the backup timer and indicate that the code has been received
    TCCR1 = 0;
    codeReceived();
  }*/
}

void requestEvent()
{  
    //turn off interrupt
    WGD_OUT_REG &= ~_BV(WGD_IRQ);

    //send 0 if no new value is available
    if(!hasNewData) {
      TinyWireS.send(0);
      TinyWireS.send(0);
      TinyWireS.send(0);
      return;
    }

    TinyWireS.send(static_cast<uint8_t>(completedCode >> 16));
    TinyWireS.send(static_cast<uint8_t>(completedCode >> 8));
    TinyWireS.send(static_cast<uint8_t>(completedCode));

    completedCode = 0;
    hasNewData = false;
}

/**
 * Timer1 Overflow Interrupt
 *
 * Used to timeout the Wiegand transmission
 */
ISR(TIM1_OVF_vect)
{
    // Turn off timer
    TCCR1 = 0;

    //only support 26bit wiegand
    if (counter != 26) {
        counter = 0;
        buffer = 0ULL;
        return;
    }

    if (!hasValidWiegand26Parity(buffer)) {
        counter = 0;
        buffer = 0ULL;
        return;
    }

    codeReceived();
}

void codeReceived() {
    if (hasNewData) {
        if (droppedFrameCount != UINT32_MAX) ++droppedFrameCount;
    } else {
        completedCode = static_cast<uint32_t>((buffer >> 1) & 0xFFFFFFULL);
        hasNewData = true;
        WGD_OUT_REG |= _BV(WGD_IRQ);
    }
    counter = 0;
    buffer = 0ULL;
}

// Local diagnostic API; the existing three-byte I2C response is unchanged.
uint32_t wiegandDroppedFrames() {
    uint8_t savedSreg = SREG;
    cli();
    uint32_t count = droppedFrameCount;
    SREG = savedSreg;
    return count;
}
