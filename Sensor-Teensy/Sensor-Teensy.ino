#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_HTU21DF.h>

// pinning
#define flowSensorPin 2 // interrupt pin
#define dataLinkRX 0 // constant (Serial1)
#define dataLinkTX 1 // constant (Serial1)

#define currentPhase1Pin A0
#define currentPhase2Pin A1
#define currentPhase3Pin A2
#define flowCurrrentPin A5
#define busVoltagePin A6

#define transmissionInterval 300

#define dataLink Serial1

const byte numberOfSensors = 13, bytesPerSensor = 4;

byte * data, * sensorValue;

int dataLength;

unsigned long lastTransmission = 0;

// sensors
Adafruit_MCP9808 tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor2 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor3 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor4 = Adafruit_MCP9808();

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

// AC current (3 sensors, A0, A1, A2)
float gfLineVoltage = 230.0f; // current in V
float gfACS712_Factor = 60.0f; // 50.0f for 20A sensor, 75.76f for 30A sensor; 27.03f for 5A sensor
unsigned long gulSamplePeriod_us = 20000; // 50ms is 10 cycles at 50Hz and 12 cycles at 60Hz
int giADCOffset[3] = { 512, 512, 512 }; // offset for "0" Sensor WSC1800

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
  
  dataLink.begin(58824);
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
  attachInterrupt(flowSensorPin, pulseCounter, FALLING);
}

void loop() {
  if ((millis() - lastTransmission) > transmissionInterval && readSensorData()) {
    lastTransmission = millis();
    // data has changed
    dataLink.write(data, dataLength);
    Serial.println("data transmitted");
  }
  
  if((millis() - oldTimeFlowSensor) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(flowSensorPin);
        
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
    attachInterrupt(flowSensorPin, pulseCounter, FALLING);
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
        sensorValue = measureCurrent(currentPhase1Pin);
        break;
      case 1:
        // Current Phase 2
        sensorValue = measureCurrent(currentPhase2Pin);
        break;
      case 2:
        // Current Phase 3
        sensorValue = measureCurrent(currentPhase3Pin);
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


// Interrupt Service Routine Flow Sensor
void pulseCounter()
{
  cli();
  // Increment the pulse counter
  pulseCount++;
  sei();
}

float measureCurrent(byte analogPin) { // analogPin [0..7] 
return (float)analogPin;
}
/* long lNoSamples=0;
 long lCurrentSumSQ = 0;
 long lCurrentSum=0;

  // set-up ADC
  ADCSRA = 0x87; // turn on adc, adc-freq = 1/128 of CPU ; keep in min: adc converseion takes 13 ADC clock cycles
  ADMUX = 0x40; // internal 5V reference

  ADMUX |= analogPin; // choose pin
    
  
  // 1st sample is slower due to datasheet - so we spoil it
  ADCSRA |= (1 << ADSC);
  while (!(ADCSRA & 0x10));
  
  // sample loop - with inital parameters, we will get approx 800 values in 100ms
  unsigned long ulEndMicros = micros()+gulSamplePeriod_us;
  while(micros()<ulEndMicros)
  {
    // start sampling and wait for result
    ADCSRA |= (1 << ADSC);
    while (!(ADCSRA & 0x10));
    
    // make sure that we read ADCL 1st
    long lValue = ADCL; 
    lValue += (ADCH << 8);
    lValue -= giADCOffset[analogPin];

    lCurrentSum += lValue;
    lCurrentSumSQ += lValue*lValue;
    lNoSamples++;
  }
  
  // stop sampling
  ADCSRA = 0x00;

  if (lNoSamples>0) // if no samples, micros did run over
  {  
    // correct quadradic current sum for offset: Sum((i(t)+o)^2) = Sum(i(t)^2) + 2*o*Sum(i(t)) + o^2*NoSamples
    // sum should be zero as we have sampled 5 cycles at 50Hz (or 6 at 60Hz)
    float fOffset = (float)lCurrentSum/lNoSamples;
    lCurrentSumSQ -= 2*fOffset*lCurrentSum + fOffset*fOffset*lNoSamples;
    if (lCurrentSumSQ<0) {lCurrentSumSQ=0;} // avoid NaN due to round-off effects
    
    float fCurrentRMS = sqrtf((float)lCurrentSumSQ/(float)lNoSamples) * gfACS712_Factor / 1024;
  
    // correct offset for next round
    giADCOffset[analogPin] = (int)(giADCOffset[analogPin] + fOffset + 0.5f);
    
    return fCurrentRMS;
  }
  return 0;
}*/
