#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <FastLED.h>

//Wifi manager starts as AccessPoint if there is no wifi connection
WiFiManager wifiManager;
ESP8266WebServer server(80);


// ACTION TO DO
// - when call the timer function, the last effect will be remember. This is also nessesery for brighness
// - over the air update 
// - remove all effects without nice working

//LED strip definition WS2812B
#define NUM_LEDS 648
CRGB leds[NUM_LEDS];
#define PIN D5
//#define DEBUG_ENABLE
int brightness = 0;
int effect = 0;
String effectSetting = "";
boolean effectChanged = true;
int lastEffect = 0;

boolean debug = true;

// interval to read data
unsigned long lastIntervalTime;
int interval = 1000;              //update interval for checking the temperature
unsigned long tmpTime = 0;        //use instead of delay for wait time
unsigned long tmpHandleTime = 0;  //Server handle time

void setup() {
  Serial.begin(115200);

  String deviceName = "http-" + String(ESP.getChipId()); //Generate device name based on ID
  Serial.print("wifiManager...");
  Serial.print(deviceName);
  wifiManager.autoConnect(deviceName.c_str(), ""); //Start wifiManager
  Serial.println("Connected!");
  Serial.print(" IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.mode(WIFI_STA);

//20190825 niet de indruk dat dit iets doet
//  if (MDNS.begin(deviceName)) {
//    Serial.println("MDNS responder started");
//  }

  server.on("/",            handleNotFound);
  server.on("/action",      handleAction);
  server.onNotFound(handleNotFound);
  server.begin();

  //FastLED.addLeds<WS2812B, PIN, GRB>(leds, NUM_LEDS); //.setCorrection( TypicalLEDStrip );
  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS); //.setCorrection( TypicalLEDStrip );

  Serial.println("HTTP server started");
}



/*  MAIN LOOP START */
void loop() {
  server.handleClient();
//20190825 niet de indruk dat dit iets doet
//MDNS.update();

  if(effectChanged){
    effectChanged = false;
    changeEffectTo(effect);
    doPrint("effect aangepast");
  }
  
  if(effect > 1){ //strobe effect only ones
    changeEffectTo(effect);
  }
}
/* MAIN LOOP END */


