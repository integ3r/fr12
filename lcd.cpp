/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#include "lcd.h"
#include "config.h"

fr12_lcd::fr12_lcd() {
  // Stuff. I'm tired. No comment.
  this->hw = NULL;
  this->msg.r = this->msg.g = this->msg.b = 0xff;
  
  // X, Y, flags, etc
  this->x = this->y = 0;
 
  // Clear the message
  memset(this->msg.text, 0x00, sizeof(this->msg.text));
}

fr12_lcd::~fr12_lcd() {
  delete this->hw;
}

uint8_t fr12_lcd::begin() {
  // Start up the LCD
  this->hw = new LiquidCrystal(fr12_lcd_rs, fr12_lcd_rw, fr12_lcd_enable, fr12_lcd_d0, fr12_lcd_d1, fr12_lcd_d2, fr12_lcd_d3, fr12_lcd_d4, fr12_lcd_d5, fr12_lcd_d6, fr12_lcd_d7);

  if (!this->hw) {
    return 1;
  }

  // Initialize the LCD
  this->hw->begin(fr12_lcd_width, fr12_lcd_height);

  // Set up the color pins
  this->set_color(this->msg.r, this->msg.g, this->msg.b);

  // Clear the LCD
  this->hw->clear();

  return 0;
}

void fr12_lcd::configure(fr12_lcd_serialized *ee) {
  this->set_message(&ee->msg);
}

void fr12_lcd::serialize(fr12_lcd_serialized *ee) {
  memcpy(&ee->msg, &this->msg, sizeof(fr12_lcd_message));
}

void fr12_lcd::set_color(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(fr12_lcd_red, 255 - r);
  analogWrite(fr12_lcd_green, 255 - g);
  analogWrite(fr12_lcd_blue, 255 - b);
}

void fr12_lcd::set_message(fr12_lcd_message *message) {
  // Copy the message
  memcpy(&this->msg, message, sizeof(fr12_lcd_message));

  // Clear the display
  this->hw->clear();

  // Clear X and Y positions
  this->x = this->y = 0;

  // Print the message
  this->print_wrap((char *)this->msg.text);

  // Set the backlight
  this->set_color(this->msg.r, this->msg.g, this->msg.b);
}

void fr12_lcd::print_wrap(char *msg) {
  // Copy the message string
  size_t len = strlen(msg);
  char *c = (char *)malloc(len);
  strcpy(c, msg);

  // Start tokenizing
  char *p = strtok(c, " ");
  while (p != NULL) {
    size_t p_len = strlen(p), len = p_len + this->x;

    // Zero-length token. Write an extra space and continue.
    if (len == 0) {
      p = strtok(NULL, " ");
      this->putc_wrap(' ');
      continue;
    }

    // This token will fit. Just write it...
    else if (len <= fr12_lcd_width) {
      this->puts_wrap(p, p_len);

      // Length plus trailing space would fill the line
      if (len + 1 >= fr12_lcd_width) {
        this->x = 0;
        this->hw->setCursor(this->x, ++this->y);
      } 
      else {
        this->hw->write(' ');
        this->x++;
      }
    }

    // Everything else
    else {
      // Simple wrap for too long stuff
      if (p_len > fr12_lcd_width) {
        this->puts_wrap(p, p_len);
      }

      // Next line
      else {
        this->x = 0;
        this->hw->setCursor(this->x, ++this->y);
        this->hw->print(p);
        if (p_len + 1 < fr12_lcd_width) {
          this->hw->write(' ');
          this->x++;
        }
      }
    }

    p = strtok(NULL, " ");
  }

  // Free memory
  free(c);
}

void fr12_lcd::puts_wrap(char *s, size_t len) {
  for (size_t a = 0; a < len; a++) {
    this->putc_wrap(s[a]);
  }
}

void fr12_lcd::putc_wrap(char c) {
  // Write the character
  this->hw->write(c);

  // Increment X and/or Y
  if (++this->x >= fr12_lcd_width) {
    this->x = 0;
    this->hw->setCursor(this->x, ++this->y);
  }
}

fr12_lcd_message *fr12_lcd::get_message() {
  return &this->msg;
}

