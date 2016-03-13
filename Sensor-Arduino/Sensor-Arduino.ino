#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_HTU21DF.h>

// pinning
#define inputCurrentPin A0
#define flowCurrrentPin A1
#define busVoltagePin A2

#define flowSensorInterrupt 0  // 0 = digital pin 2
#define flowSensorPin 2

#define dataLinkRX 3
#define dataLinkTX 4

#define transmissionInterval 300

const byte numberOfSensors = 13, bytesPerSensor = 4;

byte * data, * sensorValue;

int dataLength;

unsigned long lastTransmission = 0;

SoftwareSerial dataLink(dataLinkRX, dataLinkTX); // RX, TX

// sensors
Adafruit_MCP9808 tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor2 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor3 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor4 = Adafruit_MCP9808();

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

// flow sensor
// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;
volatile byte pulseCount;  
float flowRate, flowSensorOutput;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTimeFlowSensor;
// end flow sensor

void setup() {
  Serial.begin(9600);
  dataLink.begin(57600);
  // baud rates 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 31250, 38400, 57600, 115200
    
  dataLength = numberOfSensors * bytesPerSensor;
  data = (byte *) malloc(sizeof(byte) * dataLength);
  sensorValue = (byte *) malloc(sizeof(byte) * bytesPerSensor);
  
  for (int i = 0; i < dataLength; i++) {
    data[i] = 0;
  }
  
  
  // temp sensors
  tempsensor1.begin(0x18);
  tempsensor2.begin(0x19);
  tempsensor3.begin(0x1A);
  tempsensor4.begin(0x1B);
  
  htu.begin();
  
  // flow sensor
  pinMode(flowSensorPin, INPUT);
  digitalWrite(flowSensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTimeFlowSensor = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(flowSensorInterrupt, pulseCounter, FALLING);
}

void loop() {
  readSensorData();
  if ((millis() - lastTransmission) > transmissionInterval) {
    lastTransmission = millis();
    dataLink.write(data, dataLength);
    Serial.println("data sent");
  }
  else {
    Serial.println(lastTransmission);
  }
  
  if((millis() - oldTimeFlowSensor) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(flowSensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTimeFlowSensor)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTimeFlowSensor = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    
    // sensor value variable
    flowSensorOutput = (flowRate - int(flowRate)) * 10;

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(flowSensorInterrupt, pulseCounter, FALLING);
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
        // Current Phase 1
        sensorValue = (float)analogRead(inputCurrentPin);
        break;
      case 1:
        // Current Phase 2
        sensorValue = (float)analogRead(inputCurrentPin + 1);
        break;
      case 2:
        // Current Phase 3
        sensorValue = (float)analogRead(inputCurrentPin + 2);
        break;
      case 3:
        // BUS Spannung
        sensorValue = (float)analogRead(busVoltagePin);
        break;
      case 4:
        // Current Primary
        sensorValue = (float)analogRead(flowCurrrentPin);
        break;
      case 5:
        // Temp PFC
        sensorValue = tempsensor2.readTempC();
        break;
      case 6:
        // Temp Bridge
        sensorValue = tempsensor1.readTempC();
        break;
      case 7:
        // Temp MMC
        sensorValue = tempsensor3.readTempC();
        break;
      case 8:
        // Temp Water
        sensorValue = tempsensor4.readTempC();
        break;
      case 9:
        // Water Flow
        sensorValue = flowSensorOutput;
        break;
      case 10:
        // HTU21D-F
        sensorValue = htu.readTemperature();
        break;
      case 11:
        // HTU21D-F
        sensorValue = htu.readHumidity();
        break;
      default: 
        sensorValue = -1.0f;
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

/*
Interrupt Service Routine Flow Sensor
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