//HTTP REQUEST HANDLES
void handleNotFound() {
  String message = defaultMessage();
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleAction() {
  unsigned long tmpMillis = millis();
  
  String tmpBrightness = server.arg("brightness");
  String tmpEffect = server.arg("effect");
  String tmpRGB = server.arg("rgb");
  String tmpColor = server.arg("color");
  String tmpTimer = server.arg("timer");
  String tmpDebug = server.arg("debug");

  //check if debug is enabled
  debug = !(tmpDebug == "off");
  
  //set brightness to leds if it is set
  if(tmpBrightness.length() > 0){
    brightness = minMax(tmpBrightness, 0, 255);
    FastLED.setBrightness(brightness);
  }
  
  //set effect if it is set
  if(tmpEffect.length() > 0){
    effect = minMax(tmpEffect, 0, 16);
    effectChanged = true;
  }

  //if RGB is set then set effect to 1 and fill all leds with this color
  if(tmpRGB.length() == 9){
    effectSetting = tmpRGB;
    effect = 101;
    effectChanged = true;
  }

  //if color is set then set the leds by the number of the string
  //color=rrrgbrgbbb meens first 30%=red,next10%=green
  if(tmpColor.length() > 0){
    effectSetting = tmpColor;
    effect = 102;
    effectChanged = true;
  }

  //if timer is set then the leds will run from green to red within the given seconds
  //timer=60
  if(tmpTimer.length() > 0){
    effectSetting = tmpTimer;
    lastEffect = effect < 100 ? effect : -1;
    effect = 103;
    effectChanged = true;
  }
  
  unsigned long processTime = millis() - tmpMillis;

  String message = "";
  if(debug){
    message = defaultMessage();
    message += "Received arguments \n";
    message += " -brightness: " + tmpBrightness + "\n";
    message += " -effect: " + tmpEffect + "\n";
    message += " -rgb: " + tmpRGB + "\n";
    message += " -color: " + tmpColor + "\n";
    message += " -timer: " + tmpTimer + "\n";
    message += " -debug: " + String(debug) + "\n";
    message += "Process time: " + String(processTime) + " milliseconds \n";
  }
  server.send(200, "text/plain", message);
  
  unsigned long totalTime = millis() - tmpMillis;
  if(debug) Serial.println("Total process and send time: " + String(totalTime) + " ms");
}

String defaultMessage() {
  if(!debug) return "";
  String message = "\n\n\n\n\n\n\n\n\nWelkom bij Syp's ledstrip feestje \n";
  message += "/action \n";
  message += " -effect=x 1-16 number of effect \n";
  message += "   x=0 = all leds are off \n";
  message += "   x=1 = Strobe \n";
  message += "   x=2 = HalloweenEyes \n";
  message += "   x=3 = CylonBounce \n";
  message += "   x=4 = Twinkle \n";
  message += "   x=5 = TwinkleRandom \n";
  message += "   x=6 = Sparkle \n";
  message += "   x=7 = SnowSparkle \n";
  message += "   x=8 = RunningLights \n";
  message += "   x=9 = colorWipe \n";
  message += "   x=10 = rainbowCycle \n";
  message += "   x=11 = theaterChase \n";
  message += "   x=12 = theaterChaseRainbow \n";
  message += "   x=13 = Fire \n";
  message += "   x=14 = BouncingColoredBalls \n";
  message += "   x=15 = BouncingMultiColoredBalls \n";
  message += "   x=16 = meteorRain \n";
  message += " -brightness=xxx 0-255 like &brightness=10 \n";
  message += " -rgb=rrrgggbbb 3 times 000-255 like &rgb=250000124 \n";
  message += " -color=xxxx  number of x is divided over the number of LEDs (" + String(NUM_LEDS) + ") like &color=0012345670 \n";
  message += "   x=0 Black \n";  //0x000000
  message += "   x=1 Red \n";    //0xFF0000
  message += "   x=2 Lime \n";   //0x00FF00 
  message += "   x=3 Blue \n";   //0x0000FF
  message += "   x=4 Yellow \n"; //0xFFFF00
  message += "   x=5 Fuchsia \n";//0xFF00FF
  message += "   x=6 Aqua \n";   //0x00FFFF
  message += "   x=7 White \n";  //0xFFFFFF
  message += " -timer=xxx xxx is the number of seconds that alle green leds will go to red \n";
  message += " -debug on or off \n";
  return message;
}



//custom functions
void doPrint(String waarde){
  if(!debug) return;
  Serial.print(" Brigtness: " + String(brightness));
  Serial.print(" SelectedEffect: " + String(effect));
  Serial.print(" effectChanged: " + String(effectChanged));
  if(waarde != "") Serial.print(" " + waarde);
  Serial.println();
}

int minMax(String curValue, int minValue, int MaxValue){
  int tmpInt = curValue.toInt();
  int tmpResult = max(min(tmpInt,MaxValue),minValue);
  return tmpResult;
}



//LED strip effects
void changeEffectTo(int tmpEffect) {
  switch (tmpEffect) {

    case -1  : {
        // all leds set before this call
        showStrip();
        break;
      }

    case 0  : {
        // all leds off
        setAll(0, 0, 0);
        break;
      }

    case 1  : {
        // Strobe - Color (red, green, blue), number of flashes, flash speed, end pause
        Strobe(0xff, 0xff, 0xff, 10, 50, 1000);
        break;
      }

    case 2  : {
        // HalloweenEyes - Color (red, green, blue), Size of eye, space between eyes, fade (true/false), steps, fade delay, end pause
        HalloweenEyes(0xff, 0x00, 0x00,
                      1, 4,
                      true, random(5,50), random(50,150),
                      random(1000, 10000));
        HalloweenEyes(0xff, 0x00, 0x00,
                      1, 4,
                      true, random(5,50), random(50,150),
                      random(1000, 10000));
        break;
      }

    case 3  : {
        // CylonBounce - Color (red, green, blue), eye size, speed delay, end pause
        CylonBounce(0xff, 0x00, 0x00, 4, 20, 100);

        break;
      }

    case 4  : {
        // Twinkle - Color (red, green, blue), count, speed delay, only one twinkle (true/false)
        Twinkle(0xff, 0x00, 0x00, 10, 100, false);
        break;
      }

    case 5  : {
        // TwinkleRandom - twinkle count, speed delay, only one (true/false)
        TwinkleRandom(20, 100, false);
        break;
      }

    case 6  : {
        // Sparkle - Color (red, green, blue), speed delay
        Sparkle(0xff, 0xff, 0xff, 20);
        break;
      }

    case 7  : {
        // SnowSparkle - Color (red, green, blue), sparkle delay, speed delay
        SnowSparkle(0x10, 0x10, 0x10, 20, random(100, 1000));
        break;
      }

    case 8 : {
        // Running Lights - Color (red, green, blue), wave delay
        RunningLights(0xff, 0x00, 0x00, 50); // red
        RunningLights(0xff, 0xff, 0xff, 50); // white
        RunningLights(0x00, 0x00, 0xff, 50); // blue
        break;
      }

    case 9 : {
        // colorWipe - Color (red, green, blue), speed delay
        colorWipe(0xff, 0xff, 0x00, 50);
        colorWipe(0x00, 0xff, 0x00, 50);
        colorWipe(0x00, 0xff, 0xff, 50);
        colorWipe(0x00, 0x00, 0xff, 50);
        colorWipe(0xff, 0x00, 0xff, 50);
        colorWipe(0xff, 0x00, 0x00, 50);
        //colorWipe(0x00, 0x00, 0x00, 50);
        break;
      }

    case 10 : {
        // rainbowCycle - speed delay
        rainbowCycle(20);
        break;
      }

    case 11 : {
        // theatherChase - Color (red, green, blue), speed delay
        theaterChase(0xff, 0, 0, 50);
        break;
      }

    case 12 : {
        // theaterChaseRainbow - Speed delay
        theaterChaseRainbow(50);
        break;
      }

    case 13 : {
        // Fire - Cooling rate, Sparking rate, speed delay
        Fire(55, 120, 15);
        break;
      }

    case 14 : {
        // mimic BouncingBalls
        byte onecolor[1][3] = { {0xff, 0x00, 0x00} };
        BouncingColoredBalls(1, onecolor, false);
        break;
      }

    case 15 : {
        // multiple colored balls
        byte colors[3][3] = { {0xff, 0x00, 0x00},
          {0xff, 0xff, 0xff},
          {0x00, 0x00, 0xff}
        };
        BouncingColoredBalls(3, colors, false);
        break;
      }

    case 16 : {
        // meteorRain - Color (red, green, blue), meteor size, trail decay, random trail decay (true/false), speed delay
        meteorRain(0xff, 0xff, 0xff, 10, 64, true, 30);
        break;
      }

    case 101 : {
        // custom effect set all leds based on RGB value
        custom101(effectSetting);
        break;
    }
    
    case 102 : {
        // custom effect set leds based on sequence in effectSetting
        custom102(effectSetting);
        break;
    }

    case 103 : {
        // custom effect set alle leds to green en run to red by the giving seconds
        custom103(effectSetting);
        break;
    }
  }
}




// *************************
// ** LEDEffect Functions **
// *************************

void RGBLoop() {
  for (int j = 0; j < 3; j++ ) {
    // Fade IN
    for (int k = 0; k < 256; k++) {
      switch (j) {
        case 0: setAll(k, 0, 0); break;
        case 1: setAll(0, k, 0); break;
        case 2: setAll(0, 0, k); break;
      }
      showStrip();
      delay(3);
    }
    // Fade OUT
    for (int k = 255; k >= 0; k--) {
      switch (j) {
        case 0: setAll(k, 0, 0); break;
        case 1: setAll(0, k, 0); break;
        case 2: setAll(0, 0, k); break;
      }
      showStrip();
      delay(3);
    }
  }
}

void FadeInOut(byte red, byte green, byte blue) {
  float r, g, b;

  for (int k = 0; k < 256; k = k + 1) {
    r = (k / 256.0) * red;
    g = (k / 256.0) * green;
    b = (k / 256.0) * blue;
    setAll(r, g, b);
    showStrip();
  }

  for (int k = 255; k >= 0; k = k - 2) {
    r = (k / 256.0) * red;
    g = (k / 256.0) * green;
    b = (k / 256.0) * blue;
    setAll(r, g, b);
    showStrip();
  }
}

void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
  for (int j = 0; j < StrobeCount; j++) {
    setAll(red, green, blue);
    showStrip();
    myDelay(FlashDelay);
    setAll(0, 0, 0);
    showStrip();
    myDelay(FlashDelay);
    if(effectChanged) return;
  }

  myDelay(EndPause);
}

