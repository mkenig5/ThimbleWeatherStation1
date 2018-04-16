/*
   Enhanced Thimble Weather Station 1
   Submitted by Mike Kenig

   https://learning.thimble.io/modules/weather-station-1

   Seeduino connections:
     D4  Button
     D8  LED Bar Graph
     I2C LCD
     I2C BME280
     I2C Sunshine Sensor

   Button actions:
       Single click advances through the three screens
       ** Temperature & humidity
       ** UV Index & Pressure
       ** Visible Light & Infrared Light
       Double click returns to initial screen (temp & humidity)
       Click & hold cycles through all the screens until you release the button

   LED Bar Graph
     Typical inside temps of 55-85F get mapped to the 0-10 bar scale.

*/

//Libraries
#include <Arduino.h>        //Basic Arduino library
#include <Wire.h>           //I2C communication
#include <Seeed_BME280.h>   //BME280 (temperature, humidity, and pressure sensor)
#include <math.h>           //BME280 uses pow function
#include <SI114X.h>         //Sunshine sensor
#include <rgb_lcd.h>        //LCD
#include <OneButton.h>      //Button
#include <Grove_LED_Bar.h>  //LED bar graph

//Defines
#define TOP 0                //Top LCD line
#define BOTTOM 1             //Bottom LCD line
#define BUTTON_PIN 4         //Button on pin D4
#define LED_BAR_CLOCK_PIN 9  //Clock pin for LED bar graph
#define LED_BAR_DATA_PIN 8   //Data pin for LED bar graph

//Declare BME280, sunshine sensor, LCD, button, LED bar graph
BME280 bme280;
//SI114X sunlightSensor = SI114X();  //not sure why this default constructor is here
SI114X sunlightSensor;
rgb_lcd lcd;
OneButton button(BUTTON_PIN, false);
Grove_LED_Bar ledBar(LED_BAR_CLOCK_PIN, LED_BAR_DATA_PIN, true);

//Possible screens
enum screens {
  TEMPHUMIDITY,
  UVPRESSURE,
  VISIBLEIR
};

//Initial screen state (temperature and humidity)
screens screenState = TEMPHUMIDITY;

//LCD display color
const int colorR = 50;
const int colorG = 50;
const int colorB = 25;

//Other global variables
float Dtemp;  //temp in Fahrenheit

//setup - run once
void setup() {
  //Initialize LCD and set color
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);

  //Begin serial for any errors
  Serial.begin(9600);
  while (!sunlightSensor.Begin()) {
    Serial.println(F("Sunlight Error"));
    delay(10);
  }
  if (!bme280.init()) {
    Serial.println(F("BME Error"));
  }

  //Initialize button click and point to callback functions
  button.attachClick(buttonClick);
  button.attachDoubleClick(buttonDoubleClick);
  button.attachLongPressStart(buttonPressStart);
  button.attachDuringLongPress(buttonPressDuring);
  button.attachLongPressStop(buttonPressStop);

  //Initialize LED bar graph
  ledBar.begin();
}

//loop - run continuously
void loop() {
  //Refresh temperature reading
  Dtemp = getTempF();

  //Check button - fires attachClick if single click detected
  button.tick();

  //Screen state - cycle through screens based on button click
  updateScreen();

  //Update LED bar graph
  ledBar.setLevel(map(Dtemp, 55, 85, 0, 10));  //using inside temp range of 55-85F

  //Small delay because we don't really need to refresh continuously
  delay(50);
}

//Float override of built-in map function because LED bar graph can display fractional bars
float map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Update screen based on screen state
void updateScreen() {
  switch (screenState)
  {
    case TEMPHUMIDITY:
      displayTemp(TOP);
      displayHumidity(BOTTOM);
      break;

    case UVPRESSURE:
      displayUV(TOP);
      displayPressure(BOTTOM);
      break;

    case VISIBLEIR:
      displayVisible(TOP);
      displayIR(BOTTOM);
      break;
  }
}

//Invoked from button.attachClick
//Advances screen state once so user can cycle through manually
void buttonClick() {
  switch (screenState)
  {
    case TEMPHUMIDITY:
      screenState = UVPRESSURE;
      break;

    case UVPRESSURE:
      screenState = VISIBLEIR;
      break;

    case VISIBLEIR:
      screenState = TEMPHUMIDITY;
      break;
  }
}

//Invoked from button.attachDoubleClick
//Resets screen to initial screen (TEMPHUMIDITY)
void buttonDoubleClick() {
  screenState = TEMPHUMIDITY;
  updateScreen();
}

//Invoked from button.attachLongPressStart
//Resets screen to initial screen (TEMPHUMIDITY)
void buttonPressStart() {
  buttonDoubleClick();  //starting the button press just goes to start like double click
}

//Invoked from button.attachDuringLongPress
//Cycles through screens so user can simply press & hold the button to see all readings
void buttonPressDuring() {
  buttonClick();
  delay(750);
}

//Invoked from button.attachLongPressStop
//Resets screen to initial screen (TEMPHUMIDITY)
void buttonPressStop() {
  buttonDoubleClick();  //ending the button press just goes to start like double click
}

//Read temperature sensor and convert to Fahrenheit (F)
float getTempF() {
  return (bme280.getTemperature() * 1.8 + 32);
}

//Display the temperature in Fahrenheit (F) on the specified LCD line
//Assumes Dtemp, refreshed in loop
void displayTemp(int line) {
  printLabel("Temp = ", line);
  printMeasurement(7, (String) Dtemp, line);
  printUnit(13, "F", line);
}

//Display the humidity percentage on the specified LCD line
void displayHumidity(int line) {
  printLabel("Humidity = ", line);
  printMeasurement(11, (String) bme280.getHumidity(), line);
  printUnit(14, "%", line);
}

//Display the UV index on the specified LCD line
void displayUV(int line) {
  printLabel("UV Index = ", line);
  printMeasurement(11, (String) ((float)sunlightSensor.ReadUV() / 100), line);
  //no unit to print
}

//Display the pressure in Pascals (Pa) on the specified LCD line
void displayPressure(int line) {
  printLabel("Press = ", line);
  printMeasurement(8, (String) bme280.getPressure(), line);
  printUnit(14, "Pa", line);
}

//Display the visible/ambient light in lumens (lm) on the specified LCD line
void displayVisible(int line) {
  printLabel("Visible = ", line);
  printMeasurement(10, (String) sunlightSensor.ReadVisible(), line);
  printUnit(14, "lm", line);
}

//Display the infrared light in lumens (lm) on the specified LCD line
void displayIR(int line) {
  printLabel("IR = ", line);
  printMeasurement(5, (String) sunlightSensor.ReadIR(), line);
  printUnit(9, "lm", line);
}

//Print a measurement label to the specified line
//Side effect: clears the specified line before printing the label
void printLabel(const char label[], int line) {
  clearLine(line);
  lcd.setCursor(0, line);
  lcd.print(label);
}

//Print a measurement number to the specified line
void printMeasurement(int lcdColumn, String measurement, int line) {
  lcd.setCursor(lcdColumn, line);
  lcd.print(measurement);
}

//Print a measurement unit to the specified line
void printUnit(int lcdColumn, const char unit[], int line) {
  lcd.setCursor(lcdColumn, line);
  lcd.print(unit);
}

//Clear the specified LCD line
void clearLine(int lineToClear) {
  lcd.setCursor(0, lineToClear);
  lcd.print(F("                "));
}
