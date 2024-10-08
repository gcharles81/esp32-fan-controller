#include "config.h"
#include "fanPWM.h"
#include "fanTacho.h"
#include "log.h"
#include "sensorBME280.h"
#include "temperatureController.h"
#include "tft.h"

#ifdef DRIVER_ILI9341
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSans9pt7b.h>
#endif
#ifdef DRIVER_ST7735
#include <Adafruit_ST7735.h>
#endif
#ifdef TFT_DRIVER_CHGA
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <TFT_eSPI.h>
#endif
//prepare driver for display
#ifdef DRIVER_ILI9341
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
#endif
#ifdef DRIVER_ST7735
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);
#endif


#ifdef TFT_DRIVER_CHGA
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
#endif

  uint16_t bg_color = TFT_BLACK;       // This is the background colour used for smoothing (anti-aliasing)
  uint16_t ng_color = 0x08a4;//TFT_DARKGREY;
  uint16_t fg_color = 0x5E9E;//TFT_CYAN;
  uint16_t x = 122;  // Position of centre of arc
  uint16_t y = 40;

  uint8_t radius       = 36; // Outer arc radius
  uint8_t thickness    = 7;
  uint8_t inner_radius = radius - thickness;        // Calculate inner radius (can be 0 for circle segment)
  // 0 degrees is at 6 o'clock position
  // Arcs are drawn clockwise from start_angle to end_angle
  // Start angle can be greater than end angle, the arc will then be drawn through 0 degrees
  uint16_t start_angle = 30; // Start angle must be in range 0 to 360
  uint16_t end_angle   = 240; // End angle must be in range 0 to 360
  uint16_t end_angle_Blank   = 330; // End angle must be in range 0 to 360
boolean screenUpdateRequired = false;  // Boolean flag for screen update
boolean TFTOTA = false;
#ifdef useTFT
const GFXfont *myFont;
int textSizeOffset;

// number of screen to display
int screen = SCREEN_NORMALMODE;

unsigned long startCountdown = 0;

void calcDimensionsOfElements(void);
void draw_screen(void);
#endif

void initTFT(void) {
  #ifdef useTFT
  // start driver
  #ifdef DRIVER_ILI9341
  // switch display on
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LED_ON);
  tft.begin();
  myFont = &FreeSans9pt7b;
  textSizeOffset = 0;
  #endif
  #ifdef DRIVER_ST7735
  tft.initR(INITR_BLACKTAB);
  myFont = NULL;
  textSizeOffset = 0;
  #endif
#ifdef TFT_DRIVER_CHGA
  tft.begin();
  tft.setRotation(3);  // portrait
  tft.fillScreen(TFT_BLUE);
    Guage_test();
#endif


  // show the displays resolution
  Log.printf("  TFT sucessfully initialized.\r\n");
  Log.printf("  tftx = %d, tfty = %d\r\n", tft.width(), tft.height());

  #else
  Log.printf("    TFT is disabled in config.h\r\n");
  #endif
}

#ifdef useTFT
int16_t getRelativeX(int16_t xBasedOnTFTwithScreenWidth320px) {
  return (float)(xBasedOnTFTwithScreenWidth320px) /(float)(320) * tft_getWidth();;
}

int16_t getRelativeY(int16_t yBasedOnTFTwithScreenHeight240px) {
  return (float)(yBasedOnTFTwithScreenHeight240px)/(float)(240) * tft_getHeight();;
}

// rect: x, y, width, heigth
int valueUpRect[4];
int valueDownRect[4];
#if defined (useStandbyButton) || defined(useShutdownButton)
int shutdownRect[4];
int confirmShutdownYesRect[4];
int confirmShutdownNoRect[4];
#endif

int plusMinusHorizontalLineMarginLeft;
int plusMinusHorizontalLineMarginTop;
int plusMinusHorizontalLineLength;
int plusMinusVerticalLineMarginTop;
int plusMinusVerticalLineLength;
int plusMinusVerticalLineMarginLeft;

