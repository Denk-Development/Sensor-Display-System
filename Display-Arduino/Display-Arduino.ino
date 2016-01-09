#include <SoftwareSerial.h>

// display
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// display
#define TFT_DC 9
#define TFT_CS 10
#define lineHeightPxl 16

#define dataLinkRX 3
#define dataLinkTX 4 
#define numberOfSensors 9
#define bytesPerSensor 4

const unsigned int millisUntilJamDetection = 50;

unsigned long lastPossibleJam = 0;
boolean jamTimerSet = false;

byte * data;
float * oldSensorValues = (float *) calloc(sizeof(float), numberOfSensors);

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
  tft.println("  Temp Bruecke:   0.00");
  tft.println("  Temp PFC:       0.00");
  tft.println("  Temp MMC:       0.00");
  tft.println("  Temp Wasser:    0.00");
  tft.println("  Input Strom:    0.00");
  tft.println("  Primaerstrom:   0.00");
  tft.println("  BUS Spannung:   0.00");
  tft.println("  FlowSensor:     0.00");
  tft.println("  HTU21D-F:       0.00");
  
  dataLength = numberOfSensors * bytesPerSensor;
  data = (byte *) malloc(sizeof(byte) * dataLength);
  
  dataLink.begin(57600); 
  // baud rates 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 31250, 38400, 57600, 115200
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
      
      // write only to the screen when values have changed
      if (oldSensorValues[i] != val) {
        int iTimesLineHeight = i * lineHeightPxl;
        tft.setCursor(192, iTimesLineHeight + 1);
        String s = floatToDisplayString(val);
        
        // handle as many tasks as possible before clearing the row
        //           x    y                 width height         color
        //           190                    to 130
        tft.fillRect(192, iTimesLineHeight, 78,   lineHeightPxl, ILI9341_BLACK); // clear row
        tft.println(s); // write new value
        
        oldSensorValues[i] = val;
      }
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
