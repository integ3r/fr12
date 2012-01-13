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
  Serial.begin(115200);
  fr12 = new fr12_union_station();
  if (fr12) {
    fr12->setup();
  } else {
    abort();
  }
}

void loop() {
  fr12->loop();
}