int tempAreaLeft; int tempAreaTop; int tempAreaWidth;
int fanAreaLeft; int fanAreaTop; int fanAreaWidth;
int ambientAreaLeft; int ambientAreaTop; int ambientAreaWidth;

#if defined (useStandbyButton) || defined(useShutdownButton)
int shutdownWidthAbsolute;
int shutdownHeightAbsolute;
#endif

void calcDimensionsOfElements(void) {
  // upper left corner is 0,0
  // width and heigth are only valid for landscape (rotation=1) or landscape upside down (rotation=3)
  //                       ILI9341   ST7735
  //                       AZ-Touch
  // tft.width   0 <= x <  320       160
  // tft.height  0 <= y <  240       128

  // ALL VALUES ARE BASED ON A 320x240 DISPLAY and automatically resized to the actual display size via getRelativeX() and getRelativeYI()
  int marginTopAbsolute   = 12;
  int marginLeftAbsolute  = 14;
  // int areaHeightAbsolute  = 64;           // make sure: 4*marginTopAbsolute + 3*areaHeightAbsolute = 240
  int areaHeightAbsolute  = (240 - 4*marginTopAbsolute) / 3;

  int valueUpDownWidthAbsolute  = 80;
  int valueUpDownHeightAbsolute = 55;
  #if defined (useStandbyButton) || defined(useShutdownButton)
      shutdownWidthAbsolute     = 40;
      shutdownHeightAbsolute    = 40;
  #endif
  int valueUpRectTop;
  int valueDownRectTop;
  #if defined (useStandbyButton) || defined(useShutdownButton)
  int shutdownRectTop;
  #endif

  tempAreaLeft     = getRelativeX(marginLeftAbsolute);
  fanAreaLeft      = getRelativeX(marginLeftAbsolute);
  ambientAreaLeft = getRelativeX(marginLeftAbsolute);
  #ifdef useAutomaticTemperatureControl
  tempAreaTop     = getRelativeY(marginTopAbsolute);
  fanAreaTop      = getRelativeY(marginTopAbsolute+areaHeightAbsolute+marginTopAbsolute);
  valueUpRectTop   = fanAreaTop;
  ambientAreaTop = getRelativeY(marginTopAbsolute+areaHeightAbsolute+marginTopAbsolute+areaHeightAbsolute+marginTopAbsolute );
  valueDownRectTop = ambientAreaTop;
    #if defined (useStandbyButton) || defined(useShutdownButton)
    tempAreaWidth     = getRelativeX(320-marginLeftAbsolute - shutdownWidthAbsolute-marginLeftAbsolute);    // screen - marginleft - [Area] - 40 shutdown - marginright
    #else
    tempAreaWidth     = getRelativeX(320-marginLeftAbsolute -    0);                                        // screen - marginleft - [Area]               - marginright
    #endif
    #ifdef useTouch
    fanAreaWidth      = getRelativeX(320-marginLeftAbsolute - valueUpDownWidthAbsolute-marginLeftAbsolute); // screen - marginleft - [Area] - 80 up/down  - marginright
    ambientAreaWidth = getRelativeX(320-marginLeftAbsolute - valueUpDownWidthAbsolute-marginLeftAbsolute);
    #else
    fanAreaWidth      = getRelativeX(320-marginLeftAbsolute -    0);                                        // screen - marginleft - [Area]               - marginright
    ambientAreaWidth = getRelativeX(320-marginLeftAbsolute -    0);
    #endif
    #if defined (useStandbyButton) || defined(useShutdownButton)
    shutdownRectTop   = getRelativeY(marginTopAbsolute);
    #endif
  #else
  fanAreaTop      = getRelativeY(marginTopAbsolute);
  valueUpRectTop    = fanAreaTop;
  ambientAreaTop = getRelativeY(marginTopAbsolute+areaHeightAbsolute+marginTopAbsolute);
  valueDownRectTop  = ambientAreaTop;
    #ifdef useTouch
    fanAreaWidth      = getRelativeX(320-marginLeftAbsolute - valueUpDownWidthAbsolute-marginLeftAbsolute); // screen - marginleft - [Area] - 80 up/down  - marginright
    ambientAreaWidth = getRelativeX(320-marginLeftAbsolute - valueUpDownWidthAbsolute-marginLeftAbsolute);
    #else
    fanAreaWidth      = getRelativeX(320-marginLeftAbsolute -    0);                                        // screen - marginleft - [Area]               - marginright
    ambientAreaWidth = getRelativeX(320-marginLeftAbsolute -    0);
    #endif
    #if defined (useStandbyButton) || defined(useShutdownButton)
    shutdownRectTop   = getRelativeY(240-shutdownHeightAbsolute-marginTopAbsolute);
    #endif
  #endif

  valueUpRect[0] = getRelativeX(320-valueUpDownWidthAbsolute-marginLeftAbsolute);
  valueUpRect[1] = valueUpRectTop;
  valueUpRect[2] = getRelativeX(valueUpDownWidthAbsolute);
  valueUpRect[3] = getRelativeY(valueUpDownHeightAbsolute);

  valueDownRect[0] = getRelativeX(320-valueUpDownWidthAbsolute-marginLeftAbsolute);
  valueDownRect[1] = valueDownRectTop;
  valueDownRect[2] = getRelativeX(valueUpDownWidthAbsolute);
  valueDownRect[3] = getRelativeY(valueUpDownHeightAbsolute);

  plusMinusHorizontalLineLength     = (valueUpRect[2] / 2) - 4; // 36
  plusMinusHorizontalLineMarginLeft = (valueUpRect[2] - plusMinusHorizontalLineLength) / 2; // 22
  plusMinusHorizontalLineMarginTop  = valueUpRect[3] / 2; // 27

  plusMinusVerticalLineLength       = plusMinusHorizontalLineLength; // 36
  plusMinusVerticalLineMarginTop    = (valueUpRect[3] - plusMinusVerticalLineLength) / 2; // 9
  plusMinusVerticalLineMarginLeft   = valueUpRect[2] / 2; // 40

  #if defined (useStandbyButton) || defined(useShutdownButton)
  shutdownRect[0] = getRelativeX(320-shutdownWidthAbsolute-marginLeftAbsolute);
  shutdownRect[1] = shutdownRectTop;
  shutdownRect[2] = getRelativeX(shutdownWidthAbsolute);
  shutdownRect[3] = getRelativeY(shutdownHeightAbsolute);

  confirmShutdownYesRect[0]  = getRelativeX(40);
  confirmShutdownYesRect[1]  = getRelativeY(90);
  confirmShutdownYesRect[2]  = getRelativeX(60);
  confirmShutdownYesRect[3]  = getRelativeY(60);
  confirmShutdownNoRect[0]   = getRelativeX(200);
  confirmShutdownNoRect[1]   = getRelativeY(90);
  confirmShutdownNoRect[2]   = getRelativeX(60);
  confirmShutdownNoRect[3]   = getRelativeY(60);
  #endif
}

