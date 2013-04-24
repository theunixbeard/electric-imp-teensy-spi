#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "string.h"
#include "uart.h"
#include "stdbool.h"

// TODO: Change to proper values!!!
#define OUTLET1_CONFIG (DDRD |= (1<<5))
#define OUTLET2_CONFIG (DDRD |= (1<<6))
#define OUTLET3_CONFIG (DDRD |= (1<<7))
#define OUTLET4_CONFIG (DDRB |= (1<<0))

#define OUTLET1_ON  (PORTD |= (1<<5))
#define OUTLET1_OFF (PORTD &= ~(1<<5))
#define OUTLET2_ON  (PORTD |= (1<<6))
#define OUTLET2_OFF (PORTD &= ~(1<<6))
#define OUTLET3_ON  (PORTD |= (1<<7))
#define OUTLET3_OFF (PORTD &= ~(1<<7))
#define OUTLET4_ON  (PORTB |= (1<<0))
#define OUTLET4_OFF (PORTB &= ~(1<<0))


#define BAUD_RATE 9600

#define OUTLET_COUNT 4
#define MESSAGES_QUEUE_LENGTH 10

#define MESSAGE1_PRODUCT_KEY_LENGTH 10
#define MESSAGE1_OUTLET_NUMBER_LENGTH 4
#define MESSAGE1_STATE_LENGTH 2
#define MESSAGE1_OUTLET_ID_LENGTH 8
#define MESSAGE1_LENGTH MESSAGE1_PRODUCT_KEY_LENGTH + MESSAGE1_OUTLET_NUMBER_LENGTH + MESSAGE1_STATE_LENGTH + MESSAGE1_OUTLET_ID_LENGTH + 2
typedef struct message1 {
  char board_product_key[MESSAGE1_PRODUCT_KEY_LENGTH];
  char board_outlet_number[MESSAGE1_OUTLET_NUMBER_LENGTH];
  char state[MESSAGE1_STATE_LENGTH];
  char outlet_id[MESSAGE1_OUTLET_ID_LENGTH];
} message1_t;

void myprint(volatile char*);
void process_message(void);
void fill_in_message1(void);
void print_message1(message1_t message);
void outlets_init(void);
void set_outlets_state(void);
void send_message2(void);
// 168a specific functions
void print(char *);
void pchar(char);

// Global Variables
int messages_index;
int replied_messages_index;
message1_t messages[MESSAGES_QUEUE_LENGTH];
bool outlet_states[OUTLET_COUNT];
int atoi_length(const char*, int);

int main(void) {

  uart_init(BAUD_RATE);
  outlets_init();
  messages_index = 0;
  replied_messages_index = 0;

  uint8_t c;

	while (1) {
    /* Echo code for testing
    if (uart_available()) {
      c = uart_getchar();
      print("Byte: ");
      pchar(c);
      pchar('\n');
    }
    */
    if (uart_available()) {
      process_message();
    }
    if (messages_index != replied_messages_index) {
      print_message1(messages[replied_messages_index]);
      // Check product key match, strncmp returns 0 if equal
      if(!strncmp(messages[replied_messages_index].board_product_key, "1234567890", MESSAGE1_PRODUCT_KEY_LENGTH)) {
        int outlet_number = atoi_length(messages[replied_messages_index].board_outlet_number, MESSAGE1_OUTLET_NUMBER_LENGTH);
        if (outlet_number < OUTLET_COUNT) {
          int outlet_state = atoi_length(messages[replied_messages_index].state, MESSAGE1_STATE_LENGTH);
          if(outlet_state == 1 || outlet_state == 0) {
            outlet_states[outlet_number] = outlet_state;
            // Send message to imp
            send_message2();
          } else {
            //print("Bad outlet state\n");
          }
        } else {
          //print("Outlet number greater than number of outlets!\n");
        }
      } else {
        //print("Message for a different board\n");
      }
      replied_messages_index++;
      if(replied_messages_index >= MESSAGES_QUEUE_LENGTH) {
        replied_messages_index = 0;
      }
    }
    set_outlets_state();
	}
}

