/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "net.h"
#include "config.h"

extern EthernetClass Ethernet;

const uint16_t fr12_net::http_codes[] = {
  200, 400, 403, 404, 413, 500};
const char fr12_net::http_response_ok[] = "OK";
const char fr12_net::http_response_bad_request[] = "Bad Request";
const char fr12_net::http_response_forbidden[] = "Forbidden";
const char fr12_net::http_response_not_found[] = "Not Found";
const char fr12_net::http_response_too_large[] = "Request Entity Too Large";
const char fr12_net::http_response_server_error[] = "Internal Server Error";
const char *fr12_net::http_responses[] = {
  http_response_ok, http_response_bad_request, http_response_forbidden, http_response_not_found, http_response_too_large, http_response_server_error};

fr12_net::fr12_net() {
  this->hw = &Ethernet;
  this->http = new EthernetServer(fr12_net_http_port);
}

fr12_net::~fr12_net() {
  delete this->http;
  delete this->http_buffer;
}

void fr12_net::begin(fr12_union_station *union_station) {
  this->union_station = union_station;
}

uint8_t fr12_net::begin_ethernet_dhcp() {
  // SS pin
  pinMode(53, OUTPUT);

  // DHCP us an IP
  return this->hw->begin(mac);
}

void fr12_net::begin_ethernet_static() {
  // SS pin
  pinMode(53, OUTPUT);

  // Use a static IP
  this->hw->begin(mac, ip, dns, gateway, subnet);
}

void fr12_net::begin_http(fr12_http_callback handler) {
  // Start the HTTP server
  this->http->begin();

  // Set up the HTTP buffer
  this->http_buffer_len = fr12_net_http_min_buffer_len;
  this->http_buffer_index = 0;
  this->http_buffer = (uint8_t *)calloc(this->http_buffer_len, sizeof(uint8_t));
  this->http_handler = handler;
}

void fr12_net::get_addresses(IPAddress *addresses) {
  addresses[0] = this->hw->localIP();
  addresses[1] = this->hw->dnsServerIP();
  addresses[2] = this->hw->gatewayIP();
  addresses[3] = this->hw->subnetMask();
}

uint8_t *fr12_net::get_mac() {
  return (uint8_t *)&this->mac;
}

uint8_t fr12_net::get_flags() {
  return this->flags;
}

void fr12_net::configure(fr12_net_serialized *ee) {
  this->flags = ee->flags;
  memcpy(this->mac, ee->mac, sizeof(ee->mac));
  this->ip = IPAddress(ee->ip[0], ee->ip[1], ee->ip[2], ee->ip[3]);
  this->dns = IPAddress(ee->dns[0], ee->dns[1], ee->dns[2], ee->dns[3]);
  this->gateway = IPAddress(ee->gateway[0], ee->gateway[1], ee->gateway[2], ee->gateway[3]);
  this->subnet = IPAddress(ee->subnet[0], ee->subnet[1], ee->subnet[2], ee->subnet[3]);
}

void fr12_net::serialize(fr12_net_serialized *ee) {
  ee->flags = this->flags;
  memcpy(ee->mac, this->mac, sizeof(ee->mac));
  
  ee->ip[0] = this->ip[0];
  ee->ip[1] = this->ip[1];
  ee->ip[2] = this->ip[2];
  ee->ip[3] = this->ip[3];
  
  ee->dns[0] = this->dns[0];
  ee->dns[1] = this->dns[1];
  ee->dns[2] = this->dns[2];
  ee->dns[3] = this->dns[3];
  
  ee->gateway[0] = this->gateway[0];
  ee->gateway[1] = this->gateway[1];
  ee->gateway[2] = this->gateway[2];
  ee->gateway[3] = this->gateway[3];
  
  ee->subnet[0] = this->subnet[0];
  ee->subnet[1] = this->subnet[1];
  ee->subnet[2] = this->subnet[2];
  ee->subnet[3] = this->subnet[3];
}

