/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "union_station.h"

#include "config.h"
#include "lcd.h"
#include "glcd.h"
#include "net.h"
#include "ntp.h"
#include "time.h"
#include "countdown.h"

fr12_union_station::fr12_union_station() {
  this->config = new fr12_config(this);
  this->lcd = new fr12_lcd();
  this->glcd = new fr12_glcd();
  this->net = new fr12_net();
  this->ntp = new fr12_ntp();
  this->time = new fr12_time();
  this->countdown = NULL;
  this->sync_index = 1;
  this->flags = 0;
}

fr12_union_station::~fr12_union_station() {
  delete this->countdown;
  delete this->time;
  delete this->ntp;
  delete this->net;
  delete this->glcd;
  delete this->lcd;
  delete this->config;
}

void fr12_union_station::setup() {
  // Initialize heartbeat LED
  pinMode(fr12_union_station_heartbeat_pin, OUTPUT);
  digitalWrite(fr12_union_station_heartbeat_pin, HIGH);
  
  // Start up the LCD
  this->lcd->begin();

  // Start up the GLCD
  this->glcd->begin();

  // Welcome message
  this->glcd->hw->Puts_P(PSTR("Froshduino " FR12_VERSION));
  this->glcd->hw->Puts_P(PSTR("\n(C) Morgan Jones 2012"));
  this->glcd->hw->Puts_P(PSTR("\nHold on just a sec..."));

  // Before starting configuration, check if digital 13 is held high. If so, reset.
  pinMode(fr12_union_station_reset_pin, INPUT);
  if (digitalRead(fr12_union_station_reset_pin) == HIGH) {
    this->glcd->status->Puts_P(PSTR("Resetting EEPROM."));
    this->config->reset();
  }

  // Start up configuration
  this->glcd->status->ClearArea();
  this->glcd->status->Puts_P(PSTR("Loading configuration."));
  this->config->begin();

  // Start up networking
  uint8_t *mac = this->net->get_mac();
  this->glcd->status->ClearArea();
  this->glcd->status->Puts_P(PSTR("MAC: "));
  this->glcd->status->Printf_P(PSTR("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  delay(1500);

  // Initial setup of networking
  this->net->begin(this);

  // Decide how to get an IP
  int ret = 0;
  this->glcd->status->ClearArea();
  if (this->net->get_flags() & fr12_net_use_dhcp) {
    this->glcd->status->Puts_P(PSTR("DHCP..."));
    ret = this->net->begin_ethernet_dhcp();
  }

  if (!(this->net->get_flags() & fr12_net_use_dhcp) || ret == 0) {
    this->glcd->status->ClearArea();
    this->glcd->status->Puts_P(PSTR("Using static IP."));
    delay(1500);
    this->net->begin_ethernet_static();
  }

  // Print IP/DNS/gateway/subnet
  IPAddress addresses[4];
  this->net->get_addresses((IPAddress *)&addresses);

  for (uint8_t a = 0; a < 4; a++) {
    this->glcd->status->ClearArea();
    switch(a) {
    case 0:
      this->glcd->status->Puts_P(PSTR("IP"));
      break;
    case 1:
      this->glcd->status->Puts_P(PSTR("DNS"));
      break;
    case 2:
      this->glcd->status->Puts_P(PSTR("Gateway"));
      break;
    case 3:
      this->glcd->status->Puts_P(PSTR("Subnet"));
      break;
    default:
      continue;
      break;
    }
    this->glcd->status->Puts_P(PSTR(": "));
    this->glcd->status->Printf_P(PSTR("%u.%u.%u.%u"), addresses[a][0], addresses[a][1], addresses[a][2], addresses[a][3]);
    delay(1500);
  }

  // Set up NTP
  this->glcd->status->ClearArea();
  this->glcd->status->Puts_P(PSTR("Syncing local clock..."));
  this->ntp->begin(addresses[1]);
  this->do_sync_ntp();
  delay(1500);
  this->glcd->status->ClearArea();
  this->glcd->status->Printf_P(PSTR("Interval: %lus"), this->time->get_sync_interval());
  delay(1500);

  // Set up automatic clock synchronization
  this->time->set_auto_sync(this, &fr12_union_station::sync_handler);

  // Start HTTP
  this->glcd->status->ClearArea();
  this->net->begin_http(&fr12_union_station::http_handler);

  // Switch to the main loop. Clear the screen and all areas.
  this->glcd->hw->ClearScreen();
  this->glcd->status->ClearArea();
  this->glcd->title->Puts_P(PSTR("FR 12"));
  this->glcd->caption->Puts_P(PSTR("COUNTDOWN!"));
  this->do_status_reset();
}

void fr12_union_station::loop() {
  // Update time
  this->time->update();

  if (!this->countdown->target_reached()) {
    // Update countdown
    this->countdown->update(this->time);

    // Are we there yet?
    if (this->countdown->target_reached()) {
      // boom shaka laka
      this->glcd->countdown->ClearArea();

      // font optimization
      this->glcd->countdown->Puts_P(PSTR(" RIGHT NOW!"));
    } 
    else {
      // Start at the beginning of the string
      this->glcd->countdown->CursorToXY(0, 0);
      this->glcd->countdown->write(' ');

      // Display results based upon colon
      if (this->flags & fr12_union_station_colon) {
        this->glcd->countdown->Printf_P(PSTR("%.2u:%.2u:%.2u:%.2u.%.2u"), this->countdown->days, this->countdown->hours, this->countdown->mins, this->countdown->secs, this->countdown->millis / 10);
      } 
      else {
        this->glcd->countdown->Printf_P(PSTR("%.2u %.2u %.2u %.2u %.2u"), this->countdown->days, this->countdown->hours, this->countdown->mins, this->countdown->secs, this->countdown->millis / 10);
      }
    }
  }


  // Process Ethernet connections
  this->net->handle_http();
}

void fr12_union_station::configure(fr12_union_station_serialized *ee) {
  delete this->countdown;
  this->countdown = new fr12_countdown(ee->countdown_to);
}

void fr12_union_station::serialize(fr12_union_station_serialized *ee) {
  ee->countdown_to = this->countdown->get_timestamp();
}

void fr12_union_station::sync_handler() {
  // Sync with NTP if we need to
  if (this->sync_index % fr12_union_station_ntp_sync_interval == 0) {
    this->do_sync_ntp();
    this->do_status_reset();
    this->sync_index = 0;
  } 

  // Serialize the current time and write it to EEPROM if we need to as well
  if (this->sync_index % fr12_union_station_config_write_interval == 0) {
    fr12_time_serialized t;
    this->time->serialize(&t);
    this->config->write_time(&t);
  }
  
  // Toggle the heartbeat LED
  PORTB ^= _BV(PB7);

  // Toggle the colon
  this->flags ^= fr12_union_station_colon;

  // Increment sync index
  this->sync_index++;
}

void fr12_union_station::http_handler(EthernetClient *client, char *path) {
  // Malformed URL, so 400
  if (path[0] != '/') {
    this->net->http_respond(client, 400); 
    return;
  }
  
  // Root isn't accessible. Make it forbidden.
  else if (strcmp_P(path, PSTR("/")) == 0) {
    this->net->http_respond(client, 403);
    return;
  }
  
  // Tokenize
  path = strtok(path, "/");

  // Tokenize
  if (path != NULL) {
    if (strcasecmp_P(path, PSTR("get")) == 0) {
      if ((path = strtok(NULL, "/")) != NULL) {
        if (strcasecmp_P(path, PSTR("countdown")) == 0) {
          this->http_get<fr12_union_station, fr12_union_station_serialized>(&fr12_union_station::http_get_countdown, this, client);
          return;
        }
        else if (strcasecmp_P(path, PSTR("lcd")) == 0) {
          this->http_get<fr12_lcd, fr12_lcd_serialized>(&fr12_union_station::http_get_lcd, this->lcd, client);
          return;
        } 
        else if (strcasecmp_P(path, PSTR("net")) == 0) {
          this->http_get<fr12_net, fr12_net_serialized>(&fr12_union_station::http_get_net, this->net, client);
          return;
        } 
        else if (strcasecmp_P(path, PSTR("ntp")) == 0) {
          this->http_get<fr12_ntp, fr12_ntp_serialized>(&fr12_union_station::http_get_ntp, this->ntp, client);
          return;
        } 
        else if (strcasecmp_P(path, PSTR("time")) == 0) {
          this->http_get<fr12_time, fr12_time_serialized>(&fr12_union_station::http_get_time, this->time, client);
          return;
        }
      }
    } 
    else if (strcasecmp_P(path, PSTR("set")) == 0) {
      if ((path = strtok(NULL, "/?")) != NULL) {
        if (strcasecmp_P(path, PSTR("countdown")) == 0) {
          this->http_set<fr12_union_station, fr12_union_station_serialized>(&fr12_union_station::http_set_countdown, &fr12_config::write_union_station, this, path);
          this->http_get<fr12_union_station, fr12_union_station_serialized>(&fr12_union_station::http_get_countdown, this, client);
          return;
        }
        else if (strcasecmp_P(path, PSTR("lcd")) == 0) {
          this->http_set<fr12_lcd, fr12_lcd_serialized>(&fr12_union_station::http_set_lcd, &fr12_config::write_lcd, this->lcd, path);
          this->http_get<fr12_lcd, fr12_lcd_serialized>(&fr12_union_station::http_get_lcd, this->lcd, client);
          return;
        }
        else if (strcasecmp_P(path, PSTR("net")) == 0) {
          this->http_set<fr12_net, fr12_net_serialized>(&fr12_union_station::http_set_net, &fr12_config::write_net, this->net, path);
          this->http_get<fr12_net, fr12_net_serialized>(&fr12_union_station::http_get_net, this->net, client);
          return;
        }
        else if (strcasecmp_P(path, PSTR("ntp")) == 0) {
          this->http_set<fr12_ntp, fr12_ntp_serialized>(&fr12_union_station::http_set_ntp, &fr12_config::write_ntp, this->ntp, path);
          this->http_get<fr12_ntp, fr12_ntp_serialized>(&fr12_union_station::http_get_ntp, this->ntp, client);
          return;
        }
        else if (strcasecmp_P(path, PSTR("time")) == 0) {
          this->http_set<fr12_time, fr12_time_serialized>(&fr12_union_station::http_set_time, &fr12_config::write_time, this->time, path);
          this->http_get<fr12_time, fr12_time_serialized>(&fr12_union_station::http_get_time, this->time, client);
          return;
        }
      }
    }
  }
  
  // Respond 404 otherwise.
  net->http_respond(client, 404);
}

void fr12_union_station::http_get_countdown(void *ee, EthernetClient *client) {
  fr12_union_station_serialized *us = (fr12_union_station_serialized *)ee;
  char countdown_to[11], countdown_time[32];
  char *arr[] = {
    (char *)&countdown_to,
    (char *)&countdown_time
  };
  snprintf_P((char *)&countdown_to, sizeof(countdown_to), PSTR("%lu"), us->countdown_to);
  snprintf_P((char *)&countdown_time, sizeof(countdown_time), PSTR("%.2u:%.2u:%.2u:%.2u.%.4u"), this->countdown->days, this->countdown->hours, this->countdown->mins, this->countdown->secs, this->countdown->millis);
  this->net->http_respond_json(client, 200, (const char **)arr, 2);
}

void fr12_union_station::http_get_lcd(void *ee, EthernetClient *client) {
  fr12_lcd_serialized *lcd = (fr12_lcd_serialized *)ee;
  char lcd_r_str[4], lcd_g_str[4], lcd_b_str[4];
  char *arr[] = {
    (char *)&lcd_r_str,
    (char *)&lcd_g_str,
    (char *)&lcd_b_str,
    (char *)&lcd->msg.text
  };

  snprintf_P((char *)&lcd_r_str, sizeof(lcd_r_str), PSTR("%u"), lcd->msg.r);
  snprintf_P((char *)&lcd_g_str, sizeof(lcd_g_str), PSTR("%u"), lcd->msg.g);
  snprintf_P((char *)&lcd_b_str, sizeof(lcd_b_str), PSTR("%u"), lcd->msg.b);

  this->net->http_respond_json(client, 200, (const char **)arr, 4);
}

void fr12_union_station::http_get_net(void *ee, EthernetClient *client) {
  fr12_net_serialized *network = (fr12_net_serialized *)ee;
  IPAddress ip(network->ip), dns(network->dns), gateway(network->gateway), subnet(network->subnet);
  char ip_str[16], dns_str[16], gateway_str[16], subnet_str[16], mac_str[18];
  char *arr[] = {
    (char *)&mac_str,
    (char *)&ip_str,
    (char *)&dns_str,
    (char *)&gateway_str,
    (char *)&subnet_str
  };

  snprintf_P((char *)&mac_str, sizeof(mac_str), PSTR("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"), network->mac[0], network->mac[1], network->mac[2], network->mac[3], network->mac[4], network->mac[5]);
  snprintf_P((char *)&ip_str, sizeof(ip_str), PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
  snprintf_P((char *)&dns_str, sizeof(dns_str), PSTR("%u.%u.%u.%u"), dns[0], dns[1], dns[2], dns[3]);
  snprintf_P((char *)&gateway_str, sizeof(gateway_str), PSTR("%u.%u.%u.%u"), gateway[0], gateway[1], gateway[2], gateway[3]);
  snprintf_P((char *)&subnet_str, sizeof(subnet_str), PSTR("%u.%u.%u.%u"), subnet[0], subnet[1], subnet[2], subnet[3]);

  this->net->http_respond_json(client, 200, (const char **)arr, 5);
}

void fr12_union_station::http_get_ntp(void *ee, EthernetClient *client) {
  fr12_ntp_serialized *ntp = (fr12_ntp_serialized *)ee;
  char *arr[] = {
    (char *)&ntp->server
  };
  this->net->http_respond_json(client, 200, (const char **)arr, 1);
}

void fr12_union_station::http_get_time(void *ee, EthernetClient *client) {
  fr12_time_serialized *time = (fr12_time_serialized *)ee;
  char time_now[11], time_sync_interval[11];
  char *arr[] = {
    (char *)&time_now,
    (char *)&time_sync_interval
  };

  snprintf_P((char *)&time_now, sizeof(time_now), PSTR("%lu"), time->seconds);
  snprintf_P((char *)&time_sync_interval, sizeof(time_sync_interval), PSTR("%lu"), time->sync_interval);
  this->net->http_respond_json(client, 200, (const char **)arr, 2);
}

void fr12_union_station::http_set_countdown(void *ee_new, void *ee_old, char *key, char *value) {
  fr12_union_station_serialized *us_new = (fr12_union_station_serialized *)ee_new;
  fr12_union_station_serialized *us_old = (fr12_union_station_serialized *)ee_old;

  if (strcasecmp_P(key, PSTR("time")) == 0) {
    us_new->countdown_to = strtoul(value, NULL, 0);
    if (us_new->countdown_to < this->time->now()) {
      us_new->countdown_to = us_old->countdown_to;
    }
  }
}

void fr12_union_station::http_set_lcd(void *ee_new, void *ee_old, char *key, char *value) {
  fr12_lcd_serialized *lcd_new = (fr12_lcd_serialized *)ee_new;
  //fr12_lcd_serialized *lcd_old = (fr12_lcd_serialized *)ee_old;

  if (strcasecmp_P(key, PSTR("msg")) == 0) {
    strncpy((char *)&lcd_new->msg.text, value, sizeof(lcd_new->msg.text));
  } 
  else if (strcasecmp_P(key, PSTR("r")) == 0) {
    lcd_new->msg.r = (uint8_t)strtoul(value, NULL, 0);
  } 
  else if (strcasecmp_P(key, PSTR("g")) == 0) {
    lcd_new->msg.g = (uint8_t)strtoul(value, NULL, 0);
  } 
  else if (strcasecmp_P(key, PSTR("b")) == 0) {
    lcd_new->msg.b = (uint8_t)strtoul(value, NULL, 0);
  }
}

void fr12_union_station::http_set_net(void *ee_new, void *ee_old, char *key, char *value) {
  fr12_net_serialized *network_new = (fr12_net_serialized *)ee_new;
  fr12_net_serialized *network_old = (fr12_net_serialized *)ee_old;
  if (strcasecmp_P(key, PSTR("flags")) == 0) {
    network_new->flags = (uint8_t)strtoul(value, NULL, 0);
  } 
  else if (strcasecmp_P(key, PSTR("mac")) == 0) {
    if (sscanf_P(value, PSTR("%hhx:%hhx:%hhx:%hhx:%hhx:%hhx"), &network_new->mac[0], &network_new->mac[1], &network_new->mac[2], &network_new->mac[3], &network_new->mac[4], &network_new->mac[5]) != 6) {
      memcpy(&network_new->mac, &network_old->mac, sizeof(network_new->mac));
    } 
  } 
  else if (strcasecmp_P(key, PSTR("ip")) == 0) {
    IPAddress ip(network_new->ip);
    if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
      network_new->ip = ip;
    }
  } 
  else if (strcasecmp_P(key, PSTR("dns")) == 0) {
    IPAddress dns(network_new->dns);
    if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &dns[0], &dns[1], &dns[2], &dns[3]) == 4) {
      network_new->dns = dns;
    }
  } 
  else if (strcasecmp_P(key, PSTR("gateway")) == 0) {
    IPAddress gateway(network_new->gateway);
    if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &gateway[0], &gateway[1], &gateway[2], &gateway[3]) == 4) {
      network_new->gateway = gateway;
    }
  } 
  else if (strcasecmp_P(key, PSTR("subnet")) == 0) {
    IPAddress subnet(network_new->subnet);
    if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &subnet[0], &subnet[1], &subnet[2], &subnet[3]) == 4) {
      network_new->subnet = subnet;
    }
  }
}

