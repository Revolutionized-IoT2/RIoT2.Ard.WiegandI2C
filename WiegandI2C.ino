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
    sei();
    counter = 0;
    hasNewData = false;
    buffer = 0ULL;
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
    counter++;
  }

  //D1 falling edge while D0 high
  if (!(PINB & (1<<WGD_D1)) && (PINB & (1<<WGD_D0))) {
    buffer <<= 1;
    buffer++;
    counter++;
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

    //convert buffer to uint8_t array
    uint8_t *p = (uint8_t *)&buffer;
  
    uint8_t result[3]; //only take first 3 bytes
    for(int i = 0; i < 3; i++) {
      result[i] = p[i];
    }
    
    TinyWireS.send(result[2]);
    TinyWireS.send(result[1]);
    TinyWireS.send(result[0]);

    buffer = 0ULL;
    hasNewData = false;
    counter = 0;
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
        hasNewData = false;
        return;
    }

    codeReceived();
}

void codeReceived() {
    // remove parity bits from the buffer
    buffer >>= 1;
    buffer &= ~_BV_ULL(counter - 2);

    // raise interrupt and tell data is available
    hasNewData = true;
    WGD_OUT_REG |= _BV(WGD_IRQ);
}
