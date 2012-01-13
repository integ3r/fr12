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
    {
      fr12_union_station_countdown_to
    },

    // LCD
    {
      {
        fr12_lcd_auto | fr12_lcd_random,
        fr12_lcd_interval
      },
      {
        {
          "Hoist the sails",
          89,
          19,
          174
        },
        {
          "Bring on the horizon",
          41,
          199,
          23
        },
        {
          "Get pumped!",
          182,
          9,
          149
        },
        {
          "Shine on!",
          180,
          133,
          86
        },
        {
          "WSSAM?",
          237,
          242,
          207
        },
        {
          "Actual size!",
          40,
          191,
          84
        },
        {
          "Check it out!",
          153,
          98,
          229
        },
        {
          "Good luck class of 2015!",
          153,
          19,
          180
        },
        {
          "Amazing!",
          241,
          150,
          150
        },
        {
          "Begin a new journey.",
          97,
          47,
          75
        },
        {
          "January 27th!",
          28,
          30,
          165
        },
        {
          "Approved by Tim Tebow!",
          56,
          91,
          208
        },
        {
          "Mind-blowing!",
          169,
          184,
          83
        },
        {
          "Not available in stores!",
          12,
          235,
          196
        },
        {
          "Awesome!",
          184,
          61,
          225
        },
        {
          "Message 16",
          120,
          80,
          28
        },
        {
          "Message 17",
          212,
          245,
          21
        },
        {
          "Message 18",
          50,
          32,
          48
        },
        {
          "Message 19",
          60,
          161,
          78
        },
        {
          "Message 20",
          47,
          246,
          79
        },
        {
          "Message 21",
          102,
          132,
          208
        },
        {
          "Message 22",
          237,
          88,
          99
        },
        {
          "Message 23",
          25,
          216,
          224
        },
        {
          "Message 24",
          16,
          154,
          12
        },
        {
          "Message 25",
          195,
          232,
          79
        },
        {
          "Message 26",
          248,
          92,
          141
        },
        {
          "Message 27",
          11,
          176,
          81
        },
        {
          "Message 28",
          55,
          12,
          38
        },
        {
          "Message 29",
          200,
          47,
          252
        },
        {
          "Message 30",
          222,
          98,
          137
        },
        {
          "Message 31",
          83,
          245,
          169
        },
        {
          "Message 32",
          157,
          21,
          241
        },
        {
          "Message 33",
          133,
          230,
          249
        },
        {
          "Message 34",
          193,
          48,
          112
        },
        {
          "Message 35",
          30,
          206,
          132
        },
        {
          "Message 36",
          236,
          57,
          193
        },
        {
          "Message 37",
          85,
          11,
          166
        },
        {
          "Message 38",
          236,
          23,
          112
        },
        {
          "Message 39",
          153,
          62,
          208
        },
        {
          "Message 40",
          36,
          212,
          79
        },
        {
          "Message 41",
          77,
          39,
          249
        },
        {
          "Message 42",
          147,
          6,
          91
        },
        {
          "Message 43",
          11,
          9,
          73
        },
        {
          "Message 44",
          11,
          111,
          37
        },
        {
          "Message 45",
          2,
          139,
          222
        },
        {
          "Message 46",
          204,
          199,
          37
        },
        {
          "Message 47",
          249,
          217,
          139
        },
        {
          "Message 48",
          243,
          223,
          245
        },
        {
          "Message 49",
          27,
          61,
          148
        },
        {
          "Message 50",
          68,
          112,
          46
        }
      }
    }
    ,

    // Network
    {
      0x00,
      { 
        0x72, 0x65, 0x64, 0x64, 0x69, 0x74
      },
      { 
        192, 168, 23, 100
      },
      { 
        192, 168, 24, 84
      },
      { 
        192, 168, 23, 1
      },
      { 
        255, 255, 255, 0
      }
    },

    // NTP
    {
      "pool.ntp.org"
    },

    // Time
    {
      fr12_time_default,
      1L
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

fr12_lcd_serialized_header *fr12_config::read_lcd() {
  return (fr12_lcd_serialized_header *)this->read(sizeof(fr12_lcd_serialized_header), offsetof(fr12_eeprom, lcd) + offsetof(fr12_lcd_serialized, header));
}

fr12_lcd_message *fr12_config::read_lcd_message(size_t index) {
  return (fr12_lcd_message *)this->read(sizeof(fr12_lcd_message), offsetof(fr12_eeprom, lcd) + offsetof(fr12_lcd_serialized, messages) + sizeof(fr12_lcd_message) * index);
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

void fr12_config::write_lcd(fr12_lcd_serialized_header *lcd) {
  this->union_station->lcd->configure(lcd);
  this->write((uint8_t *)lcd, sizeof(fr12_lcd_serialized_header), offsetof(fr12_eeprom, lcd) + offsetof(fr12_lcd_serialized, header));
}

void fr12_config::write_lcd_message(fr12_lcd_message *msg, size_t index) {
  this->write((uint8_t *)msg, sizeof(fr12_lcd_message), offsetof(fr12_eeprom, lcd) + offsetof(fr12_lcd_serialized, messages) + sizeof(fr12_lcd_message) * index);
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