void fr12_union_station::http_set_ntp(void *ee_new, void *ee_old, char *key, char *value) {
  fr12_ntp_serialized *ntp_new = (fr12_ntp_serialized *)ee_new;
  //fr12_ntp_serialized *ntp_old = (fr12_ntp_serialized *)ee_old;
  
  size_t sz = strlen(key);
  // Only copy if it's valid
  if (strcasecmp_P(key, PSTR("server")) == 0 && sz < sizeof(ntp_new->server) && sz != 0) {
    strcpy((char *)&ntp_new->server, (const char *)value);
  }
}

void fr12_union_station::http_set_time(void *ee_new, void *ee_old, char *key, char *value) {
  fr12_time_serialized *time_new = (fr12_time_serialized *)ee_new;
  fr12_time_serialized *time_old = (fr12_time_serialized *)ee_old;
  if (strcasecmp_P(key, PSTR("time")) == 0) {
    time_new->seconds = strtoul(value, NULL, 0);
    if (time_new->seconds < this->countdown->get_timestamp()) {
      time_new->seconds = time_old->seconds;
    }
  } 
  else if (strcasecmp_P(key, PSTR("sync_interval")) == 0) {
    time_new->sync_interval = strtoul(value, NULL, 0);
  }
}

void fr12_union_station::do_status_reset() {
  this->glcd->status->ClearArea();
  if (this->flags & fr12_union_station_time_inaccurate) {
    this->glcd->status->Puts_P(PSTR("Time slightly inaccurate."));
  } 
  else {
    // centiseconds... recommended by Billy Willson
    this->glcd->status->Puts_P(PSTR("       d      h       m      s    cs"));
  }
}

