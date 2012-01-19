/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#ifndef FR12_UNION_STATION_H
#define FR12_UNION_STATION_H

#include "defs.h"

// Mixed variables
enum {
  // Countdown
  fr12_union_station_countdown_to = 1327626000L,
  
  // Configuration write interval
  fr12_union_station_config_write_interval = 5,
  
  // NTP sync interval
  fr12_union_station_ntp_sync_interval = 3600,
  
  // NTP try timeout
  fr12_union_station_ntp_timeout = 1000,
  
  // Max NTP tries
  fr12_union_station_sync_max_tries = 5
};

// Flags
enum {
  fr12_union_station_time_inaccurate = (1 << 0),
  fr12_union_station_colon = (1 << 1)
};

// Built-in classes
class EthernetClient;

// FR 12 classes
class fr12_config;
class fr12_lcd;
class fr12_glcd;
class fr12_net;
class fr12_ntp;
class fr12_time;
class fr12_countdown;

// Serialization structs
struct fr12_union_station_serialized;

class fr12_union_station {
public:
  friend class fr12_config;
  
  // Constructor
  fr12_union_station();
  
  // Destructor
  virtual ~fr12_union_station();
  
  // Called by sketch start routines
  void setup();
  void loop();
  
  // Configuration
  void configure(fr12_union_station_serialized *ee);
  void serialize(fr12_union_station_serialized *ee);
  
  // Handlers
  uint32_t sync_handler(fr12_time *time);
  void http_handler(fr12_net *net, EthernetClient *client, char *path);
private:
  // Utilities
  void do_status_reset();
  void do_sync_ntp();
  
  // HTTP queries
  char *do_find_query(char *str);
  void do_break_query(char *str, char **key, char **value);
  
  // Synchronization index
  uint16_t sync_index;
protected:
  // Pointers to all FR 12 components
  fr12_config *config;
  fr12_lcd *lcd;
  fr12_glcd *glcd;
  fr12_net *net;
  fr12_ntp *ntp;
  fr12_time *time;
  fr12_countdown *countdown;
  
  // Global flags
  uint8_t flags;
};

#endif /* FR12_UNION_STATION_H */
