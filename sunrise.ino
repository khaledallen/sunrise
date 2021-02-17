#include "arduino_secrets.h"
/*
  Sunrise

 This sketch connects to a website ( https://api.sunrise-sunset.org)
 using a WiFi shield and initiates a sunrise simulation when the connected
 Real-time Clock time matches the sunrise hour and minute.
 
 Check for the new sunrise occurs at midnight;
 
 https://api.sunrise-sunset.org/json?lat=36.7201600&lng=-4.4203400&date=today

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the Wifi.begin() call accordingly.

 Circuit:
 * WiFi shield attached
 * PCF8523 RTC connected via I2C and Wire lib
 * Adafruit Neopixel 12LED Ring on pin 6

 created 13 July 2010 by dlf (Metodo2 srl)
 modified 31 May 2012 by Tom Igoe
 modified 15 Feb 2021 by Khaled Allen
 */

#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <RTClib.h> // Make sure to use the downloaded github version, NOT the web editor version
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
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
const int steps = 15;
const int stepSize = 255/steps;
const int totalDuration = 120000; // how long the sequence should run in milliseconds
const int phaseLength = totalDuration / steps; // divide two minutes into the number of steps available
const int brightness = 255;

int red[]   = {255,255,255,255,255,255,255,255,255,255,255,255,255,223,161};
int green[] = {25,31,39,66,72,78,89,96,101,138,150,155,159,141,101};
int blue[]  = {0,0,0,20,34,66,88,99,111,150,190,246,255,255,255};
bool risen = false;

// Wifi login credentials set in arduino_secrets.h
char ssid[] = SECRET_SSID;                               //  your network SSID (name)
char pass[] = SECRET_PWD;             // your network password (use for WPA, or use as key for WEP)
char server[] = "api.sunrise-sunset.org";

// declare the real-time clock
RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// object to store the next sunrise
DateTime nextRise;

// Allocate the JSON document
// For parsing the response from the sunrise API
// Inside the brackets, 512 is the capacity of the memory pool in bytes.
// Use arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<512> doc;

int status = WL_IDLE_STATUS;

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(57600);
  
  while (!Serial) {
    ;
  }
  Serial.println("Initializing");
  startRTC();
  setupWifi();
  
  strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();             // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)

}

void loop() {
  DateTime now = rtc.now();
  now = (now + TimeSpan(0,-7,0,0)); // adjusting timezone offset because the webcompiler is on Zulu
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.println("");
  // if it's midnight, get the next sunrise
  if(now.hour() == 0) {
    risen = false;
    nextRise = getSunrise();
  }
  // if it's the right hour and minute, and we haven't risen, trigger the sunrise
  if(now.hour() == nextRise.hour() && now.minute() == nextRise.minute() && !risen) {
    sunrise();
    risen = true;
  }
}

void sunrise() {
  for(int i = 0; i < steps-1; i++) {
    int brightnessFactor = ((i+1) * stepSize) * 100/255;
    int redLED = (red[i] * brightnessFactor) / 100;
    int greenLED = (green[i] * brightnessFactor) /100;
    int blueLED = (blue[i] * brightnessFactor) /100;
    strip.fill(strip.Color(redLED, greenLED, blueLED, 0));
    strip.show();
    delay(phaseLength);
  }
  delay(10000); //wait ten seconds, then turn it off
  strip.clear();
  strip.show();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

DateTime getSunrise() {
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET https://api.sunrise-sunset.org/json?lat=36.7201600&lng=-4.4203400&date=today");
    client.println("Host: api.sunrise-sunset.org");
    client.println("Connection: close");
    client.println();
  }
  
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, client);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    abort();
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  const char * sunrise = doc["results"]["sunrise"];

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
  }
  char * formattedSunrise = formatSunriseFromAPI(sunrise);
  DateTime riseTime = DateTime(F(__DATE__), F(formattedSunrise));
  return riseTime;
}

char * formatSunriseFromAPI(const char * APIsunrise) {
  String adjustedRise = String() + "0" + APIsunrise;
  int endIdx = adjustedRise.lastIndexOf(" ");
  adjustedRise.remove(endIdx);
  char cbuff[adjustedRise.length() - 2];
  adjustedRise.toCharArray(cbuff, adjustedRise.length() - 2);
  return cbuff;
}

void setupWifi() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 5 seconds for connection:
    delay(5000);
  }
  
  Serial.println("Connected to wifi");
  printWifiStatus();
}

void startRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  
  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }
  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();
  Serial.println("System time at compile was");
  Serial.println(__TIME__);
}
