/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */
 
#ifndef FR12_COUNTDOWN_H
#define FR12_COUNTDOWN_H

#include "defs.h"

// FR 12 classes
class fr12_time;
class fr12_countdown;

class fr12_countdown {
public:
  // Constructor, with timestamp
  fr12_countdown(uint32_t timestamp);
  
  // Destructor
  virtual ~fr12_countdown();
  
  // Updates to a time object
  void update(fr12_time *time);
  
  // Gets the timestamp
  uint32_t get_timestamp();
  
  // Returns true if the target has been reached
  uint8_t target_reached();
  
  // Cached data
  uint16_t days, hours, mins, secs, millis;
private:
  // Whether we've reached our mark or not
  uint8_t reached;
  
  // Timestamp
  uint32_t timestamp;
};

#endif /* FR12_COUNTDOWN_H */
