/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#ifndef FR12_NET_H
#define FR12_NET_H

#include "defs.h"

#include "../../../../libraries/SPI/SPI.h"
#include "../../../../libraries/Ethernet/Dhcp.h"
#include "../../../../libraries/Ethernet/Dns.h"
#include "../../../../libraries/Ethernet/Ethernet.h"
#include "../../../../libraries/Ethernet/EthernetClient.h"
#include "../../../../libraries/Ethernet/EthernetServer.h"
#include "../../../../libraries/Ethernet/EthernetUdp.h"
#include "../../../../libraries/Ethernet/util.h"

// Built-in classes
class EthernetClass;
class EthernetServer;
class EthernetClient;

// FR 12 classes
class fr12_net;
class fr12_config;

// Serialization structs
struct fr12_net_serialized;

// HTTP handler callback
typedef void (fr12_union_station::*fr12_http_callback)(EthernetClient *, char *);

// Defaults
enum {
  fr12_net_http_port = 80,
  fr12_net_http_min_buffer_len = 64,
  fr12_net_http_max_buffer_len = 256,
};

// Flags
enum {
  fr12_net_use_dhcp = (1 << 0)
};

class fr12_net {
public:
  // Constructor
  fr12_net();
  
  // Destructor
  virtual ~fr12_net();
  
  // Initializers
  void begin(fr12_union_station *union_station);
  
  // Starts up Ethernet
  uint8_t begin_ethernet_dhcp();
  void begin_ethernet_static();
  
  // Starts up HTTP
  void begin_http(fr12_http_callback handler);
  
  // Gets addresses, subnet mask, etc
  void get_addresses(IPAddress *addresses);
  uint8_t *get_mac();
  uint8_t get_flags();
  
  // Configuration
  void configure(fr12_net_serialized *ee);
  void serialize(fr12_net_serialized *ee);
  
  // HTTP utilities
  void handle_http();
  void http_respond(EthernetClient *client, uint16_t response_code, const char *data = NULL, size_t data_length = 0, const char **headers = NULL, size_t header_length = 0);
  void http_respond_json(EthernetClient *client, uint16_t response_code, const char **data = NULL, size_t data_length = 0, const char **headers = NULL, size_t header_length = 0);
  void http_unescape(char *s);
private:
  void http_send_headers(EthernetClient *client, uint16_t response_code, const char *content_type, const char **headers = NULL, size_t header_length = 0);
  int http_unhex(char c);
  
  static const uint16_t http_codes[] PROGMEM;
  static const char http_response_ok[] PROGMEM;
  static const char http_response_bad_request[] PROGMEM;
  static const char http_response_forbidden[] PROGMEM;
  static const char http_response_not_found[] PROGMEM;
  static const char http_response_too_large[] PROGMEM;
  static const char http_response_server_error[] PROGMEM;
  static const char *http_responses[] PROGMEM;
  
  // Ethernet members
  uint8_t flags;
  uint8_t mac[6];
  IPAddress ip, dns, gateway, subnet;
  EthernetClass *hw;
  fr12_union_station *union_station;
  
  // HTTP members
  EthernetServer *http;
  uint8_t *http_buffer;
  size_t http_buffer_len, http_buffer_index;
  fr12_http_callback http_handler;
};

#endif /* FR12_NET_H */
