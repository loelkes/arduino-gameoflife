// Copyright 2019 Christian Lölkes
//
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <FastLED.h>

#define LED_PIN  51
#define COLOR_ORDER GRB
#define CHIPSET     SK6812
#define BRIGHTNESS 64
#define GENERATION_LIMIT 1000

// Helper functions for an two-dimensional XY matrix of pixels.
// Simple 2-D demo code is included as well.
//
//     XY(x,y) takes x and y coordinates and returns an LED index number,
//             for use like this:  leds[ XY(x,y) ] == CRGB::Red;
//             No error checking is performed on the ranges of x and y.
//
//     XYsafe(x,y) takes x and y coordinates and returns an LED index number,
//             for use like this:  leds[ XY(x,y) ] == CRGB::Red;
//             Error checking IS performed on the ranges of x and y, and an
//             index of "-1" is returned.  Special instructions below
//             explain how to use this without having to do your own error
//             checking every time you use this function.  
//             This is a slightly more advanced technique, and 
//             it REQUIRES SPECIAL ADDITIONAL setup, described below.


// Params for width and height
const uint8_t kMatrixWidth = 80;
const uint8_t kMatrixHeight = 8;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;

// Set 'kMatrixSerpentineLayout' to false if your pixels are 
// laid out all running the same way, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//     .----<----<----<----'
//     |
//     5 >  6 >  7 >  8 >  9
//                         |
//     .----<----<----<----'
//     |
//    10 > 11 > 12 > 13 > 14
//                         |
//     .----<----<----<----'
//     |
//    15 > 16 > 17 > 18 > 19
//
// Set 'kMatrixSerpentineLayout' to true if your pixels are 
// laid out back-and-forth, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//                         |
//     9 <  8 <  7 <  6 <  5
//     |
//     |
//    10 > 11 > 12 > 13 > 14
//                        |
//                        |
//    19 < 18 < 17 < 16 < 15
//
// Bonus vocabulary word: anything that goes one way 
// in one row, and then backwards in the next row, and so on
// is call "boustrophedon", meaning "as the ox plows."


// This function will return the right 'led index number' for 
// a given set of X and Y coordinates on your matrix.  
// IT DOES NOT CHECK THE COORDINATE BOUNDARIES.  
// That's up to you.  Don't pass it bogus values.
//
// Use the "XY" function like this:
//
//    for( uint8_t x = 0; x < kMatrixWidth; x++) {
//      for( uint8_t y = 0; y < kMatrixHeight; y++) {
//      
//        // Here's the x, y to 'led index' in action: 
//        leds[ XY( x, y) ] = CHSV( random8(), 255, 255);
//      
//      }
//    }
//
//

uint16_t XY( uint8_t x, uint8_t y ) {
  
  uint16_t i;
  if( kMatrixSerpentineLayout == false ) {
    i = ( y * kMatrixWidth ) + x;
  }

  if( kMatrixSerpentineLayout == true ) {
    if( y & 0x01 ) { // Odd rows run backwards
      uint8_t reverseX = ( kMatrixWidth - 1 ) - x;
      i = ( y * kMatrixWidth ) + reverseX;
    } else { // Even rows run forwards
      i = ( y * kMatrixWidth ) + x;
    }
  }
  return i;
}