void HalloweenEyes(byte red, byte green, byte blue,
                   int EyeWidth, int EyeSpace,
                   boolean Fade, int Steps, int FadeDelay,
                   int EndPause) {

  int i;
  int StartPoint  = random( 0, NUM_LEDS - (2 * EyeWidth) - EyeSpace );
  int Start2ndEye = StartPoint + EyeWidth + EyeSpace;

  for (i = 0; i < EyeWidth; i++) {
    setPixel(StartPoint + i, red, green, blue);
    setPixel(Start2ndEye + i, red, green, blue);
  }

  showStrip();

  if (Fade == true) {
    float r, g, b;

    for (int j = Steps; j >= 0; j--) {
      r = j * (red / Steps);
      g = j * (green / Steps);
      b = j * (blue / Steps);

      for (i = 0; i < EyeWidth; i++) {
        setPixel(StartPoint + i, r, g, b);
        setPixel(Start2ndEye + i, r, g, b);
      }

      showStrip();
      myDelay(FadeDelay);
      if(effectChanged) return;
    }
  }

  setAll(0, 0, 0); // Set all black

  myDelay(EndPause);
}

void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {

  for (int i = 0; i < NUM_LEDS - EyeSize - 2; i++) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
  }

  myDelay(ReturnDelay);

  for (int i = NUM_LEDS - EyeSize - 2; i > 0; i--) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
  }

  myDelay(ReturnDelay);
}

void Twinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0);

  for (int i = 0; i < Count; i++) {
    setPixel(random(NUM_LEDS), red, green, blue);
    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
    if (OnlyOne) {
      setAll(0, 0, 0);
    }
  }

  myDelay(SpeedDelay);
}

void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0);

  for (int i = 0; i < Count; i++) {
    setPixel(random(NUM_LEDS), random(0, 255), random(0, 255), random(0, 255));
    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
    if (OnlyOne) {
      setAll(0, 0, 0);
    }
  }

  myDelay(SpeedDelay);
}

void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel, red, green, blue);
  showStrip();
  myDelay(SpeedDelay);
  setPixel(Pixel, 0, 0, 0);
}

void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red, green, blue);

  int Pixel = random(NUM_LEDS);
  setPixel(Pixel, 0xff, 0xff, 0xff);
  showStrip();
  myDelay(SparkleDelay);
  setPixel(Pixel, red, green, blue);
  showStrip();
  myDelay(SpeedDelay);
}

void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position = 0;

  for (int i = 0; i < NUM_LEDS * 2; i++)
  {
    Position++; // = 0; //Position + Rate;
    for (int i = 0; i < NUM_LEDS; i++) {
      // sine wave, 3 offset waves make a rainbow!
      //float level = sin(i+Position) * 127 + 128;
      //setPixel(i,level,0,0);
      //float level = sin(i+Position) * 127 + 128;
      setPixel(i, ((sin(i + Position) * 127 + 128) / 255)*red,
               ((sin(i + Position) * 127 + 128) / 255)*green,
               ((sin(i + Position) * 127 + 128) / 255)*blue);
    }

    showStrip();
    myDelay(WaveDelay);
    if(effectChanged) return;
  }
}