int16_t tft_getWidth(void) {
  return tft.width();
}


int16_t tft_getHeight(void) {
  return tft.height();
}

void tft_fillScreen(void) {
  tft.fillScreen(TFT_BLACK);
};

boolean GetScreenStatus(void) {
  return screenUpdateRequired;
}
void SetScreenStatus(boolean Status) {
  screenUpdateRequired = Status;
}
/*
https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
https://www.heise.de/ratgeber/Adafruit-GFX-Library-Einfache-Grafiken-mit-ESP32-und-Display-erzeugen-7546653.html?seite=all
https://www.heise.de/select/make/2023/2/2304608284785808657
                        AZ-Touch
              ST7735    ILI9341
  TextSize    Standard  FreeSans9pt7b FreeSerif9pt7b  FreeMono9pt7b  FreeSerifBoldItalic24pt7b
              y1/h
  1 pwm hum   0/8       -12/17        -11/16          -9/13          -30/40
  2 temp      0/16      -24/34        -22/32          -18/26         -60/80
  3           0/24      -36/51        -33/48          -27/39         -90/120
  4 countdown 0/32      -48/68        -44/64          -36/52         -120/160
  8           0/64      -96/136       -88/128         -72/104        -240/320
  15          0/120     -180/255      -165240         -135/195       -450/600
*/
void printText(int areaX, int areaY, int areaWidth, int lineNr, const char *str, uint8_t textSize, const GFXfont *f, bool wipe) {
  
  #ifdef useTouch
 
  #endif

  // Version 2: flicker-free, but slower. Touch becomes unusable.
  #ifndef useTouch

  #endif
}
#endif

