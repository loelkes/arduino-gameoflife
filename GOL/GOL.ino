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

#define SERIAL_SPEED 115200

////////////////////////////
// G A M E   O F  L I F E //
////////////////////////////

#define WORLD_WIDTH 16
#define WORLD_HEIGHT 16

// Holds the current world. Note that ( 0, 0 ) is at the top left of the corner
// and x represents the horizontal and y the vertical axis.
int world[ WORLD_WIDTH ][ WORLD_HEIGHT ];

boolean triggerReset = false; // Set true to exit the loop.
int generation = 0;           // Current generation.
int cellsAlive = 0;           // Cells alive in the world.
int genDiff = 0;              // Difference from current to previous generation (amount of cells alive).
int prevGenDiff = 0;          // Difference from the previous generation to its ancestor (amount of cells alive).
int identicalGenCounter = 0;  // Counter of identical generation (amout of cells alive).

#define GENERATION_LIMIT 1000
#define IDENTICAL_GENERATION_LIMIT 10
#define GENERATION_DELAY 100
#define CELL_AGING true
#define SERIAL_MONITOR false

CRGB aliveColor = 0xFFFFFF;
CRGB deadColor = 0x000000;

//////////////////
// LED Settings //
//////////////////

#define LED_PIN     51
#define COLOR_ORDER GRB
#define CHIPSET     SK6812

#define SELFTEST_TIME       1000
#define SELFTEST_BRIGHTNESS 8
#define GAME_BRIGHTNESS     16

CRGB selfTestColors[] = { CRGB::Red, CRGB::Green, CRGB::Blue };

#define NUM_LEDS ( WORLD_WIDTH * WORLD_HEIGHT )
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1 ];
CRGB* const leds( leds_plus_safety_pixel + 1 );

// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;

//////////////////////////

void loop() {
  playGOL();
}

void setup() {

  Serial.begin( SERIAL_SPEED );
  Serial.print( millis() );
  Serial.println( " Hello World!" );
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>( leds, NUM_LEDS ).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness( GAME_BRIGHTNESS );
  pinMode(LED_BUILTIN, OUTPUT);

  selfTest();
  // Output some information.
  Serial.print( millis() );
  Serial.print( " Grid width (" );
  Serial.print( WORLD_WIDTH );
  Serial.print( ") and height (" );
  Serial.print( WORLD_HEIGHT );
  Serial.println( ")." );

  // Get seed for PRNG
  randomSeed( analogRead( 0 ) );

} // End of setup()

int count_neighbours( int x, int y ) {

  // count the number of neighbough live cells for a given cell
  int count = 0;
  boolean wrapX = true;
  boolean wrapY = true;

  int x_l = x - 1;
  int x_r = x + 1;
  int y_t = y - 1;
  int y_b = y + 1;
  
  
  if ( wrapX ) {
    if      ( x == 0 )                 x_l = WORLD_WIDTH - 1;
    else if ( x >= WORLD_WIDTH - 1 )   x_r = 0;
  }
  if ( wrapY ) {
    if      ( y == 0 )                 y_t = WORLD_HEIGHT - 1;
    else if ( y >= WORLD_HEIGHT - 1 )  y_b = 0;  
  }

  count += world[ x_l ][ y_t ] > 0; // above left
  count += world[  x  ][ y_t ] > 0; // above 
  count += world[ x_r ][ y_t ] > 0; // above right

  count += world[ x_l ][  y  ] > 0; // left
  count += world[ x_r ][  y  ] > 0; // right
  
  count += world[ x_l ][ y_b ] > 0; // bottom left
  count += world[  x  ][ y_b ] > 0; // bottom 
  count += world[ x_r ][ y_b ] > 0; // bottom right

  return count;

}

int newGeneration() {
  
  // Computes a new generation for a given world.
  // The int of each cell reprents the number of generations the cell is alive.
  // Use this number to change the cell (e.g. color) depending on it's age.

  // Crate a new world for the next generation.
  int newWorld[ WORLD_WIDTH ][ WORLD_HEIGHT ];

  // Increase the generation counter.
  generation++;

  // Toogle LED with each generation.
  digitalWrite(LED_BUILTIN, generation % 2); 

  // Check the neighbours for each cell
  for ( int y = 0; y < WORLD_HEIGHT; y++ ) {   // Top to bottom
    for ( int x = 0; x < WORLD_WIDTH; x++ ) {  // Left to right

      int neighbours = count_neighbours( x, y );

      if ( world[ x ][ y ] > 0 ) { // Cell is alive. Lives it it has 2 or 3 living neighbours.
        if ( ( neighbours == 2 ) || ( neighbours == 3 ) ) newWorld[ x ][ y ] = world[ x ][ y ] + 1;
        else newWorld[ x ][ y ] = 0; // Cell bas too many (> 3) or too few (< 2) neighbours and dies.
      } else { // Cell is dead. Needs 3 neighbours to spawn.
        if ( neighbours == 3 ) newWorld[ x ][ y ] = world[ x ][ y ] + 1;
        else newWorld[ x ][ y ] = 0; // Cell dies or remains dead.
      }
    }
  }

  // Apply new world to the current world
  prevGenDiff = genDiff;
  genDiff = 0; // How many changes in this generation.
  cellsAlive = 0;
  for ( int y = 0; y < WORLD_HEIGHT; y++ ) {
    for ( int x = 0; x < WORLD_WIDTH; x++ ) {
      if( world[ x ][ y ] == newWorld[ x ][ y ] ) genDiff++;
      else world[ x ][ y ] = newWorld[ x ][ y ];
      cellsAlive += world[ x ][ y ] > 0;
    }
  }
  checkReset();
} // End of newGeneration()

