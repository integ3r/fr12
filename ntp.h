/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */
 
#ifndef FR12_NTP_H
#define FR12_NTP_H

#include "defs.h"

#include "../../../../libraries/SPI/SPI.h"
#include "../../../../libraries/Ethernet/Dhcp.h"
#include "../../../../libraries/Ethernet/Dns.h"
#include "../../../../libraries/Ethernet/Ethernet.h"
#include "../../../../libraries/Ethernet/EthernetClient.h"
#include "../../../../libraries/Ethernet/EthernetServer.h"
#include "../../../../libraries/Ethernet/EthernetUdp.h"
#include "../../../../libraries/Ethernet/util.h"

// Various constants
enum {
  fr12_ntp_local_port = 8888,
  fr12_ntp_hostname_size = 32,
  fr12_ntp_packet_size = 48,
  fr12_ntp_seventy_years = 2208988800UL
};

// FR 12 classes
class fr12_ntp;

// Serialization structs
struct fr12_ntp_serialized;

class fr12_ntp {
public:
  // Constructor
  fr12_ntp();
  
  // Destructor
  virtual ~fr12_ntp();
  
  // Starts up NTP with a DNS server
  void begin(IPAddress &dns);
  
  // Sends a packet
  void send_packet();
  
  // Gets the time
  uint32_t get_time();
  
  // Configuration
  void configure(fr12_ntp_serialized *ee);
  void serialize(fr12_ntp_serialized *ee);
  
  // Getters
  const char *get_hostname();
  IPAddress get_ip();
private:
  uint8_t hostname[fr12_ntp_hostname_size];
  uint8_t buffer[fr12_ntp_packet_size];
  IPAddress addr;
  EthernetUDP udp;
};

#endif /* FR12_NTP_H */
