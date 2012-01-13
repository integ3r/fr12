/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */

#ifndef FR12_GLCD_H
#define FR12_GLCD_H

#include "defs.h"

#include <glcd.h>
#include <glcd_Buildinfo.h>
#include <glcd_Config.h>

// Margins
enum {
  fr12_glcd_margin_left = 3,
  fr12_glcd_margin_right = 3
};

// Built-ins
class glcd;
class gText;

// FR 12 classes
class fr12_glcd;

class fr12_glcd {
public:
  // Constructor
  fr12_glcd();
  
  // Destructor
  virtual ~fr12_glcd();
  
  // Initializes the LCD
  uint8_t begin();
  
  // Public members
  glcd *hw;
  gText *status, *title, *caption, *countdown;
};

#endif /* FR12_GLCD_H */