void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    setPixel(i, red, green, blue);
    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
  }
}

void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  //for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
  for (j = 0; j < 256; j++) { // 1 cycles of all colors on wheel
    for (i = 0; i < NUM_LEDS; i++) {
      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      setPixel(i, *c, *(c + 1), *(c + 2));
    }
    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
  }
}

// used by rainbowCycle and theaterChaseRainbow
byte * Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}

void theaterChase(byte red, byte green, byte blue, int SpeedDelay) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        setPixel(i + q, red, green, blue);  //turn every third pixel on
      }
      showStrip();

      myDelay(SpeedDelay);

      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        setPixel(i + q, 0, 0, 0);    //turn every third pixel off
      }
    }
  }
}

void theaterChaseRainbow(int SpeedDelay) {
  byte *c;

  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        c = Wheel( (i + j) % 255);
        setPixel(i + q, *c, *(c + 1), *(c + 2)); //turn every third pixel on
      }
      showStrip();

      myDelay(SpeedDelay);
      if(effectChanged) return;

      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        setPixel(i + q, 0, 0, 0);    //turn every third pixel off
        if(effectChanged) return;
      }
    }
  }
}

void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  showStrip();
  myDelay(SpeedDelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void BouncingColoredBalls(int BallCount, byte colors[][3], boolean continuous) {
  float Gravity = -9.81;
  int StartHeight = 1;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];
  boolean ballBouncing[BallCount];
  boolean ballsStillBouncing = true;

  for (int i = 0 ; i < BallCount ; i++) {
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i) / pow(BallCount, 2);
    ballBouncing[i] = true;
  }

  while (ballsStillBouncing) {
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i] / 1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i] / 1000;

      if ( Height[i] < 0 ) {
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();

        if ( ImpactVelocity[i] < 0.01 ) {
          if (continuous) {
            ImpactVelocity[i] = ImpactVelocityStart;
          } else {
            ballBouncing[i] = false;
          }
        }
      }
      Position[i] = round( Height[i] * (NUM_LEDS - 1) / StartHeight);
    }

    ballsStillBouncing = false; // assume no balls bouncing
    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i], colors[i][0], colors[i][1], colors[i][2]);
      if ( ballBouncing[i] ) {
        ballsStillBouncing = true;
      }
    }

    showStrip();
    myDelay(1);
    setAll(0, 0, 0);
    if(effectChanged) return;
  }
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {
  setAll(0, 0, 0);

  for (int i = 0; i < NUM_LEDS + NUM_LEDS; i++) {


    // fade brightness all LEDs one step
    for (int j = 0; j < NUM_LEDS; j++) {
      if ( (!meteorRandomDecay) || (random(10) > 5) ) {
        fadeToBlack(j, meteorTrailDecay );
      }
    }

    // draw meteor
    for (int j = 0; j < meteorSize; j++) {
      if ( ( i - j < NUM_LEDS) && (i - j >= 0) ) {
        setPixel(i - j, red, green, blue);
      }
    }

    showStrip();
    myDelay(SpeedDelay);
    if(effectChanged) return;
  }
}

