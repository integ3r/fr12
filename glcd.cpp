/*  _______ ______    ____   ______ 
 * |    ___|   __ \  |_   | |__    |
 * |    ___|      <   _|  |_|    __|
 * |___|   |___|__|  |______|______|
 */
 
#include "glcd.h"
#include "minecraft.h"

extern glcd GLCD;

fr12_glcd::fr12_glcd() {
  this->hw = &GLCD;
  this->status = new gText(0, this->hw->Bottom - fr12_minecraft_8_height + 1, this->hw->Width, 1, fr12_minecraft_8);
  this->title = new gText(fr12_glcd_margin_left, 10, this->hw->CenterX - fr12_glcd_margin_right, 1, fr12_minecraft_16);
  this->caption = new gText(this->hw->CenterX + 1, 17, this->hw->Right - fr12_glcd_margin_right, 1, fr12_minecraft_8);
  this->countdown = new gText(0, this->hw->Bottom - fr12_minecraft_8_height - fr12_minecraft_16_height, this->hw->Width, 1, fr12_minecraft_16);
}

fr12_glcd::~fr12_glcd() {
  delete this->countdown;
  delete this->caption;
  delete this->title;
  delete this->status;
}

uint8_t fr12_glcd::begin() {
  this->hw->Init(NON_INVERTED);
  this->hw->SelectFont(fr12_minecraft_8);
  return 0;
}
