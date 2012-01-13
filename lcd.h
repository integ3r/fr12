/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#ifndef FR12_LCD_H
#define FR12_LCD_H

#include "defs.h"

#include "../../../../libraries/LiquidCrystal/LiquidCrystal.h"

// Built-in classes
class LiquidCrystal;

// FR 12 classes
class fr12_config;
class fr12_lcd;

// Serialization structs
struct fr12_lcd_serialized;
struct fr12_lcd_serialized_header;

// Pins
enum {
  fr12_lcd_red = 4,
  fr12_lcd_green = 5,
  fr12_lcd_blue = 6,
  fr12_lcd_rs = 38,
  fr12_lcd_rw = 39,
  fr12_lcd_enable = 40,
  fr12_lcd_d0 = 41,
  fr12_lcd_d1 = 42,
  fr12_lcd_d2 = 43,
  fr12_lcd_d3 = 44,
  fr12_lcd_d4 = 45,
  fr12_lcd_d5 = 46,
  fr12_lcd_d6 = 47,
  fr12_lcd_d7 = 48,
};

// Dimensions and message count
enum {
  fr12_lcd_width = 16,
  fr12_lcd_height = 2,
  fr12_lcd_message_count = 50
};

// Flags
enum {
  fr12_lcd_auto = (1 << 0),
  fr12_lcd_random = (1 << 1)
};

// Defaults
enum {
  fr12_lcd_interval = 5000UL
};

// Mesages
struct fr12_lcd_message {
  uint8_t msg[fr12_lcd_width * fr12_lcd_height];
  uint8_t r, g, b;
} __attribute__ ((packed));

class fr12_lcd {
public:
  // Constructor
  fr12_lcd(fr12_config *config);
  
  // Destructor
  virtual ~fr12_lcd();
  
  // Initializes the LCD
  uint8_t begin();
  
  // Configuration
  void configure(fr12_lcd_serialized_header *ee);
  void serialize(fr12_lcd_serialized_header *ee);
  void serialize_message(fr12_lcd_message *msg);
  
  // Picking
  void next();
  
  // Setters
  void set_color(uint8_t r, uint8_t g, uint8_t b);
  void set_message(fr12_lcd_message *message);
  
  // Text wrap
  void print_wrap(char *msg);
  
  // Getters
  uint32_t get_interval();
  fr12_lcd_message *get_message();
private:
  // Puts a wrapped string to the LCD
  void puts_wrap(char *s, size_t len);
  
  // Puts a wrapped character to the LCD
  void putc_wrap(char c);
  
  // Current X and Y positions
  size_t x, y;
  
  // Random message stuff
  uint8_t flags;
  uint32_t interval;
  size_t index;
  
  // Configuration pointer
  fr12_config *config;
  
  // Hardware
  LiquidCrystal *hw;
  
  // Current message
  fr12_lcd_message msg;
};

#endif /* FR12_LCD_H */
