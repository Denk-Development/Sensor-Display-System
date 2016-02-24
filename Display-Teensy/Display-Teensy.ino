#include <SPI.h>

// sd card / logging
#include <SD.h>
#include <Time.h>  

// display
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define dataLink Serial1 // RX: 0; TX: 1

// display
#define TFT_DC 9
#define TFT_CS 10
#define lineHeightPxl 16

// sd card breakout
#define SD_CS 5

#define BUFFPIXEL 20

#define numberOfSensors 10
#define bytesPerSensor 4

File logFile;
char logFileName[12]; // log files have a fixed file name length of 10+1 chars ("MMDDHH.LOG")

const unsigned int millisUntilJamDetection = 50;

unsigned long lastPossibleJam = 0;
boolean jamTimerSet = false;

byte * data;
float * oldSensorValues = (float *) calloc(sizeof(float), numberOfSensors);

int dataLength, bytesAvailable, bytesAvailableAfterDelay;

long numberOfBytesAvailableLastTime = 0;

// display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  Serial.begin(9600);
  
  // start tft
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  
  // start sd card
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS, SPI_QUARTER_SPEED)) {
    Serial.println("SD card failed");
  }
  Serial.println("SD card ok");

  // draw logo
  bmpDraw("logo.bmp", 0, 0);
  
  Serial.println("showing logo");
  
  delay(5000); // show logo
  
  
  // initialize time module 
  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);
  delay(100);
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }
  
  
  Serial.println("showing sensor data");
  tft.setTextSize(2);
  tft.fillScreen(ILI9341_BLACK);

  // print sensor names
  tft.println("  Temp Bruecke:   0.00");
  tft.println("  Temp PFC:       0.00");
  tft.println("  Temp MMC:       0.00");
  tft.println("  Temp Wasser:    0.00");
  tft.println("  Input Strom:    0.00");
  tft.println("  Primaerstrom:   0.00");
  tft.println("  BUS Spannung:   0.00");
  tft.println("  FlowSensor:     0.00");
  tft.println("  HTU21D-F Temp:  0.00");
  tft.println("  HTU21D-F Hum:   0.00");
  
  dataLength = numberOfSensors * bytesPerSensor;
  data = (byte *) malloc(sizeof(byte) * dataLength);
  
  // create log file
  String tmp = getLogFileName();
  logFileName[tmp.length()+1];
  tmp.toCharArray(logFileName, sizeof(logFileName));
  Serial.println(tmp.length());
  Serial.println(tmp);
  Serial.println(logFileName);
  logFile = SD.open(logFileName, FILE_WRITE);
  logFile.close();
  
  // check if the log file has been created successfully
  if (SD.exists(logFileName)) {
    Serial.println("log file created");
  } else {
    Serial.println("couldn't create log file");
  }
  
  
  // serial connection to sensor arduino
  dataLink.begin(57600); 
  // baud rates 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 31250, 38400, 57600, 115200
}

void loop() {
  // time module
  /*time_t t = processSyncMessage();
  if (t != 0) {
    Teensy3Clock.set(t); // set the RTC
    setTime(t);
  }
    
  if (jamTimerSet && lastPossibleJam < millis() - millisUntilJamDetection) {
    jamTimerSet = false;
    flushDataLinkBuffer();
    Serial.println("flushed");
  }
  
  
  // check if data is currently incoming
  if (numberOfBytesAvailableLastTime != dataLink.available()) {
    jamTimerSet = false;
    Serial.println("jam timer set");
  }
  
  if (dataLink.available() > 0 && dataLink.available() % dataLength == 0) {
    jamTimerSet = false;
    
    Serial.println("data incoming");
    
    dataLink.readBytes((char *)data, dataLength);
    
    // open log file
    logFile = SD.open(logFileName, FILE_WRITE);
    logFile.print(getTimeString()); // log time
    
    for (int i = 0;  i < numberOfSensors; i++) {
      float * sensorValue = (float *) & data[i * bytesPerSensor];
      float val = * sensorValue;
      
      logFile.print(",");
      logFile.print(val);
      
      Serial.println("Sensor " + String(i) + ": " + String(val));
      
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
    
    // log file end line
    logFile.println();
    logFile.close(); // save changes
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


// time module code

String getTimeString() {
  return addLeadingZeros(year(), 4) + "." + addLeadingZeros(month(), 2) + "." + addLeadingZeros(day(), 2) + " " + addLeadingZeros(hour(), 2) + ":" + addLeadingZeros(minute(), 2) + ":" + addLeadingZeros(second(), 2);
}

String getLogFileName() {
  return addLeadingZeros(month(), 2) + addLeadingZeros(day(), 2) + addLeadingZeros(hour(), 2) + ".LOG";  
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
  }
  return pctime;
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

String addLeadingZeros(int x, int digits) {
  String str = String(x);
  while (str.length() < digits) {
    str = '0' + str;
  }
  return str;
}



// draw bitmap code


// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("file not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
