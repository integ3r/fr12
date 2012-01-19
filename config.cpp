/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "config.h"

#include "union_station.h"
#include "time.h"
#include "ntp.h"
#include "net.h"
#include "lcd.h"

fr12_config::fr12_config(fr12_union_station *union_station) {
  this->union_station = union_station;
}

fr12_config::~fr12_config() {

}

void fr12_config::begin() {
  // Step 1: Read the header.
  fr12_eeprom_header *h = this->read_header();

  // Step 2: Verify the magic bytes and version.
  if (h->magic != fr12_config_magic || h->version != fr12_config_version) {
    this->reset();
  }

  // Step 3: Free memory.
  free(h);

  // Step 4: Configure stuff.
  fr12_union_station_serialized *us = this->read_union_station();
  this->union_station->configure(us);
  free(us);

  fr12_net_serialized *net = this->read_net();
  this->union_station->net->configure(net);
  free(net);

  fr12_ntp_serialized *ntp = this->read_ntp();
  this->union_station->ntp->configure(ntp);
  free(ntp);

  fr12_time_serialized *time = this->read_time();
  this->union_station->time->configure(time);
  free(time);
}

void fr12_config::reset() {
  static fr12_eeprom ee PROGMEM = {
    // Magic
    fr12_config_magic,

    // Version
    fr12_config_version,

    // Union Station
    { fr12_union_station_countdown_to },

    // LCD
    { fr12_lcd_auto },

    // Network
    {
      0x00,
      { 0x72, 0x65, 0x64, 0x64, 0x69, 0x74 },
      IPAddress(192, 168, 23, 100),
      IPAddress(192, 168, 24, 84),
      IPAddress(192, 168, 23, 1),
      IPAddress(255, 255, 255, 0)
    },

    // NTP
    { "pool.ntp.org" },

    // Time
    {
      fr12_time_default,
      1UL
    }
  };

  // Copy from PROGMEM to EEPROM
  for (uint32_t a = 0; a < sizeof(ee); a++) {
    _EEPUT(a, pgm_read_byte(a + (const prog_char *)&ee));
  }
}

fr12_eeprom_header *fr12_config::read_header() {
  return (fr12_eeprom_header *)this->read(sizeof(fr12_eeprom_header), offsetof(fr12_eeprom, header));
}

fr12_union_station_serialized *fr12_config::read_union_station() {
  return (fr12_union_station_serialized *)this->read(sizeof(fr12_union_station_serialized), offsetof(fr12_eeprom, union_station));
}

fr12_lcd_serialized *fr12_config::read_lcd() {
  return (fr12_lcd_serialized *)this->read(sizeof(fr12_lcd_serialized), offsetof(fr12_eeprom, lcd));
}

fr12_net_serialized *fr12_config::read_net() {
  return (fr12_net_serialized *)this->read(sizeof(fr12_net_serialized), offsetof(fr12_eeprom, net));
}

fr12_ntp_serialized *fr12_config::read_ntp() {
  return (fr12_ntp_serialized *)this->read(sizeof(fr12_ntp_serialized), offsetof(fr12_eeprom, ntp));
}

fr12_time_serialized *fr12_config::read_time() {
  return (fr12_time_serialized *)this->read(sizeof(fr12_time_serialized), offsetof(fr12_eeprom, time));
}

void fr12_config::write_header(fr12_eeprom_header *header) {
  this->write((uint8_t *)header, sizeof(fr12_eeprom_header), offsetof(fr12_eeprom, header));
}

void fr12_config::write_union_station(fr12_union_station_serialized *union_station) {
  this->union_station->configure(union_station);
  this->write((uint8_t *)union_station, sizeof(fr12_union_station_serialized), offsetof(fr12_eeprom, union_station));
}

void fr12_config::write_lcd(fr12_lcd_serialized *lcd) {
  this->union_station->lcd->configure(lcd);
  this->write((uint8_t *)lcd, sizeof(fr12_lcd_serialized), offsetof(fr12_eeprom, lcd));
}

void fr12_config::write_net(fr12_net_serialized *net) {
  this->union_station->net->configure(net);
  this->write((uint8_t *)net, sizeof(fr12_net_serialized), offsetof(fr12_eeprom, net));
}

void fr12_config::write_ntp(fr12_ntp_serialized *ntp) {
  this->union_station->ntp->configure(ntp);
  this->write((uint8_t *)ntp, sizeof(fr12_ntp_serialized), offsetof(fr12_eeprom, ntp));
}

void fr12_config::write_time(fr12_time_serialized *time) {
  this->union_station->time->configure(time);
  this->write((uint8_t *)time, sizeof(fr12_time_serialized), offsetof(fr12_eeprom, time));
}

uint8_t *fr12_config::read(size_t len, size_t offset) {
  uint8_t *ptr = (uint8_t *)malloc(len);

  if (ptr == NULL) {
    return NULL;
  }

  for (register size_t a = 0; a < len; a++) {
    _EEGET(ptr[a], a + offset);
  }

  return ptr;
}

void fr12_config::write(uint8_t *ptr, size_t len, size_t offset) {
  for (register size_t a = 0; a < len; a++) {
    _EEPUT(a + offset, ptr[a]);
  }
}