int atoi_length(const char* str, int length) {
  int ret = 0;
  int i = 0;
  for (i = 0; i < length; ++i) {
    ret = 10*ret + *str - '0'; 
    ++str;
  }
  return ret;
}

void outlets_init(void) {
  OUTLET1_CONFIG;
  OUTLET2_CONFIG;
  OUTLET3_CONFIG;
  OUTLET4_CONFIG;
  int i = 0;
  /* Replace with code to get actual states from DB !!! */
  for(i = 0; i < OUTLET_COUNT; ++i) {
    outlet_states[i] = true;
  }
  set_outlets_state();
}

void set_outlets_state(void) {
  if (outlet_states[0]) {
    OUTLET1_ON;
  } else {
    OUTLET1_OFF;
  }
  if (outlet_states[1]) {
    OUTLET2_ON;
  } else {
    OUTLET2_OFF;
  }
  if (outlet_states[2]) {
    OUTLET3_ON;
  } else {
    OUTLET3_OFF;
  }
  if (outlet_states[3]) {
    OUTLET4_ON;
  } else {
    OUTLET4_OFF;
  }

}

void myprint(volatile char *str) {
  while(*str != '\0') {
    pchar(*str++);
  }
}

void process_message(void) {
  uint8_t message_type;
  if (uart_available() < MESSAGE1_LENGTH ) {
      _delay_ms(1000); // So we have a full message to process
  }
  if (uart_available() < MESSAGE1_LENGTH ) {
    print("Something went wrong, only have partial message!\n");
  }
  // First character should indicate message type
  message_type = uart_getchar();
  if (message_type != '!') {
    print("Received unrecognized Message type!\n");
  }
  if (message_type == '!') {
    fill_in_message1();
  }
  return;
}

void fill_in_message1(void) {
  int i = 0;
  uint8_t c;
  for(i = 0; i < MESSAGE1_PRODUCT_KEY_LENGTH; ++i) {
    messages[messages_index].board_product_key[i] = uart_getchar();
  }
  for(i = 0; i < MESSAGE1_OUTLET_NUMBER_LENGTH; ++i) {
    messages[messages_index].board_outlet_number[i] = uart_getchar();
  }
  for(i = 0; i < MESSAGE1_STATE_LENGTH; ++i) {
    messages[messages_index].state[i] = uart_getchar();
  }
  for(i = 0; i < MESSAGE1_OUTLET_ID_LENGTH; ++i) {
    messages[messages_index].outlet_id[i] = uart_getchar();
  }
  c = uart_getchar();
  if (c != '\0') {
    print("Characte ending message was NOT a null... Possible Error! (message_index NOT incremented)\n");
  } else {
    messages_index++;
    if(messages_index >= MESSAGES_QUEUE_LENGTH) {
      messages_index = 0;
    }
  }
  return;
}

void print_message1(message1_t message) {
  int i = 0;
  print("Message1 Received: ");
  print("\n\tProduct Key: ");
  for(i = 0; i < MESSAGE1_PRODUCT_KEY_LENGTH; ++i) {
    pchar(message.board_product_key[i]);
  }
  print("\n\tOutlet Number: ");
  for(i = 0; i < MESSAGE1_OUTLET_NUMBER_LENGTH; ++i) {
    pchar(message.board_outlet_number[i]);
  }
  print("\n\tState: ");
  for(i = 0; i < MESSAGE1_STATE_LENGTH; ++i) {
    pchar(message.state[i]);
  }
  print("\n\tOutlet Id: ");
  for(i = 0; i < MESSAGE1_OUTLET_ID_LENGTH; ++i) {
    pchar(message.outlet_id[i]);
  }
  print("\n\n");
}

void send_message2(void) {
  
  int i = 0;
  uart_putchar('@');
  for(i = 0; i < MESSAGE1_OUTLET_ID_LENGTH; ++i) {
    uart_putchar(messages[replied_messages_index].outlet_id[i]);
  }
  for(i = 0; i < MESSAGE1_STATE_LENGTH; ++i) {
    uart_putchar(messages[replied_messages_index].state[i]);
  }
  uart_putchar('\0');
  return;
}

void pchar(char c) {
  // Do nothing for now...
  return;
}

void print(char * str) {
  // Do nothing for now...
  return;
}
