/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "countdown.h"
#include "time.h"

fr12_countdown::fr12_countdown(uint32_t timestamp) {
  this->timestamp = timestamp;
  this->days = this->hours = this->mins = this->secs = this->millis = 0;
  this->reached = 0;
}

fr12_countdown::~fr12_countdown() {
  
}

void fr12_countdown::update(fr12_time *time) {
  // Take the difference between now and the timestamp
  uint32_t s = this->timestamp - time->now(), m, h;
  
  // We WANT this counter to roll over (one below zero) - then we're sure that the countdown is done
  if (s == 0xffffffff) {
    this->days = this->hours = this->mins = this->secs = this->millis = 0;
    this->reached = 1;
    return;
  }
  
  this->secs = s % 60;
  m = s / 60;
  this->mins = m % 60;
  h = m / 60;
  this->hours = h % 24;
  this->days = h / 24;
  this->millis = 1000 - time->time_millis;
}

uint32_t fr12_countdown::get_timestamp() {
  return this->timestamp;
}

uint8_t fr12_countdown::target_reached() {
  return this->reached;
}