void checkReset() {
  // Chekc the world for reset condition. If they are met, set triggerReset to true
  // and return nothing.
    
   // Check if the world is not changing any more.
  if ( genDiff == prevGenDiff ) identicalGenCounter++;
  else identicalGenCounter = 0;
 
  if ( identicalGenCounter > IDENTICAL_GENERATION_LIMIT ) {
    triggerReset = true;
    Serial.print( millis() );
    Serial.print( " World did not change since " );
    Serial.print( identicalGenCounter );
    Serial.println( " generations. Resetting world!" );
    return;    
  }

  // Check if the world is empty
  if ( cellsAlive == 0 ) {
    triggerReset = true;
    Serial.print( millis() );
    Serial.println( " No more cells alive. Resetting world!" );
    return;
  }
  

  // Check if the generation limit ist met (hard reset).
  if ( generation > GENERATION_LIMIT ) {
    triggerReset = true;
    Serial.print( millis() );
    Serial.println( " Generation limit reached. Resetting world!" );
    return;
  }
} // End of checkReset()

void showSerialWorld() {

  for ( int y = 0; y < WORLD_HEIGHT; y++ ) {
    for ( int x = 0; x < WORLD_WIDTH; x++ ) {
      if ( world[ x ][ y ] > 0 ) Serial.print( "x" );
      else Serial.print( "." );
    }
    Serial.println();
  }

} // End of showSerialWorld()

void showLEDWorld() {
  
  FastLED.clear();
  for ( int x = 0; x < WORLD_WIDTH; x++ ) {    // Go from left to right
    for ( int y = 0; y < WORLD_HEIGHT; y++ ) { // Go from top to bottom
      #if CELL_AGING // Do not change color based on age.
        if ( world[ x ][ y ] > 0 ) leds[ XYsafe( x, y ) ].setHue( min( world[ x ][ y ], 255 ) ); 
        else leds[ XYsafe( x, y ) ] = deadColor;
      #else
        if ( world[ x ][ y ] > 0 ) leds[ XYsafe( x, y ) ] = aliveColor;
        else leds[ XYsafe( x, y ) ] = deadColor; 
      #endif
    }
  }
  FastLED.show();

} // End of showLEDWorld()

void showWorld( int target ){

  Serial.print( millis() );
  Serial.print( " World generation: " );
  Serial.print( generation );
  Serial.print( " | Cells alive: " );
  Serial.println( cellsAlive );

  #if SERIAL_MONITOR
    showSerialWorld();
  #endif
  
  showLEDWorld();

} // End of showWorld()

void randomWorld() {

  Serial.print( millis() );
  Serial.println( " Generating a new random world." );
  for ( int y = 0; y < WORLD_HEIGHT; y++ ) {
    for ( int x = 0; x < WORLD_WIDTH; x++ ) {
      int state = random( 0, 2 );
      world[ x ][ y ] = state;
      cellsAlive += state;
    }
  }

} // End of randomWorld()

void resetWorld() {
  generation = 0;
  cellsAlive = 0;
  randomWorld();
}

void playGOL() {

  Serial.print( millis() );
  Serial.println( " Starting a new game of life!" );
  resetWorld();
  while ( !triggerReset ) {
    // showWorld( 1 );
    showWorld( 2 );
    newGeneration();
    delay( GENERATION_DELAY );
  }
  triggerReset = false;

} // End of playGOL()

void selfTest() {

  Serial.print( millis() );
  Serial.println( " Performing selftest" );
  FastLED.clear();
  FastLED.setBrightness( SELFTEST_BRIGHTNESS );
  for ( byte c = 0; c < (sizeof( selfTestColors ) / sizeof( selfTestColors[ 0 ] ) ); c++ ) { // Go through a set of colors
    for ( int x = 0; x < WORLD_WIDTH; x++ ) {    // Go from left to right
      for ( int y = 0; y < WORLD_HEIGHT; y++ ) { // Go from top to bottom
        leds[ XYsafe( x, y ) ]  = selfTestColors[ c ];
      }
    }
    Serial.print( millis() );
    Serial.print( " Testing color " );
    Serial.println( c );
    FastLED.show();
    delay( SELFTEST_TIME );
  }
  FastLED.clear();
  FastLED.setBrightness( GAME_BRIGHTNESS );
  Serial.print( millis() );
  Serial.println( " Selftest complete!" );

} // End of selfTest()

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
//
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
//    for( uint8_t x = 0; x < WORLD_WIDTH; x++) {
//      for( uint8_t y = 0; y < WORLD_HEIGHT; y++) {
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
  if( kMatrixSerpentineLayout == false ) i = ( y * WORLD_WIDTH ) + x;

  if( kMatrixSerpentineLayout == true ) {
    if( y & 0x01 ) { // Odd rows run backwards
      uint8_t reverseX = ( WORLD_WIDTH - 1 ) - x;
      i = ( y * WORLD_WIDTH ) + reverseX;
    } else { // Even rows run forwards
      i = ( y * WORLD_WIDTH ) + x;
    }
  }
  return i;
}

uint16_t XYsafe( uint8_t x, uint8_t y ) {
  if( x >= WORLD_WIDTH ) return -1;
  if( y >= WORLD_HEIGHT ) return -1;
  return XY( x, y );
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
