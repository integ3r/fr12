/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */
 
#ifndef FR12_TIME_H
#define FR12_TIME_H

#include "defs.h"

// Defaults
enum {
  fr12_time_default = 946684800UL // Y2K
};

// Flags
enum {
  fr12_time_should_sync = (1 << 0),
  fr12_time_auto_sync = (1 << 1)
};

// FR 12 classes
class fr12_union_station;
class fr12_time;
class fr12_countdown;

// Serialization structs
struct fr12_time_serialized;

// Time callback handler
typedef uint32_t (fr12_union_station::*fr12_time_callback)(fr12_time *);

class fr12_time {
public:
  friend class fr12_countdown;
  
  // Constructor
  fr12_time();
  
  // Destructor
  virtual ~fr12_time();
  
  // Sets current time
  void set(uint32_t now);
  
  // Enables auto sync and changes sync interval
  void set_auto_sync(fr12_union_station *union_station, fr12_time_callback callback);
  void set_auto_sync(fr12_union_station *union_station, fr12_time_callback callback, uint32_t interval);
  void set_sync_interval(uint32_t sync_interval);
  
  // Updates the clock
  void update();
  
  // Getters
  uint32_t now();
  uint32_t get_sync_interval();
  uint8_t get_flags();
  
  // Configuration
  void configure(fr12_time_serialized *ee);
  void serialize(fr12_time_serialized *ee);
private:
  // Autosync function pointer
  fr12_time_callback auto_sync;
  
  // Pointer to Union Station
  fr12_union_station *union_station;
  
  // Sync interval and next sync
  uint32_t sync_interval, next_sync;
  
  // Previous millis() count
  uint32_t prev_millis;
protected:
  // Current time (seconds)
  uint32_t time_seconds;
  
  // Current time (left over milliseconds)
  uint16_t time_millis;
  
  // Flags
  uint8_t flags;
};

#endif /* FR12_TIME_H */
