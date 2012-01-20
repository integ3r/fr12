/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#ifndef FR12_CONFIG_H
#define FR12_CONFIG_H

#include "defs.h"

#include "lcd.h"
#include "net.h"

// FR 12 classes
class fr12_union_station;

// FR 12 structs
struct fr12_lcd_message;

// Headers
enum {
  fr12_config_magic = 0x34544c40,
  fr12_config_version = FR12_VERSION_NUMERIC
};

struct fr12_eeprom_header {
  uint32_t magic;
  uint16_t version;
}
__attribute__ ((packed));

// Union Station
struct fr12_union_station_serialized {
  uint32_t countdown_to;
}
__attribute__ ((packed));

// LCD
struct fr12_lcd_serialized {
  fr12_lcd_message msg;
}
__attribute__ ((packed));

// Net
struct fr12_net_serialized {
  uint8_t flags;
  uint8_t mac[6];
  uint32_t ip;
  uint32_t dns;
  uint32_t gateway;
  uint32_t subnet;
}
__attribute__ ((packed));

// NTP
struct fr12_ntp_serialized {
  uint8_t server[32];
}
__attribute__ ((packed));

// Time
struct fr12_time_serialized {
  uint32_t seconds;
  uint32_t sync_interval;
}
__attribute__ ((packed));

struct fr12_eeprom {
  fr12_eeprom_header header;
  fr12_union_station_serialized union_station;
  fr12_lcd_serialized lcd;
  fr12_net_serialized net;
  fr12_ntp_serialized ntp;
  fr12_time_serialized time;
}
__attribute__ ((packed));

class fr12_config {
public:
  friend class fr12_union_station;
  friend class fr12_lcd;
  
  // Constructor
  fr12_config(fr12_union_station *union_station);
  
  // Destructor
  virtual ~fr12_config();
  
  // Initializes configuration
  void begin();
  
  // Resets configuration
  void reset();
protected:
  // Readers
  fr12_eeprom_header *read_header();
  fr12_union_station_serialized *read_union_station();
  fr12_lcd_serialized *read_lcd();
  fr12_net_serialized *read_net();
  fr12_ntp_serialized *read_ntp();
  fr12_time_serialized *read_time();
  
  // Writers
  void write_header(fr12_eeprom_header *header);
  void write_union_station(fr12_union_station_serialized *union_station);
  void write_lcd(fr12_lcd_serialized *lcd);
  void write_net(fr12_net_serialized *net);
  void write_ntp(fr12_ntp_serialized *ntp);
  void write_time(fr12_time_serialized *time);
private:
  uint8_t *read(size_t len, size_t offset);
  void write(uint8_t *ptr, size_t len, size_t offset);
  fr12_union_station *union_station;
};

#endif /* FR12_CONFIG_H */


