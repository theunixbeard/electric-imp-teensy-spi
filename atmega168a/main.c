#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "string.h"
#include <stdbool.h>

void usart_init(unsigned short ubrr);
void usart_out(char ch);
char usart_in();
void spi_slave_init(void);
void myprint(volatile char*);
bool is_new_message(void);
void simple_slave_receive(void);

#define FOSC 9830400 // Frequency of Oscillation AKA Clock Rate
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1

#define SPI_BUFFER_LENGTH 500
#define MESSAGE1_LENGTH 25

volatile char spi_data[SPI_BUFFER_LENGTH];
volatile int spi_data_index; // SPI next place to write data to
volatile int processed_data_index;
bool processing_message;
int current_message_start_index;

int main(void) {
  /* insert your hardware initialization here */
  usart_init(MYUBRR);
  spi_slave_init();
  sei();
  (DDRD |= (1<<1));
  while(1){
    (PORTD |= (1<<1));
    (PORTD &= ~(1<<1));
    
    //simple_slave_receive();
    /* If there is a message to process, do so */
    /*
    if (processing_message) {
    
    } else {
      if (spi_data_index >) {

      }
    }*/
  }
  return 0;   /* never reached */
}

void spi_slave_init(void) {
  /* Set MISO output, all others input */
  /* Should be 3 for teensy, 4 for atmega 168a*/
  DDRB = (1<<4);
  /* Enable SPI */
  SPCR = (1<<SPE) | (1<<SPIE);
  spi_data_index = 0;
  processed_data_index = 0;
  current_message_start_index = 0;
  processing_message = false;
}

ISR(SPI_STC_vect) {
//void simple_slave_receive(void) {
  // while(!(SPSR & (1<<SPIF))); // For polling
  spi_data[spi_data_index++] = SPDR;
  SPDR = 'y';
/*
  if(spi_data[spi_data_index-1]  == '\0') {
    myprint(spi_data);
    //_delay_ms(5000);
  }
  if (spi_data_index == SPI_BUFFER_LENGTH) {
    spi_data_index = 0;
  }
*/
}

void myprint(volatile char *str) {
 /*
  while(*str != '\0') {
    usart_out(*str);
    str++;
  }
  */
  int i = 0;
  for (i = 0; i < 25; ++i) {
    usart_out(*(str+i));
  }
}

bool is_new_message(void) {
  /* Determine if normal case or wrap around case*/
  if (spi_data_index >= processed_data_index) {
      /* In normal case, if more than 25 forward we're good*/
    if (spi_data_index - MESSAGE1_LENGTH >= processed_data_index) {
      return true;
    } else {
      return false;
    }
  }
    /* In wrap around case */
    return true;
}

/*
usart_init - Initialize the USART port
*/
void usart_init(unsigned short ubrr) {
  UBRR0 = ubrr ; // Set baud rate
  UCSR0B |= (1 << TXEN0); // Turn on transmitter
  UCSR0B |= (1 << RXEN0); // Turn on receiver
  UCSR0C = (3 << UCSZ00); // Set for async . operation , no parity ,
  // one stop bit , 8 data bits
}
/*
usart_out - Output a byte to the USART0 port
*/
void usart_out(char ch) {
  while ((UCSR0A & (1 << UDRE0)) == 0);
  UDR0 = ch ;
}
/*
usart_in - Read a byte from the USART0 and return it
*/
char usart_in() {
  while (!(UCSR0A & (1 << RXC0)));
  return UDR0 ;
}

