#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(7,6);

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

const int payload_size = 2;

char payload[payload_size+1];

void radio_setup() {
  radio.begin();

  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(payload_size);
  radio.setRetries(5,15);
  radio.setChannel(100);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
}

/*
 * Rapido y feito
 *  ST -> Start, transiciona a ST_NORMAL
 *  TM -> Retorna los minutos
 *  An -> Agrega n*5 minutos
 *  
 */

int handle_radio() {
  int rt = -1;
  while ( radio.available() ) {
    radio.read(payload, payload_size);
    payload[payload_size] = 0;
  
    //Serial.print("Got message=");
    Serial.println(payload);

    if (!strcmp(payload, "ST")) {
      rt = 1; // Definir reset en algun lado
      strcpy(payload, "OK");
      Serial.println("ST");
    } else if (!strcmp(payload, "TM")) {
      int m = fclock_ticks() / 60;
      payload[0] = (m/10) + '0';
      payload[1] = (m%10) + '0';
      payload[2] = 0;
    } else if (payload[0] == 'A') {
      int ticks = (payload[1] - '0') * 10 * 60 + fclock_ticks();
      fclock_reset(ticks);
      strcpy(payload, "TA");
    } else {
      strcpy(payload, "??");
    }
  
    radio.stopListening();
    
    delay(100);
  
    //strcpy(payload, "OK");
    radio.write(payload, 2);
    //Serial.println(F("Sent response."));
    delay(100);
    
    radio.startListening();
  }
  return rt;
}