void fr12_net::handle_http() {
  // Process HTTP
  EthernetClient http_client = this->http->available();

  if (http_client) {
    size_t length = http_client.available();
    register uint8_t last = 0x00;

    // Read all bytes from the ethernet card, resizing the buffer as needed
    while (length > 0) {
      uint8_t incoming = http_client.read();
      if (this->http_buffer_index >= this->http_buffer_len) {
        if (this->http_buffer_index < fr12_net_http_max_buffer_len) {
          size_t new_buffer_len = this->http_buffer_len + length + 1;
          uint8_t *new_buffer = (uint8_t *)realloc(this->http_buffer, new_buffer_len);
          if (new_buffer == NULL) {
            // Request Entity Too Large
            this->http_respond(&http_client, 413);
          } 
          else {
            this->http_buffer = new_buffer;
            this->http_buffer_len = new_buffer_len;
          }
        } 
        else {
          // Request Entity Too Large
          this->http_respond(&http_client, 413);
        }
      }

      // Save the incoming byte
      this->http_buffer[this->http_buffer_index] = incoming;

      // Check for line endings
      if (incoming == '\n' && last == '\r') {
        uint8_t *start = NULL;
        if (strstr_P((const char *)this->http_buffer, PSTR("GET ")) == (const char *)this->http_buffer) {
          uint8_t *end;
          start = this->http_buffer + 4;
          end = (uint8_t *)strstr_P((const char *)start, PSTR(" HTTP/1.1"));
          if (end != NULL) {
            *end = '\0';
            ((this->union_station)->*(this->http_handler))(this, &http_client, (char *)start);
          } 
          else {
            // Bad request
            this->http_respond(&http_client, 400);
          }
        }

        this->http_buffer_index = 0;
      } 
      else {
        this->http_buffer_index++;
      }

      length = http_client.available();
      last = incoming;
    }

    http_client.stop();
  }
}

void fr12_net::http_send_headers(EthernetClient *client, uint16_t response_code, const char *content_type, const char **headers, size_t header_length) {
  client->print("HTTP/1.1 ");
  client->print(response_code);

  // Find the response string
  for (size_t a = 0; a < sizeof(this->http_codes); a++) {
    uint16_t code = pgm_read_word(&this->http_codes[a]);
    if (code == response_code) {
      // Read the pointer to the response string
      PGM_P p = (PGM_P)pgm_read_word(&this->http_responses[a]);

      // Allocate memory to hold the response
      size_t len = strlen_P(p) + 1;
      char *code = (char *)calloc(len, sizeof(char));

      // Copy the data at the pointer into the buffer
      strncpy_P(code, p, len);

      client->print(' ');
      client->println(code);
      
      free(code);
      break;
    }
  }

  // Print headers
  client->println("Server: Froshduino v" FR12_VERSION);

  if (headers != NULL) {
    for(size_t i = 0; i < header_length; i++) {
      client->println(headers[i]); 
    }
  }

  // Print final headers
  client->print("Content-Type: ");
  client->println(content_type);
  client->println("Connection: close");
  client->println();
}

void fr12_net::http_respond(EthernetClient *client, uint16_t response_code, const char *data, size_t data_length, const char **headers, size_t header_length) {
  this->http_send_headers(client, response_code, "text/html", headers, header_length);
  if (data_length == 0) {
    data_length = strlen(data);
  }

  // Print data
  if (data != NULL) {
    for(size_t i = 0; i < data_length; i++) {
      client->write(data[i]);
    }
  } 
  else {
    client->write(response_code);
  }

  client->println();
  client->flush();
  client->stop();
}

void fr12_net::http_respond_json(EthernetClient *client, uint16_t response_code, const char **data, size_t data_length, const char **headers, size_t header_length) {
  this->http_send_headers(client, response_code, "application/json", headers, header_length);
  client->print("{\"data\":[");
  if (data != NULL) {
    for(size_t i = 0; i < data_length; i++) {
      client->write('"');
      client->print(data[i]);
      client->write('"');
      if (i < data_length - 1) {
        client->write(',');
      }
    }
  }
  client->print("]}");

  client->println();
  client->flush();
  client->stop();
}

void fr12_net::http_unescape(char *s) {
  /*
   * Remove URL hex escapes from s... done in place.  The basic concept for
   * this routine is borrowed from the WWW library HTUnEscape() routine.
   */
  char *p;

  for (p = s; *s != '\0'; ++s) {
    if (*s == '%') {
      if (*++s != '\0') {
        *p = this->http_unhex(*s) << 4;
      }
      if (*++s != '\0') {
        *p++ += this->http_unhex(*s);
      }
    } 
    else {
      *p++ = *s;
    }
  }

  *p = '\0';
}

int fr12_net::http_unhex(char c) {
  return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
}