// Once you've gotten the basics working (AND NOT UNTIL THEN!)
// here's a helpful technique that can be tricky to set up, but 
// then helps you avoid the needs for sprinkling array-bound-checking
// throughout your code.
//
// It requires a careful attention to get it set up correctly, but
// can potentially make your code smaller and faster.
//
// Suppose you have an 8 x 5 matrix of 40 LEDs.  Normally, you'd
// delcare your leds array like this:
//    CRGB leds[40];
// But instead of that, declare an LED buffer with one extra pixel in
// it, "leds_plus_safety_pixel".  Then declare "leds" as a pointer to
// that array, but starting with the 2nd element (id=1) of that array: 
//    CRGB leds_with_safety_pixel[41];
//    CRGB* const leds( leds_plus_safety_pixel + 1);
// Then you use the "leds" array as you normally would.
// Now "leds[0..N]" are aliases for "leds_plus_safety_pixel[1..(N+1)]",
// AND leds[-1] is now a legitimate and safe alias for leds_plus_safety_pixel[0].
// leds_plus_safety_pixel[0] aka leds[-1] is now your "safety pixel".
//
// Now instead of using the XY function above, use the one below, "XYsafe".
//
// If the X and Y values are 'in bounds', this function will return an index
// into the visible led array, same as "XY" does.
// HOWEVER -- and this is the trick -- if the X or Y values
// are out of bounds, this function will return an index of -1.
// And since leds[-1] is actually just an alias for leds_plus_safety_pixel[0],
// it's a totally safe and legal place to access.  And since the 'safety pixel'
// falls 'outside' the visible part of the LED array, anything you write 
// there is hidden from view automatically.
// Thus, this line of code is totally safe, regardless of the actual size of
// your matrix:
//    leds[ XYsafe( random8(), random8() ) ] = CHSV( random8(), 255, 255);
//
// The only catch here is that while this makes it safe to read from and
// write to 'any pixel', there's really only ONE 'safety pixel'.  No matter
// what out-of-bounds coordinates you write to, you'll really be writing to
// that one safety pixel.  And if you try to READ from the safety pixel,
// you'll read whatever was written there last, reglardless of what coordinates
// were supplied.

#define NUM_LEDS ( kMatrixWidth * kMatrixHeight )
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1 ];
CRGB* const leds( leds_plus_safety_pixel + 1 );

uint16_t XYsafe( uint8_t x, uint8_t y ) {

  if( x >= kMatrixWidth ) return -1;
  if( y >= kMatrixHeight ) return -1;
  return XY( x, y );

}


// Demo that USES "XY" follows code below

void loop() {

  playGOL();

}

void setup() {
  
  Serial.begin( 115200 );
  Serial.print( millis() );
  Serial.println( " Hello World!" );
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>( leds, NUM_LEDS ).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness( BRIGHTNESS );

  // Output some information.
  Serial.print( millis() );
  Serial.print( " Grid width )(" );
  Serial.print( kMatrixWidth );
  Serial.print( ") and height (" );
  Serial.print( kMatrixHeight );
  Serial.println( ")." );

}


////////////////////////////
// G A M E   O F  L I F E //
////////////////////////////

// Holds the current world!
boolean world[ kMatrixWidth ][ kMatrixHeight ];

// Counts the number of elapsed generations
int generation;

int count_neighbours( int y, int x ) {
  
  // count the number of neighbough live cells for a given cell
  int count = 0;
  
  // Check above
  if( y > 0 ) { // We are not on the first row. There is a row above
    if ( x > 0            ) count += world[ x -1 ][ y - 1 ];  // above left
                            count += world[   x  ][ y - 1 ];  // above center
    if ( x < kMatrixWidth ) count += world[ x +1 ][ y - 1 ];  // above right
  }

  // Check the same row
  if ( x > 0            )   count += world[ x - 1 ][ y ];     // same left
  if ( x < kMatrixWidth )   count += world[ x + 1 ][ y ];     // same right
  
  // Check below
  if ( y < kMatrixHeight ) { // We are not the last row. There is a row below
    if ( x > 0            ) count += world[ x - 1 ][ y + 1 ]; // below left
                            count += world[   x   ][ y + 1 ]; // below center
    if ( x < kMatrixWidth ) count += world[ x + 1 ][ y + 1 ]; // below right
  }
  
  return count;

}

