/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "ntp.h"
#include "config.h"

fr12_ntp::fr12_ntp() {
  memset(&this->hostname, 0x00, fr12_ntp_hostname_size);
}

fr12_ntp::~fr12_ntp() {
  
}

void fr12_ntp::begin(IPAddress &dns) {
  DNSClient dns_client;
  dns_client.begin(dns);
  int err = dns_client.getHostByName((const char *)&this->hostname, this->addr);
  if (err != 1) {
    memcpy_P(&this->hostname, PSTR("time.nist.gov"), 28);
    this->addr = IPAddress(192, 43, 244, 18); // time.nist.gov
  }
  this->udp.begin(fr12_ntp_local_port);
}

// send an NTP request to the time server at the given address 
void fr12_ntp::send_packet() {
  // set all bytes in the buffer to 0
  memset(&this->buffer, 0x00, fr12_ntp_packet_size); 

  this->buffer[0] = 0b11100011;   // LI, Version, Mode
  this->buffer[1] = 0;     // Stratum, or type of clock
  this->buffer[2] = 6;     // Polling Interval
  this->buffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  this->buffer[12]  = 49; 
  this->buffer[13]  = 0x4E;
  this->buffer[14]  = 49;
  this->buffer[15]  = 52;

  // all NTP fields have been given values, now	   
  this->udp.beginPacket(this->addr, 123); // NTP requests are to port 123
  this->udp.write(this->buffer, fr12_ntp_packet_size);
  this->udp.endPacket(); 
}

uint32_t fr12_ntp::get_time() {
  if (this->udp.parsePacket()) {
    // Read the UDP packet
    this->udp.read(this->buffer, fr12_ntp_packet_size);
    
    // Return the seconds since 1900 minus 70 years (to get the unix timestamp)
    uint32_t hi = word(this->buffer[40], this->buffer[41]);
    uint32_t lo = word(this->buffer[42], this->buffer[43]);
    uint32_t secs_since_1900 = hi << 16 | lo;
    uint32_t unix = secs_since_1900 - fr12_ntp_seventy_years;
    
    return unix;
  }

  return 0;
}

void fr12_ntp::configure(fr12_ntp_serialized *ee) {
  memcpy(&this->hostname, ee->server, sizeof(ee->server));
}

void fr12_ntp::serialize(fr12_ntp_serialized *ee) {
  memcpy(ee->server, &this->hostname, sizeof(ee->server));
}

const char *fr12_ntp::get_hostname() {
  return (const char *)&this->hostname;
}

IPAddress fr12_ntp::get_ip() {
  return this->addr;
}

