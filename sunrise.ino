// Simulate a sunrise.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    6

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 12

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

// Color gradients used to simulate sunrise
// Based on https://docs.google.com/spreadsheets/d/1jNqVLzl-nsXRPoXtlj6pjFGAwI1ylIB1Ep_PBgpyqgo/edit#gid=0
// Green gradients have been reduced by 90
// Blue gradients have been smoothed manually
// Steps must be the same size as the arrays
int steps = 15;
int stepSize = 255/steps;
int phaseLength = 120000 / steps;
int brightness = 255;
int red[]   = {255,255,255,255,255,255,255,255,255,255,255,255,255,223,161};
int green[] = {25,31,39,66,72,78,89,96,101,138,150,155,159,141,101};
int blue[]  = {0,0,0,20,34,66,88,99,111,150,190,246,255,255,255};

// setup() function -- runs once at startup --------------------------------

void setup() {
  strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();             // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)
}

// loop() function -- runs repeatedly as long as board is on ---------------

void loop() {
  for(int i = 0; i < steps-1; i++) {
    int brightnessFactor = ((i+1) * stepSize) * 100/255;
    int redLED = (red[i] * brightnessFactor) / 100;
    int greenLED = (green[i] * brightnessFactor) /100;
    int blueLED = (blue[i] * brightnessFactor) /100;
    strip.fill(strip.Color(redLED, greenLED, blueLED, 0));
    strip.show();
    delay(phaseLength);
  }
  strip.clear();
  strip.show();
  delay(2000);
}