void switchOff_screen(boolean switchOff) {
  #ifdef useTFT
 
  #endif
}

void draw_screen(void) {
screenUpdateRequired = true;
/*
    // Calculate the average RPM based on the buffer
  int average_rpm = calculateAverageRPM();
  
 Log.printf("Average fan rpm = %d\r\n", average_rpm);

  char buffer[100];
  spr.createSprite(160,80);
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_WHITE,TFT_BLACK);
  spr.setFreeFont(FF6);
    #ifdef useAutomaticTemperatureControl
     spr.drawString(String(getActualTemperature()),25,6);
      spr.drawString(String(getTargetTemperature()),25,26);

int vad = getPWMvalue();
int Value2 = vad;

if(vad>=1){
spr.drawString(String(average_rpm),25,60);
}
else{
  spr.drawString(String(0),25,60);
}

spr.pushSprite(0,0);
    #endif
  */
}

void OTA_TFT_RESET(void){
  TFTOTA=false;
}
void OTA_Guage_test(void){

  spr.createSprite(160,80);
  spr.fillSprite(TFT_RED);
  spr.setTextColor(TFT_WHITE,TFT_RED);
 
  //spr.setFreeFont(FM9);
  spr.setFreeFont(FF6);


 spr.setTextColor(TFT_WHITE,TFT_RED);
 spr.setTextDatum(MC_DATUM);
 spr.setFreeFont(FF21);
 String Stemp = "OTA STARTED";
 spr.drawString(String(Stemp ),80,40);
 spr.pushSprite(0,0);

TFTOTA=true; // set to True so main screen will not update;
}




void Guage_test(void){
if (TFTOTA) return;

  spr.createSprite(160,80);
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_WHITE,TFT_BLACK);
 
  //spr.setFreeFont(FM9);
  spr.setFreeFont(FF6);

 int vad = getPWMvalue();

 if(vad>0){
   fg_color = 0x5E9E;//TFT_CYAN;
 }

 else{
 fg_color  = 0x08a4;//TFT_DARKGREY;
 }

  //bool smooth = random(2); // true = smooth sides, false = no smooth sides
  end_angle = map(vad,0,255,30,330);


  spr.drawArc(x, y, radius, inner_radius, start_angle, end_angle, fg_color, bg_color);
  spr.drawArc(x, y, radius, inner_radius, end_angle, end_angle_Blank, ng_color, bg_color);
  //int Xs = spr.GET
/*
  int index = (waterTemp > 10.00) ? 153 : 138;
  spr.setCursor(index, 64);
*/


    spr.setTextColor(0x5E9E,TFT_BLACK);
 
  spr.setTextDatum(MC_DATUM);
  spr.drawString(String(vad),x,y);

 spr.setFreeFont(FF21);

String Stemp = "ST:" + String(getTargetTemperature());
spr.drawString(String(Stemp ),39,5);

String temp = "AT:" + String(getActualTemperature());
spr.drawString(String(temp),39,30);
  



int fsa = getPWMvalue();
//int Value2s = fsa;
String rpm = "   ";
if(fsa>=1){
    // Calculate the average RPM based on the buffer
  int average_rpm = calculateAverageRPM();
rpm = "RPM:" + String(average_rpm);
spr.drawString(String(rpm),41,55);
}
else{

 rpm = "RPM:" + String(0);
spr.drawString(String(rpm),35,55);
}




spr.pushSprite(0,0);
}