// used by meteorrain
void fadeToBlack(int ledNo, byte fadeValue) {
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  uint32_t oldColor;
  uint8_t r, g, b;
  int value;

  oldColor = strip.getPixelColor(ledNo);
  r = (oldColor & 0x00ff0000UL) >> 16;
  g = (oldColor & 0x0000ff00UL) >> 8;
  b = (oldColor & 0x000000ffUL);

  r = (r <= 10) ? 0 : (int) r - (r * fadeValue / 256);
  g = (g <= 10) ? 0 : (int) g - (g * fadeValue / 256);
  b = (b <= 10) ? 0 : (int) b - (b * fadeValue / 256);

  strip.setPixelColor(ledNo, r, g, b);
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  leds[ledNo].fadeToBlackBy( fadeValue );
#endif
}

// *** REPLACE TO HERE ***


// *** CUSTOM FUNCTIONS ***
void custom101(String setting){
    byte newR = byte(minMax(setting.substring(0,3),0,255));
    byte newG = byte(minMax(setting.substring(3,6),0,255));
    byte newB = byte(minMax(setting.substring(6,9),0,255));
    setAll(newR, newG, newB);
    showStrip();
    effect = -1;
}
    
void custom102(String setting){
    for(int s = 0; s < setting.length(); s++){
      int factor = NUM_LEDS / setting.length();
      int x = s * factor;
      int y = x + factor;
      for(int i = x; i < y; i++){
        String clr = setting.substring(s, s+1);
        if(clr.equals("0")) setPixel(i,          0,          0,          0);
        if(clr.equals("1")) setPixel(i, brightness,          0,          0);
        if(clr.equals("2")) setPixel(i,          0, brightness,          0);
        if(clr.equals("3")) setPixel(i,          0,          0, brightness);
        if(clr.equals("4")) setPixel(i, brightness, brightness,          0);
        if(clr.equals("5")) setPixel(i, brightness,          0, brightness);
        if(clr.equals("6")) setPixel(i,          0, brightness, brightness);
        if(clr.equals("7")) setPixel(i, brightness, brightness, brightness);
      }
    }

    showStrip();
    effect = -1;
}       

void custom103(String setting){
    //timer function
    doPrint(setting);
    //all to green
    setAll(0, brightness, 0);
    showStrip();
    myDelay(1000);
    
    unsigned long tmpTime = millis();
    unsigned long tmpDeltaTime = (setting.toInt() * 1000) / NUM_LEDS;
    int curLed = 1;

    do{
      //check is interval time is reached => set next led to red
      if( tmpTime + (curLed * tmpDeltaTime) < millis() ){
        for (int i = 0; i < NUM_LEDS; i++) {
          if( i < curLed ){
            setPixel(i, brightness, 0, 0);
          }else{
            setPixel(i, 0, brightness, 0);
          }
        }    
        showStrip();
        myDelay(1);
        if(effectChanged) return;
        
        curLed++;
      }
      myDelay(1);
      if(effectChanged) return;
    } while (curLed <= NUM_LEDS);
    
    //flash leds
    Strobe(0xff, 0xff, 0xff, 10, 50, 1000);
    Strobe(0xff, 0xff, 0xff, 10, 50, 1000);
    Strobe(0xff, 0xff, 0xff, 10, 50, 1000);
    //showStrip();
    //set effect back to last effect
    effect = lastEffect;
    effectChanged = true;
}   


// ***************************************
// ** FastLed/NeoPixel Common Functions **
// ***************************************

// Apply LED color changes
void showStrip() {
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  strip.show();
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  FastLED.show();
#endif
  myDelay(1);
}

// Set a LED color (not yet visible)
void setPixel(int Pixel, byte red, byte green, byte blue) {
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
#endif
}

// Set all LEDs to a given color and apply it (visible)
void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void myDelay(unsigned long timeToWait){
  //delay(timeToWait);
  server.handleClient(); //handle incomming http requests
  tmpTime = millis();
  while( (millis() - tmpTime < timeToWait) ){ //&& !effectChanged){
    if(millis() - tmpHandleTime > 100){
      tmpHandleTime = millis();
      server.handleClient();
    }
    if(effectChanged) return;
  }
}
