/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "time.h"
#include "union_station.h"
#include "config.h"

fr12_time::fr12_time() {
  this->time_seconds = 0;
  this->time_millis = this->prev_millis = 0;
  this->sync_interval = this->next_sync = 0;
  this->auto_sync = NULL;
  this->union_station = NULL;
  this->flags = 0x00;
}

fr12_time::~fr12_time() {
  
}

void fr12_time::set(uint32_t now) {
  this->time_seconds = now;
  this->prev_millis = millis();
  this->next_sync = this->time_seconds + this->sync_interval;
}

void fr12_time::set_sync_interval(uint32_t sync_interval) {
  this->sync_interval = sync_interval;
  this->next_sync = this->time_seconds + this->sync_interval;
}

void fr12_time::set_auto_sync(fr12_union_station *union_station, fr12_time_callback callback) {
  this->auto_sync = callback;
  this->union_station = union_station;
  this->flags |= fr12_time_auto_sync;
}

void fr12_time::set_auto_sync(fr12_union_station *union_station, fr12_time_callback callback, uint32_t interval) {
  this->set_auto_sync(union_station, callback);
  this->set_sync_interval(interval);
}

void fr12_time::update() {
  // Find the current time in seconds and milliseconds
  while (millis() - this->prev_millis >= 1000) {
    this->time_seconds++;
    this->prev_millis += 1000;
  }
  this->time_millis = millis() % this->prev_millis;
  
  // Determine if we need to sync
  if (this->sync_interval > 0 && this->time_seconds >= this->next_sync && !(this->flags & fr12_time_should_sync)) {
    this->flags |= fr12_time_should_sync;
    
    // If auto sync is enabled, do so
    if (this->flags & fr12_time_auto_sync && this->auto_sync != NULL && this->union_station != NULL) {
      // Sync and set the time
      this->time_seconds = ((this->union_station)->*(this->auto_sync))(this);
      
      // Set the next sync time
      this->next_sync = this->time_seconds + this->sync_interval;
      
      // Clear sync flag
      this->flags &= ~fr12_time_should_sync;
    }
  }
}

uint32_t fr12_time::now() {
  return this->time_seconds;
}

uint32_t fr12_time::get_sync_interval() {
  return this->sync_interval;
}

uint8_t fr12_time::get_flags() {
  return this->flags;
}

void fr12_time::configure(fr12_time_serialized *ee) {
  this->set_sync_interval(ee->sync_interval);
  this->set(ee->seconds);
}

void fr12_time::serialize(fr12_time_serialized *ee) {
  ee->seconds = this->time_seconds;
  ee->sync_interval = this->sync_interval;
}