void fr12_union_station::do_sync_ntp() {
  uint8_t tries = 0;
  uint32_t t = 0;

  while (t == 0 && tries < fr12_union_station_sync_max_tries) {
    this->glcd->status->ClearArea();
    if (this->ntp->get_hostname() != NULL) {
      this->glcd->status->Printf_P(PSTR("%s (%u)"), this->ntp->get_hostname(), ++tries);
    }

    // Send a packet
    uint32_t then = millis();
    this->ntp->send_packet();

    // Wait for a response
    while (millis() - then < fr12_union_station_ntp_timeout) {
      t = this->ntp->get_time();
      if (t > 0) {
        break;
      }
    }

    // Set time
    this->glcd->status->ClearArea();
    if (t > 0) {
      int32_t delta = t - this->time->now();
      this->time->set(t);
      this->glcd->status->Printf_P(PSTR("Delta: %+lds"), delta);
      this->flags &= ~fr12_union_station_time_inaccurate;
    }
  }

  if (t == 0) {
    this->glcd->status->Puts_P(PSTR("Error syncing time."));
    this->flags |= fr12_union_station_time_inaccurate;
  }
}

void fr12_union_station::do_break_query(char *c, char **key, char **value) {
  char *v;
  for (v = c; *v != '\0'; v++) {
    if (*v == '=') {
      *v = '\0';
      v++;
      break;
    }
  }

  // Unescape the rest
  if (*v != '\0') {
    net->http_unescape(v);
  }

  *key = c;
  *value = v;
}

char *fr12_union_station::do_find_query(char *str) {
  while(*str != '\0') {
    str++;
  }

  return str + 1;
}
