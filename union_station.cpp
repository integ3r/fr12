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
  this->lcd = new fr12_lcd(this->config);
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
  // Start up the LCD
  this->lcd->begin();

  // Set initial message
  fr12_lcd_message msg;
  strcpy_P((char *)&msg.text, PSTR("Froshduino " FR12_VERSION));
  msg.r = msg.g = msg.b = 0xff;
  this->lcd->set_message(&msg);

  // Start up the GLCD
  this->glcd->begin();

  // Welcome message
  this->glcd->hw->Puts_P(PSTR("Froshduino " FR12_VERSION));
  this->glcd->hw->Puts_P(PSTR("\n(C) Morgan Jones 2012"));
  this->glcd->hw->Puts_P(PSTR("\nHold on just a sec..."));

  // Start up configuration
  this->glcd->status->Puts_P(PSTR("Initializing EEPROM."));
  this->config->begin();

  // Start up networking
  uint8_t *mac = this->net->get_mac();
  this->glcd->status->ClearArea();
  this->glcd->status->Puts_P(PSTR("MAC: "));
  this->glcd->status->Printf_P(PSTR("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  delay(2000);

  // Initial setup of networking
  this->net->begin(this);

  // Decide how to get an IP
  int ret;
  this->glcd->status->ClearArea();
  if (this->net->get_flags() & fr12_net_use_dhcp) {
    this->glcd->status->Puts_P(PSTR("DHCP..."));
    ret = this->net->begin_ethernet_dhcp();
  }

  if (!(this->net->get_flags() & fr12_net_use_dhcp) || ret == 0) {
    this->glcd->status->ClearArea();
    this->glcd->status->Puts_P(PSTR("Using static IP."));
    delay(2000);
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
    delay(2000);
  }

  // Set up NTP
  this->glcd->status->ClearArea();
  this->glcd->status->Puts_P(PSTR("Syncing local clock..."));
  this->ntp->begin(addresses[1]);
  this->do_sync_ntp();
  delay(2000);
  this->glcd->status->ClearArea();
  this->glcd->status->Printf_P(PSTR("Interval: %lus"), this->time->get_sync_interval());
  delay(2000);

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

uint32_t fr12_union_station::sync_handler(fr12_time *time) {
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
  
  // Toggle the colon
  this->flags ^= fr12_union_station_colon;
  
  // Increment sync index
  this->sync_index++;
  
  // Boom, we're done
  return time->now();
}

void fr12_union_station::http_handler(fr12_net *net, EthernetClient *client, char *path) {
  if (strcmp_P(path, "/") == 0) {
    net->http_respond_json(client, 403); 
    return;
  } 
  char *c = strtok((char *)path, "/");

  // Tokenize
  if (c != NULL) {
    if (strcasecmp_P(c, PSTR("get")) == 0) {
      c = strtok(NULL, "/");
      if (c != NULL) {
        if (strcasecmp_P(c, PSTR("countdown")) == 0) {
          char buffer[32];
          char *arr[] = {
            (char *)&buffer
          };
          snprintf_P((char *)&buffer, sizeof(buffer), PSTR("%.2u:%.2u:%.2u:%.2u.%.4u"), this->countdown->days, this->countdown->hours, this->countdown->mins, this->countdown->secs, this->countdown->millis);
          net->http_respond_json(client, 200, (const char **)arr, 1);
          return;
        }
        else if (strcasecmp_P(c, PSTR("lcd")) == 0) {
          fr12_lcd_serialized lcd;
          char lcd_flags_str[4], lcd_r_str[4], lcd_g_str[4], lcd_b_str[4];
          char *arr[] = {
            (char *)&lcd_flags_str,
            (char *)&lcd_r_str,
            (char *)&lcd_g_str,
            (char *)&lcd_b_str,
            (char *)&lcd.msg.text
          };
          this->lcd->serialize(&lcd);

          snprintf_P((char *)&lcd_flags_str, sizeof(lcd_flags_str), PSTR("%u"), lcd.flags);
          snprintf_P((char *)&lcd_r_str, sizeof(lcd_r_str), PSTR("%u"), lcd.msg.r);
          snprintf_P((char *)&lcd_g_str, sizeof(lcd_g_str), PSTR("%u"), lcd.msg.g);
          snprintf_P((char *)&lcd_b_str, sizeof(lcd_b_str), PSTR("%u"), lcd.msg.b);

          net->http_respond_json(client, 200, (const char **)arr, 5);
          return;
        } 
        else if (strcasecmp_P(c, PSTR("net")) == 0) {
          fr12_net_serialized network;
          char ip_str[16], dns_str[16], gateway_str[16], subnet_str[16], mac_str[18];
          char *arr[] = {
            (char *)&mac_str,
            (char *)&ip_str,
            (char *)&dns_str,
            (char *)&gateway_str,
            (char *)&subnet_str
          };
          this->net->serialize(&network);
          
          snprintf_P((char *)&mac_str, sizeof(mac_str), PSTR("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"), network.mac[0], network.mac[1], network.mac[2], network.mac[3], network.mac[4], network.mac[5], network.mac[6]);
          snprintf_P((char *)&ip_str, sizeof(ip_str), PSTR("%u.%u.%u.%u"), network.ip[0], network.ip[1], network.ip[2], network.ip[3]);
          snprintf_P((char *)&dns_str, sizeof(dns_str), PSTR("%u.%u.%u.%u"), network.dns[0], network.dns[1], network.dns[2], network.dns[3]);
          snprintf_P((char *)&gateway_str, sizeof(gateway_str), PSTR("%u.%u.%u.%u"), network.gateway[0], network.gateway[1], network.gateway[2], network.gateway[3]);
          snprintf_P((char *)&subnet_str, sizeof(subnet_str), PSTR("%u.%u.%u.%u"), network.subnet[0], network.subnet[1], network.subnet[2], network.subnet[3]);
          
          net->http_respond_json(client, 200, (const char **)arr, 5);
          return;
        } 
        else if (strcasecmp_P(c, PSTR("ntp")) == 0) {
          fr12_ntp_serialized ntp;
          char *arr[] = {
            (char *)&ntp.server
          };
          this->ntp->serialize(&ntp);
          net->http_respond_json(client, 200, (const char **)arr, 1);
          return;
        } 
        else if (strcasecmp_P(c, PSTR("time")) == 0) {
          fr12_time_serialized time;
          char time_now[11], time_sync_interval[11];
          char *arr[] = {
            (char *)&time_now,
            (char *)&time_sync_interval
          };
          this->time->serialize(&time);
          snprintf_P((char *)&time_now, sizeof(time_now), PSTR("%lu"), time.seconds);
          snprintf_P((char *)&time_sync_interval, sizeof(time_sync_interval), PSTR("%lu"), time.sync_interval);
          net->http_respond_json(client, 200, (const char **)arr, 2);
          return;
        }
      }
    } 
    else if (strcasecmp_P(c, PSTR("set")) == 0) {
      c = strtok(NULL, "/?");
      if (c != NULL) {
        if (strcasecmp_P(c, PSTR("countdown")) == 0) {
          // Get a serialized copy of the Union Station configuration
          fr12_union_station_serialized us, old_us;
          this->serialize(&us);
          memcpy(&old_us, &us, sizeof(fr12_union_station_serialized));

          // Tokenize the query
          c = this->do_find_query(c);
          c = strtok(c, "&");

          while (c != NULL) {
            // Split it up
            char *key, *value;
            this->do_break_query(c, &key, &value);

            if (strcasecmp_P(key, PSTR("time")) == 0) {
              us.countdown_to = strtoul(value, NULL, 0);
              if (us.countdown_to < this->time->now()) {
                us.countdown_to = old_us.countdown_to;
              }
            }

            c = strtok(NULL, "&");
          }

          // Write settings (if different)
          if (memcmp(&us, &old_us, sizeof(fr12_union_station_serialized)) != 0) {
            this->config->write_union_station(&us);
          }
          net->http_respond_json(client, 200);
          return;
        }
        else if (strcasecmp_P(c, PSTR("lcd")) == 0) {
          // Serialize LCD mesage and config
          fr12_lcd_serialized lcd, old_lcd;
          this->lcd->serialize(&lcd);
          memcpy(&old_lcd, &lcd, sizeof(fr12_lcd_serialized));

          // Tokenize the query
          c = this->do_find_query(c);
          c = strtok(c, "&");

          while (c != NULL) {
            // Split it up
            char *key, *value;
            this->do_break_query(c, &key, &value);

            if (strcasecmp_P(key, PSTR("msg")) == 0) {
              if (strlen(value) < sizeof(lcd.msg.text)) {
                strcpy((char *)&lcd.msg.text, value);
              }
            } 
            else if (strcasecmp_P(key, PSTR("r")) == 0) {
              lcd.msg.r = (uint8_t)strtoul(value, NULL, 0);
            } 
            else if (strcasecmp_P(key, PSTR("g")) == 0) {
              lcd.msg.g = (uint8_t)strtoul(value, NULL, 0);
            } 
            else if (strcasecmp_P(key, PSTR("b")) == 0) {
              lcd.msg.b = (uint8_t)strtoul(value, NULL, 0);
            }

            c = strtok(NULL, "&");
          }

          // Write settings (if different)
          if (memcmp(&lcd, &old_lcd, sizeof(fr12_lcd_serialized)) != 0) {
            this->config->write_lcd(&lcd);
          }

          net->http_respond_json(client, 200);
          return;
        }
        else if (strcasecmp_P(c, PSTR("net")) == 0) {
          // Serialize net config
          fr12_net_serialized network, old_network;
          this->net->serialize(&network);
          memcpy(&old_network, &network, sizeof(fr12_net_serialized));

          // Tokenize the query
          c = this->do_find_query(c);
          c = strtok(c, "&");

          while (c != NULL) {
            // Split it up
            char *key, *value;
            this->do_break_query(c, &key, &value);

            if (strcasecmp_P(key, PSTR("flags")) == 0) {
              network.flags = (uint8_t)strtoul(value, NULL, 0);
            } 
            else if (strcasecmp_P(key, PSTR("mac")) == 0) {
              if (sscanf_P(value, PSTR("%hhx:%hhx:%hhx:%hhx:%hhx:%hhx"), &network.mac[0], &network.mac[1], &network.mac[2], &network.mac[3], &network.mac[4], &network.mac[5]) != 6) {
                memcpy(&network.mac, &old_network.mac, sizeof(network.mac));
              } 
            } 
            else if (strcasecmp_P(key, PSTR("ip")) == 0) {
              if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &network.ip[0], &network.ip[1], &network.ip[2], &network.ip[3]) != 4) {
                memcpy(&network.ip, &old_network.ip, sizeof(network.ip));
              }
            } 
            else if (strcasecmp_P(key, PSTR("dns")) == 0) {
              if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &network.dns[0], &network.dns[1], &network.dns[2], &network.dns[3]) != 4) {
                memcpy(&network.dns, &old_network.dns, sizeof(network.dns));
              }
            } 
            else if (strcasecmp_P(key, PSTR("gateway")) == 0) {
              if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &network.gateway[0], &network.gateway[1], &network.gateway[2], &network.gateway[3]) != 4) {
                memcpy(&network.gateway, &old_network.gateway, sizeof(network.gateway));
              }
            } 
            else if (strcasecmp_P(key, PSTR("subnet")) == 0) {
              if (sscanf_P(value, PSTR("%hhu.%hhu.%hhu.%hhu"), &network.subnet[0], &network.subnet[1], &network.subnet[2], &network.subnet[3]) != 4) {
                memcpy(&network.subnet, &old_network.subnet, sizeof(network.subnet));
              }
            }

            c = strtok(NULL, "&");
          }

          // Write settings (if different)
          if (memcmp(&network, &old_network, sizeof(fr12_net_serialized)) != 0) {
            this->config->write_net(&network);
          }

          net->http_respond_json(client, 200);
          return;
        }
        else if (strcasecmp_P(c, PSTR("ntp")) == 0) {
          // Serialize NTP config
          fr12_ntp_serialized ntp, old_ntp;
          this->ntp->serialize(&ntp);
          memcpy(&old_ntp, &ntp, sizeof(fr12_ntp_serialized));

          // Tokenize query
          c = this->do_find_query(c);
          c = strtok(c, "&");

          while (c != NULL) {
            // Split it up
            char *key, *value;
            size_t sz = strlen(value);
            this->do_break_query(c, &key, &value);

            // Only copy if it's valid
            if (strcasecmp_P(key, PSTR("server")) == 0 && sz < sizeof(ntp.server) && sz != 0) {
              strcpy((char *)&ntp.server, (const char *)value);
            }

            c = strtok(NULL, "&");
          }

          // Write settings (if different)
          if (memcmp(&ntp, &old_ntp, sizeof(fr12_ntp_serialized)) != 0) {
            this->config->write_ntp(&ntp);
          }

          net->http_respond_json(client, 200);
          return;
        }
        else if (strcasecmp_P(c, PSTR("time")) == 0) {
          // Serialize time
          fr12_time_serialized time, old_time;
          this->time->serialize(&time);
          memcpy(&old_time, &time, sizeof(fr12_time_serialized));

          // Tokenize query
          c = this->do_find_query(c);
          c = strtok(c, "&");

          while (c != NULL) {
            // Split it up
            char *key, *value;
            this->do_break_query(c, &key, &value);

            if (strcasecmp_P(key, PSTR("time")) == 0) {
              time.seconds = strtoul(value, NULL, 0);
              if (time.seconds < this->countdown->get_timestamp()) {
                time.seconds = old_time.seconds;
              }
            } 
            else if (strcasecmp_P(key, PSTR("sync_interval")) == 0) {
              time.sync_interval = strtoul(value, NULL, 0);
            }

            c = strtok(NULL, "&");
          }

          // Write settings (if different)
          if (memcmp(&time, &old_time, sizeof(fr12_time_serialized)) != 0) {
            this->config->write_time(&time);
          }

          net->http_respond_json(client, 200);
          return;
        }
      }
    }
  }
  net->http_respond_json(client, 404);
}

