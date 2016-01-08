#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_HTU21DF.h>

// pinning
#define inputCurrentPin A0
#define flowCurrrentPin A1
#define busVoltagePin A2
#define dataLinkRX 2
#define dataLinkTX 3

const byte numberOfSensors = 9, bytesPerSensor = 4;

byte * data, * sensorValue;

int dataLength;

SoftwareSerial dataLink(dataLinkRX, dataLinkTX); // RX, TX

// sensors
/*Adafruit_MCP9808 tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor2 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor3 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor4 = Adafruit_MCP9808();*/

void setup() {
  dataLink.begin(1200);
  
  dataLength = numberOfSensors * bytesPerSensor;
  data = (byte *) malloc(sizeof(byte) * dataLength);
  sensorValue = (byte *) malloc(sizeof(byte) * bytesPerSensor);
  
  for (int i = 0; i < dataLength; i++) {
    data[i] = 0;
  }
  
  
  // temp sensors
  /*tempsensor1.begin(0x18);
  tempsensor2.begin(0x19);
  tempsensor3.begin(0x1A);
  tempsensor4.begin(0x1B);*/
}

void loop() {
  if (readSensorData()) { 
    // data has changed
    dataLink.write(data, dataLength);
    delay(1000); // interval e.g. 1200ms
  }
}

boolean readSensorData() {
  boolean changeDetected = false;
  // iterate through sensors
  for (int sensor = 0; sensor < numberOfSensors; sensor++) {
    // read the actual value
    float sensorValue;
    switch(sensor) {
      case 0:
        // Temp Brücke
        sensorValue = 50;//tempsensor1.readTempC();
        break;
      case 1:
        // Temp PFC
        sensorValue = 20;//tempsensor2.readTempC();
        break;
      case 2:
        // Temp MMC
        sensorValue = 122;//tempsensor3.readTempC();
        break;
      case 3:
        // Temp Wasser
        sensorValue = -1.4;//tempsensor4.readTempC();
        break;
      case 4:
        // Eingangsstrom
        sensorValue = (float)analogRead(inputCurrentPin);
        break;
      case 5:
        // Primärstrom
        sensorValue = (float)analogRead(flowCurrrentPin);
        break;
      case 6:
        // BUS Spannung
        sensorValue = (float)analogRead(busVoltagePin);
        break;
      case 7:
        // FlowSensor
        // TODO: read sensor value
        sensorValue = 0.0f;
        break;
      case 8:
        // HTU21D-F
        // TODO: read sensor value
        sensorValue = 0.0f;
        break;
      default: 
        sensorValue = 0.0f;
        break;
    }
    
    // byte pointer to the sensor value
    byte * sensorValuePointer = (byte *) &sensorValue;
    
    // read all bytes of the sensor value into the data array
    for (int sensorByte = 0; sensorByte < bytesPerSensor; sensorByte++) {
      if (* (data + sensor * bytesPerSensor + sensorByte) != * (sensorValuePointer + sensorByte)) {
        changeDetected = true;
        * (data + sensor * bytesPerSensor + sensorByte) = * (sensorValuePointer + sensorByte);
      }
    }
  }
  
  return changeDetected;
}
