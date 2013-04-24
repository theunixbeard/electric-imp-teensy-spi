#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"
#include "string.h"

// Teensy 2.0: LED is active high
#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
  #define LED_ON		(PORTD |= (1<<6))
  #define LED_OFF		(PORTD &= ~(1<<6))
  #define LED2_ON		(PORTD |= (1<<7))
  #define LED2_OFF		(PORTD &= ~(1<<7))
#endif

#define LED2_CONFIG (DDRD |= (1<<7))
#define LED_CONFIG	(DDRD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

typedef struct message1 {
  char board_product_key[20]; // 16 chars in key
  char board_outlet_number[10]; // Allows up to 1 billion outlets per user!
  char state[5]; // Should only be 1 char
  char outlet_id[20]; // Up to 10^20 outlets can be sold!
} message1_t;

void myprint(volatile char*);
void spi_slave_init(void);
void spi_slave_receive(void);
void spi_simple_slave_receive(void);

// Global Variables
char message_type;
int field_num;
message1_t message1;
volatile char spi_data[500];
volatile int spi_data_index; // SPI next place to write data to
volatile int processed_data_index; // Main loop next message to process
/*

! message #1 - Includes:
  board_product_key (number)
    \0 null signifies end of field
  board_outlet_number (number)
    \0 null signifies end of field
  state (number, 0 or 1)
    \0 null signifies end of field
  outlet_id (number)
    \0 null signifies end of field
@
#

*/

int main(void) {

	// set for 16 MHz clock, and make sure the LED is off
	CPU_PRESCALE(0);

	// initialize the USB, but don't want for the host to
	// configure.  The first several messages sent will be
	// lost because the PC hasn't configured the USB yet,
	// but we care more about blinking than debug messages!
	usb_init();

  LED_CONFIG;
  LED2_CONFIG;
  spi_slave_init();

	// blink morse code messages!
	while (1) {
    //spi_slave_receive();
    //spi_simple_slave_receive();
    LED2_ON;
	}
}

void spi_slave_init(void) {
  /* Set MISO output, all others input */
  DDRB = (1<<3);
  /* Enable SPI */
  SPCR = (1<<SPE) | (1<<SPIE);
  spi_data_index = 0;
  processed_data_index = 0;
}

ISR(SPI_STC_vect) {
  spi_data[spi_data_index++] = SPDR;
  SPDR = 'y';
  if(spi_data[spi_data_index-1]  == '\0') {
    LED2_OFF;
    myprint(spi_data);
    //_delay_ms(5000);
  }
  
}

void myprint(volatile char *str) {
  print("In myprint!\n\n");
  int i = 0;
  while(*str != '\0') {
    ++i;
    pchar(*str);
    str++;
  }
  /*
  for(i = 0; i < 21; ++i) {
    pchar(*(str+i));
  }*/
  print("\n\ni (hex): ");
  phex16(i);
  print("\n\n*str: ");
  pchar(*str);

}


void spi_slave_receive(void) {
  LED2_ON;
  /* Wait for reception complete */
  while(!(SPSR & (1<<SPIF))); 
  if (SPDR == '!' || SPDR == '@' || SPDR == '#') {
    // Incoming message, set type
    print("Set Message type to: ");
    pchar(SPDR);
    print("\n\n");
    message_type = SPDR;
    field_num = 0;
    if (message_type == '!') {
      strcpy(message1.board_product_key, "");
      strcpy(message1.board_outlet_number, "");
      strcpy(message1.state, "");
      strcpy(message1.outlet_id, "");      
    }
    return;
  }
  if (message_type == '!') {
    // Message #1
    if (SPDR == '\0') {
      if (++field_num == 4) {
        // Finished last field, message is done
        message_type = '+';
        print("Final Message: \n");
        myprint(message1.board_product_key);
        print("\n");
        myprint(message1.board_outlet_number);
        print("\n");
        myprint(message1.state);
        print("\n");
        myprint(message1.outlet_id);
        print("\n\n");
      }
      print("Field num increase\n\n");
      return;
    }
    if (field_num == 0) {
      int length = strlen(message1.board_product_key);
      message1.board_product_key[length] = SPDR;
      message1.board_product_key[length+1] = '\0';
      print("field1\n");
      myprint(message1.board_product_key);
      print("\n");
    } else if (field_num == 1) {
      int length = strlen(message1.board_outlet_number);
      message1.board_outlet_number[length] = SPDR;
      message1.board_outlet_number[length+1] = '\0';
      print("field2\n");
      myprint(message1.board_outlet_number);
      print("\n");
    } else if (field_num == 2) {
      int length = strlen(message1.state);
      message1.state[length] = SPDR;
      message1.state[length+1] = '\0';
      print("field3\n");
      myprint(message1.state);
      print("\n");
    } else if (field_num == 3) {
      int length = strlen(message1.outlet_id);
      message1.outlet_id[length] = SPDR;
      message1.outlet_id[length+1] = '\0';
      print("field4\n");
      myprint(message1.outlet_id);
      print("\n");
    } else {
      print("ERROR: Field num is greater than 3 for message1\n\n\n");
    }
  }
  return;
}


