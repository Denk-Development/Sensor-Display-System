#include <SoftwareSerial.h>

// display
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// display
#define TFT_DC 9
#define TFT_CS 10
#define lineHeightPxl 16

#define dataLinkRX 2
#define dataLinkTX 3 
#define numberOfSensors 9
#define bytesPerSensor 4

const unsigned int millisUntilJamDetection = 50;

unsigned long lastPossibleJam = 0;
boolean jamTimerSet = false;

byte * data;

int dataLength, bytesAvailable, bytesAvailableAfterDelay;

long numberOfBytesAvailableLastTime = 0;

SoftwareSerial dataLink(dataLinkRX, dataLinkTX); // RX, TX

// display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  Serial.begin(9600);
  
  // start tft
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);

  // print sensor names
  tft.println("  Temp Bruecke:  ");
  tft.println("  Temp PFC:      ");
  tft.println("  Temp MMC:      ");
  tft.println("  Temp Wasser:   ");
  tft.println("  Input Strom:   ");
  tft.println("  Primaerstrom:  ");
  tft.println("  BUS Spannung:  ");
  tft.println("  FlowSensor:    ");
  tft.println("  HTU21D-F:      ");
  
  dataLink.begin(1200);
  
  dataLength = numberOfSensors * bytesPerSensor;
  data = (byte *) malloc(sizeof(byte) * dataLength);
}

void loop() {
  if (jamTimerSet && lastPossibleJam < millis() - millisUntilJamDetection) {
    jamTimerSet = false;
    flushDataLinkBuffer();
  }
  
  
  // check if data is currently incoming
  if (numberOfBytesAvailableLastTime != dataLink.available()) {
    jamTimerSet = false;
  }
  
  if (dataLink.available() > 0 && dataLink.available() % dataLength == 0) {
    jamTimerSet = false;
    
    dataLink.readBytes((char *)data, dataLength);
    for (int i = 0;  i < numberOfSensors; i++) {
      float * sensorValue = (float *) & data[i * bytesPerSensor];
      float val = * sensorValue;
      
      int iTimesLineHeight = i * lineHeightPxl;
      tft.setCursor(190, iTimesLineHeight + 1);
      String s = floatToDisplayString(val);
      
      // handle as many tasks as possible before clearing the row
      //           x    y                 width height         color
      //           190                    to 130
      tft.fillRect(190, iTimesLineHeight, 70,   lineHeightPxl, ILI9341_BLACK); // clear row
      tft.println(s); // write new value
    }
  }
  
  // possibly invalid data in the buffer
  else if (!jamTimerSet && dataLink.available() != 0) {
    lastPossibleJam = millis();
    jamTimerSet = true;
  }
}

void flushDataLinkBuffer() {
  Serial.println("flushed");
  while (dataLink.available()) {
    dataLink.read();
  }
}

String floatToDisplayString(float f) {
  String s = String(f);
  // add leading spaces
  while (s.length() < 6) {
    s = " " + s;
  }
  return s; 
}
