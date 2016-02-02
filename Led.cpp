//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.
//
//  Creator:  Scott Simpson
//  

#include <wiringPi.h>
#include <wiringSerial.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "Led.h"
#include "ws2811.h"

#define RIGHT_FRONT 0
#define LEFT_REAR   1
#define LEFT_FRONT  2
#define RIGHT_REAR  3

LedController::LedController() {

//  int ws_ret = ws2811_init(&ledstring);
//  printf("Led_init = %i", ws_ret);

  leds_on_strip = LED_MAX_BULBS;

  for (int i=0; i < LED_MAX_BULBS; i++) {
    led_strips[i] = new LedStrip();
  }

  for (int j=0; j < LED_MAX_STRIPS; j++)
  {
    for (int i=0; i < LED_MAX_BULBS; i++) {
     leds[i][j]=0x002000;
    }
  }

  leds_render(true);

  LedBulbColor* red = new LedBulbColor(255, 0, 0);
  LedBulbColor* green = new LedBulbColor(0, 255, 0);
  LedBulbColor* blue = new LedBulbColor(0, 0, 255);
  LedBulbColor* yellow = new LedBulbColor(255, 255, 0);
  //LedBulbColor* cyan = new LedBulbColor(0, 255, 255);
  //LedBulbColor* magenta = new LedBulbColor(255, 0, 255);
  LedBulbColor* off = new LedBulbColor(0, 0, 0);
  LedBulbColor* white = new LedBulbColor(255, 255, 255);

  /////////////////////////////////////////////////////////////////////////
  // Pattern 0
  // red on inside, green on outside LED
  /////////////////////////////////////////////////////////////////////////
  LedStripPattern* led_pattern = new LedStripPattern(); 
  led_pattern->add_strip_state(red, off, off, off, off, off, off, green, 0);
  add_pattern(0, 0, led_pattern);
  add_pattern(0, 1, led_pattern);
  add_pattern(0, 2, led_pattern);
  add_pattern(0, 3, led_pattern);

  /////////////////////////////////////////////////////////////////////////
  // Pattern 1
  // all off
  /////////////////////////////////////////////////////////////////////////
  led_pattern = new LedStripPattern(); 
  led_pattern->add_strip_state(off, off, off, off, off, off, off, off, 0);
  add_pattern(1, 0, led_pattern);
  add_pattern(1, 1, led_pattern);
  add_pattern(1, 2, led_pattern);
  add_pattern(1, 3, led_pattern);
  
  /////////////////////////////////////////////////////////////////////////
  // Pattern 2
  // red on inside, white on center 6, green on outside
  /////////////////////////////////////////////////////////////////////////
  led_pattern = new LedStripPattern(); 
  led_pattern->add_strip_state(red, white, white, white, white, white, white, green, 0);
  add_pattern(2, 0, led_pattern);
  add_pattern(2, 1, led_pattern);
  add_pattern(2, 2, led_pattern);
  add_pattern(2, 3, led_pattern);
  
  /////////////////////////////////////////////////////////////////////////
  // Pattern 3
  // red on inside, green on outside, expand whites in center
  /////////////////////////////////////////////////////////////////////////
  // 0                
  // 1 R             G    
  // 2 R-W         W-G
  // 3 R-W-W-    W-W-G
  // 4 R-W-W-W-W-W-W-G
  led_pattern = new LedStripPattern(); 
  led_pattern->add_strip_state(off, off, off, off, off, off, off, off, 600);
  led_pattern->add_strip_state(red, off, off, off, off, off, off, green, 100);
  led_pattern->add_strip_state(red, white, off, off, off, off, white, green, 100);
  led_pattern->add_strip_state(red, white, white, off, off, white, white, green, 100);
  led_pattern->add_strip_state(red, white, white, white, white, white, white, green, 100);
  add_pattern(3, 0, led_pattern);
  add_pattern(3, 1, led_pattern);
  add_pattern(3, 2, led_pattern);
  add_pattern(3, 3, led_pattern);

  /////////////////////////////////////////////////////////////////////////
  // Pattern 4
  //  3  1   front goes out to in with red on left
  //  2  4   back goes in to out with red on left
  /////////////////////////////////////////////////////////////////////////
  
  led_pattern = new LedStripPattern();                                          // right front out to in green
  led_pattern->add_strip_state(off, off, off, off, off, off, green, green, 100);
  led_pattern->add_strip_state(off, off, off, off, off, green, green, off, 100);
  led_pattern->add_strip_state(off, off, off, off, green, green, off, off, 100);
  led_pattern->add_strip_state(off, off, off, green, green, off, off, off, 100);
  led_pattern->add_strip_state(off, off, green, green, off, off, off, off, 100);
  led_pattern->add_strip_state(off, green, green, off, off, off, off, off, 100);
  led_pattern->add_strip_state(green, green, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(green, off, off, off, off, off, off, green, 100);
  add_pattern(4, 0, led_pattern);  

  led_pattern = new LedStripPattern();                                          // left rear in to out red
  led_pattern->add_strip_state(red, red, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, red, red, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, red, red, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, red, red, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, red, red, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, red, red, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, red, red, 100);
  led_pattern->add_strip_state(red, off, off, off, off, off, off, red, 100);
  add_pattern(4, 1, led_pattern);

  led_pattern = new LedStripPattern();                                          // left front out to in red
  led_pattern->add_strip_state(off, off, off, off, off, off, red, red, 100);
  led_pattern->add_strip_state(off, off, off, off, off, red, red, off, 100);
  led_pattern->add_strip_state(off, off, off, off, red, red, off, off, 100);
  led_pattern->add_strip_state(off, off, off, red, red, off, off, off, 100);
  led_pattern->add_strip_state(off, off, red, red, off, off, off, off, 100);
  led_pattern->add_strip_state(off, red, red, off, off, off, off, off, 100);
  led_pattern->add_strip_state(red, red, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(red, off, off, off, off, off, off, red, 100);
  add_pattern(4, 2, led_pattern);
  
  led_pattern = new LedStripPattern();                                          // right rear in to out green
  led_pattern->add_strip_state(green, green, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, green, green, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, green, green, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, green, green, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, green, green, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, green, green, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, green, green, 100);
  led_pattern->add_strip_state(green, off, off, off, off, off, off, green, 100);
  add_pattern(4, 3, led_pattern);

  /////////////////////////////////////////////////////////////////////////
  // Pattern 5
  //  all go inside to out in yellow
  /////////////////////////////////////////////////////////////////////////
  
  led_pattern = new LedStripPattern();                      
  led_pattern->add_strip_state(yellow, yellow, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, yellow, yellow, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, yellow, yellow, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, yellow, yellow, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, yellow, yellow, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, yellow, yellow, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, yellow, yellow, 100);
  led_pattern->add_strip_state(yellow, off, off, off, off, off, off, yellow, 100);
  add_pattern(5, 0, led_pattern);  

  led_pattern = new LedStripPattern();                      
  led_pattern->add_strip_state(yellow, yellow, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, yellow, yellow, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, yellow, yellow, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, yellow, yellow, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, yellow, yellow, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, yellow, yellow, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, yellow, yellow, 100);
  led_pattern->add_strip_state(yellow, off, off, off, off, off, off, yellow, 100);
  add_pattern(5, 1, led_pattern);

  led_pattern = new LedStripPattern();              
  led_pattern->add_strip_state(yellow, yellow, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, yellow, yellow, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, yellow, yellow, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, yellow, yellow, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, yellow, yellow, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, yellow, yellow, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, yellow, yellow, 100);
  led_pattern->add_strip_state(yellow, off, off, off, off, off, off, yellow, 100);
  add_pattern(5, 2, led_pattern);
  
  led_pattern = new LedStripPattern();                     
  led_pattern->add_strip_state(yellow, yellow, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, yellow, yellow, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, yellow, yellow, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, yellow, yellow, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, yellow, yellow, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, yellow, yellow, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, yellow, yellow, 100);
  led_pattern->add_strip_state(yellow, off, off, off, off, off, off, yellow, 100);
  add_pattern(5, 3, led_pattern);  
  
  /////////////////////////////////////////////////////////////////////////
  // Pattern 6
  //  all go inside to out in blue
  /////////////////////////////////////////////////////////////////////////
  
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(blue, blue, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, blue, blue, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, blue, blue, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, blue, blue, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, blue, blue, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, blue, blue, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, blue, blue, 100);
  led_pattern->add_strip_state(blue, off, off, off, off, off, off, blue, 100);
  add_pattern(6, 0, led_pattern);
    
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(blue, blue, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, blue, blue, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, blue, blue, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, blue, blue, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, blue, blue, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, blue, blue, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, blue, blue, 100);
  led_pattern->add_strip_state(blue, off, off, off, off, off, off, blue, 100);
  add_pattern(6, 1, led_pattern);  

  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(blue, blue, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, blue, blue, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, blue, blue, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, blue, blue, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, blue, blue, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, blue, blue, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, blue, blue, 100);
  led_pattern->add_strip_state(blue, off, off, off, off, off, off, blue, 100);
  add_pattern(6, 2, led_pattern);
    
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(blue, blue, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, blue, blue, off, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, blue, blue, off, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, blue, blue, off, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, blue, blue, off, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, blue, blue, off, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, blue, blue, 100);
  led_pattern->add_strip_state(blue, off, off, off, off, off, off, blue, 100);
  add_pattern(6, 3, led_pattern);  

  /////////////////////////////////////////////////////////////////////////
  // Pattern 7
  // red green alternating strobe
  /////////////////////////////////////////////////////////////////////////
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(off, off, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(green, green, green, green, green, green, green, green, 100);
  add_pattern(7, RIGHT_FRONT, led_pattern);  
  add_pattern(7, RIGHT_REAR, led_pattern);    
  
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(red, red, red, red, red, red, red, red, 100);
  led_pattern->add_strip_state(off, off, off, off, off, off, off, off, 100);
  add_pattern(7, LEFT_FRONT, led_pattern);  
  add_pattern(7, LEFT_REAR, led_pattern);  

  /////////////////////////////////////////////////////////////////////////
  // Pattern 8
  // red green simultaneous strobe
  /////////////////////////////////////////////////////////////////////////
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(off, off, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(green, green, green, green, green, green, green, green, 100);
  add_pattern(8, RIGHT_FRONT, led_pattern);  
  add_pattern(8, RIGHT_REAR, led_pattern);    
  
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(off, off, off, off, off, off, off, off, 100);
  led_pattern->add_strip_state(red, red, red, red, red, red, red, red, 100);
  add_pattern(8, LEFT_FRONT, led_pattern);  
  add_pattern(8, LEFT_REAR, led_pattern);  
  
  /////////////////////////////////////////////////////////////////////////
  // Pattern 9
  /////////////////////////////////////////////////////////////////////////
  led_pattern = new LedStripPattern();               
  led_pattern->add_strip_state(yellow, yellow, yellow, yellow, yellow, yellow, yellow, yellow, 0);
  add_pattern(9, RIGHT_FRONT, led_pattern);  
  add_pattern(9, RIGHT_REAR, led_pattern); 
  add_pattern(9, LEFT_FRONT, led_pattern);  
  add_pattern(9, LEFT_REAR, led_pattern);  
}

LedBulbColor::LedBulbColor(uint8_t red_param, uint8_t green_param, uint8_t blue_param) {
  red = red_param;
  green = green_param;
  blue = blue_param;
}

LedStripState::LedStripState() {
    state_time = LED_DEFAULT_STATE_TIME;
}

LedStripState::LedStripState(uint16_t time_param) {
  state_time = time_param;
}

LedStripState::LedStripState(LedBulbColor* c1, LedBulbColor* c2, LedBulbColor* c3, LedBulbColor* c4, LedBulbColor* c5, LedBulbColor* c6, LedBulbColor* c7, LedBulbColor* c8, uint16_t time_param) {
  int bulb_count = 0;
  bulbs[bulb_count++] = c1;
  bulbs[bulb_count++] = c2;
  bulbs[bulb_count++] = c3;
  bulbs[bulb_count++] = c4;
  bulbs[bulb_count++] = c5;
  bulbs[bulb_count++] = c6;
  bulbs[bulb_count++] = c7;
  bulbs[bulb_count++] = c8;
  state_time = time_param;
}

LedStripPattern::LedStripPattern() {
  strip_state_count = 0;
}

void LedStripPattern::add_strip_state(LedStripState* strip_state_param) {
  if(strip_state_count < LED_MAX_STRIP_STATES) {
    led_strip_states[strip_state_count] = strip_state_param;
    strip_state_count++;  
  }
}

void LedStripPattern::add_strip_state(LedBulbColor* c1, LedBulbColor* c2, LedBulbColor* c3, LedBulbColor* c4, LedBulbColor* c5, LedBulbColor* c6, LedBulbColor* c7, LedBulbColor* c8, uint16_t time_param) {
  LedStripState* strip_state = new LedStripState(c1, c2, c3, c4, c5, c6, c7, c8, time_param);
  add_strip_state(strip_state);
}

LedStrip::LedStrip() {
  for(int i=0; i<LED_MAX_PATTERNS_PER_STRIP; i++) {
    led_patterns[i] = NULL;
  }
}

void LedController::add_pattern(int pattern_number, int strip_number, LedStripPattern* pattern) {
  if(pattern_number < LED_MAX_PATTERNS_PER_STRIP) {
    LedStrip* strip_ptr = led_strips[strip_number];
    strip_ptr->led_patterns[pattern_number] = pattern;
  }
}

void LedController::change_led_state(int strip_number, LedStripPattern* pattern, LedStrip* strip_ptr, uint32_t current_time) {
  LedStripState* strip_state = pattern->led_strip_states[strip_ptr->current_state];
  strip_ptr->current_state_expiry_time = current_time + strip_state->state_time;
  for(uint8_t j=0; j<LED_MAX_BULBS; j++) {
    LedBulbColor* bulb_state = strip_state->bulbs[j];
    leds[j][strip_number] = (bulb_state->red << 16 | bulb_state->green << 8 | bulb_state->blue);
  }
}

void LedController::process_10_millisecond() {
  int8_t pattern_index = 0;
  uint8_t reverse = 0;
  if(uav_arm) {
    switch(custom_mode) {
      case 2:                           // stabilize
        pattern_index = 4;
        break;
      case 8:                           // alt hold
        pattern_index = 7;
        break;
      case 9:                           // pos hold
        pattern_index = 8;
        break;
      case 10:                           // auto
        pattern_index = 9;
        break;
      case 13:                           // rtl
        pattern_index = 6;
        break;
      case 12:                           // circle
        pattern_index = 5;
        break;
      default:
        pattern_index = 0;
        break;
    }
  }else{
    pattern_index = 2;
  }

  uint32_t current_time = millis();                                                 // make sure all LEDs start at the same time
  for(int i=0; i<8; i++) {
    LedStrip* strip_ptr = led_strips[i];
    LedStripPattern* pattern = strip_ptr->led_patterns[pattern_index]; 
    if(pattern != NULL) {  
      if(current_time >= strip_ptr->current_state_expiry_time) { 
        if(reverse)
        {
//          strip_ptr->current_state = ((strip_ptr->current_state - 1 + pattern->strip_state_count) % pattern->strip_state_count);
        } else {
//          strip_ptr->current_state = ((strip_ptr->current_state + 1) % pattern->strip_state_count);
        }
        change_led_state(i, pattern, strip_ptr, current_time);
      }
    }
  }
  leds_render(true);
}