void fr12_union_station::do_status_reset() {
  this->glcd->status->ClearArea();
  if (this->flags & fr12_union_station_time_inaccurate) {
    this->glcd->status->Puts_P(PSTR("Warning: time inaccurate!"));
  } 
  else {
    // centiseconds... recommended by Billy Willson
    this->glcd->status->Puts_P(PSTR("       d      h       m      s    cs"));
  }
}

void fr12_union_station::do_sync_ntp() {
  uint8_t synced = 0;
  uint8_t tries = 0;

  while (synced == 0 && tries < fr12_union_station_sync_max_tries) {
    this->glcd->status->ClearArea();
    if (this->ntp->get_hostname() != NULL) {
      this->glcd->status->Printf_P(PSTR("%s (%u)"), this->ntp->get_hostname(), ++tries);
    }

    // Send a packet
    uint32_t then = millis(), t;
    this->ntp->send_packet();

    // Wait for a response
    while (millis() - then < fr12_union_station_ntp_timeout) {
      t = this->ntp->get_time();
      if (t > 0) {
        synced = 1;
        break;
      }
    }

    // Set time
    this->glcd->status->ClearArea();
    if (synced == 1) {
      int32_t delta = t - this->time->now();
      this->time->set(t);
      this->glcd->status->Printf_P(PSTR("Delta: %+lds"), delta);
      this->flags &= ~fr12_union_station_time_inaccurate;
    }
  }

  if (synced == 0) {
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


