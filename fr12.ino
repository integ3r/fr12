/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include <stdint.h>
#include <stdlib.h>

#include <LiquidCrystal.h>

#include <glcd.h>
#include <glcd_Buildinfo.h>
#include <glcd_Config.h>

#include <SPI.h>

#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>

#include "union_station.h"

fr12_union_station *fr12 = NULL;

void setup() {
  fr12 = new fr12_union_station();
  if (fr12 != NULL) {
    fr12->setup();
  } else {
    // Oh damn. Something REALLY bad happened.
    // Enable our only voice to the outside world
    pinMode(13, OUTPUT);
    
    // Go into an existential loop.
    // NOTE: SHOULD ARDUINO BECOME SENTIENT, COVER UP LED 13
    while(1) {
      /// ... boulder rolls up
      digitalWrite(13, HIGH);
      
      // ... sisyphus waits
      delay(250);
      
      // ... boulder rolls down
      digitalWrite(13, LOW);
    } // repeat for all eternity
  }
}

void loop() {
  fr12->loop();
}
