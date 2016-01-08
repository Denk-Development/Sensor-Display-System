#include <SoftwareSerial.h>

// display
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// display
#define TFT_DC 9
#define TFT_CS 10
#define lineHeightPxl 20

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
    
    // reset screen
    //tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    
    for (int i = 0;  i < numberOfSensors; i++) {
      float * sensorValue = (float *) & data[i * bytesPerSensor];
      
      float val = * sensorValue;
      
      switch (i) {
        case 0: // Temp Brücke
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  Temp Bruecke:  " + floatToDisplayString(val));
          break;
        case 1: // Temp PFC
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  Temp PFC:      " + floatToDisplayString(val));
          break;
        case 2: // Temp MMC
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  Temp MMC:      " + floatToDisplayString(val));
          break;
        case 3: // Temp Wasser
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  Temp Wasser:   " + floatToDisplayString(val));
          break;
        case 4: // Eingangsstrom
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  Input Strom:   " + floatToDisplayString(val));
          break;
        case 5: // Primärstrom
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  Primaerstrom:  " + floatToDisplayString(val));
          break;
        case 6: // BUS Spannung
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  BUS Spannung:  " + floatToDisplayString(val));
          break;
        case 7: // FlowSensor
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  FlowSensor:    " + floatToDisplayString(val));
          break;
        case 8: // HTU21D-F
          clearRow(i * lineHeightPxl, lineHeightPxl);
          tft.println("  HTU21D-F:      " + floatToDisplayString(val));
          break;
        default:
          break;
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

void clearRow(int y, int height) {
  for (byte i = height - 1; i != 0; i--) {
    tft.drawFastHLine(200, y + i, 120, ILI9341_BLACK);
  }
}
