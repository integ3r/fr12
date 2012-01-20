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
  fr12_union_station_sync_max_tries = 5,
  
  // Reset pin (high)
  fr12_union_station_reset_pin = 12,
  
  // Heartbeat pin
  fr12_union_station_heartbeat_pin = 13
};

// Flags
enum {
  fr12_union_station_time_inaccurate = (1 << 0),
  fr12_union_station_colon = (1 << 1)
};

// Built-in classes
class EthernetClient;

// FR 12 classes
class fr12_union_station;
class fr12_config;
class fr12_lcd;
class fr12_glcd;
class fr12_net;
class fr12_ntp;
class fr12_time;
class fr12_countdown;

// Serialization structs
struct fr12_union_station_serialized;

// HTTP set callback
typedef void (fr12_union_station::*fr12_union_station_http_get_callback)(void *, EthernetClient *);
typedef void (fr12_union_station::*fr12_union_station_http_set_callback)(void *, void *, char *, char *);

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
  void sync_handler();
  void http_handler(EthernetClient *client, char *path);
  
private:
  // HTTP getters
  template <typename T, typename U> void http_get(fr12_union_station_http_get_callback callback, T *module, EthernetClient *client) {
    U var;
    module->serialize(&var);
    ((this)->*(callback))(&var, client);
  }
  
  void http_get_countdown(void *ee, EthernetClient *client);
  void http_get_lcd(void *ee, EthernetClient *client);
  void http_get_net(void *ee, EthernetClient *client);
  void http_get_ntp(void *ee, EthernetClient *client);
  void http_get_time(void *ee, EthernetClient *client);
  
  // HTTP setters
  template <typename T, typename U> void http_set(fr12_union_station_http_set_callback callback, T *module, char *query) {
    U var, old_var;
    module->serialize(&var);
    memcpy(&old_var, &var, sizeof(U));
    query = strtok(this->do_find_query(query), "&");
    
    while (query != NULL) {
      char *key, *value;
      this->do_break_query(query, &key, &value);
      ((this)->*(callback))(&var, &old_var, key, value);
      query = strtok(NULL, "&");
    }
  }
  
  void http_set_countdown(void *ee_new, void *ee_old, char *key, char *value);
  void http_set_lcd(void *ee_new, void *ee_old, char *key, char *value);
  void http_set_net(void *ee_new, void *ee_old, char *key, char *value);
  void http_set_ntp(void *ee_new, void *ee_old, char *key, char *value);
  void http_set_time(void *ee_new, void *ee_old, char *key, char *value);
  
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