int newGeneration() {

  // Crate a new world for the next generation
  boolean newWorld[ kMatrixWidth ][ kMatrixHeight ];
  generation++;

  // Check the neighbours for each cell
  for ( int y = 0; y < kMatrixWidth; y++ ) {      // Go from left to right
    for ( int x = 0; x < kMatrixHeight; x++ ) {   // Go from top to bottom
      
      int neighbours = count_neighbours( y, x );
      if ( world[ y ][ x ] > 1 ) {                
        // Cell is alive. Lives it it has 2 or 3 living neighbours.
        if ( ( neighbours == 2 ) || ( neighbours == 3 ) ) newWorld[ y ][ x ] = 1;
        // Cell bas too many (> 3) or too few (< 2) neighbours and dies.
        else newWorld[ y ][ x ] = 0;
      } else {                                    
        // Cell is dead. Needs 3 neighbours to spawn.
        if ( neighbours == 3 ) newWorld[ y ][ x ] = 1;
        // Cell dies or remains dead.
        // else newWorld[ y ][ x ] = 0;
      }
      
    }
  }

  // Apply world
  int generationChanges = 0;
  for ( int y = 0; y < kMatrixWidth; y++ ) {    // Go from left to right
    for ( int x = 0; x < kMatrixHeight; x++ ) { // Go from top to bottom
      
      if( world[ x ][ y ] == newWorld[ x ][ y ] ) generationChanges++;
      else world[ x ][ y ] = newWorld[ x ][ y ];

    }
  }

}

void showSerialWorld() {

  Serial.print( millis() );
  Serial.println( " The world at generation " );
  Serial.println( generation );
  for ( int x = 0; x < kMatrixHeight; x++ ) {   // Go from top to bottom
    for ( int y = 0; y < kMatrixWidth; y++ ) {  // Go from left to right
      
      if ( world[ x ][ y ] > 0 ) Serial.print( "x" );
      else Serial.print( "." );

    }
    Serial.println();
  }

}

void showLEDWorld() {

  FastLED.clear();
  int aliveColor = 0xFFFFFF;
  int deadColor = 0x000000;
  for ( int y = 0; y < kMatrixWidth; y++ ) {    // Go from left to right
    for ( int x = 0; x < kMatrixHeight; x++ ) { // Go from top to bottom
      
      if ( world[ x ][ y ] > 0 ) leds[ XYsafe( x, y ) ] = aliveColor;
      else leds[ XYsafe( x, y ) ] = deadColor;
      
    }
  }
  FastLED.show();

}

void showWorld( int target ){

  // Set target = 1 for Serial port
  // Set target = 2 for LEDs
  if ( target == 1 ) showSerialWorld();
  if ( target == 2 ) showLEDWorld();

}

void randomWorld() {

  Serial.print( millis() );
  Serial.println( " Generating a new random world." );
  generation = 0;
  for ( int y = 0; y < kMatrixWidth; y++ ) {    // Go from left to right
    for ( int x = 0; x < kMatrixHeight; x++ ) { // Go from top to bottom
      
      world[ x ][ y ] = random( 0, 2 );
      
    }
  }

}

void playGOL() {
  
  Serial.print( millis() );
  Serial.println( "Starting a new game of life!" );
  randomWorld();
  while ( generation < GENERATION_LIMIT ) {
    if ( newGeneration() == 0 ) break; // Nothing changes, world is dead!
    showWorld( 1 );
    delay( 1000 );
  }

}

void selfTest() {

  Serial.print( millis() );
  Serial.println( " Performing selftest" );
  FastLED.clear();
  FastLED.setBrightness( 1 );
  int colors[] = { 0xFF0000, 0x00FF00, 0x0000FF };
  for ( int c = 0; c < 3; c++ ) {                 // Go through a set of colors
    for ( int y = 0; y < kMatrixWidth; y++ ) {    // Go from left to right
      for ( int x = 0; x < kMatrixHeight; x++ ) { // Go from top to bottom
        
        leds[ XYsafe( x, y ) ]  = colors[ c ]; 
        
      }
    }
    FastLED.show();
    delay( 1000 );
  }
  FastLED.clear();
  Serial.print( millis() );
  Serial.println( " Selftest complete!" );

}
