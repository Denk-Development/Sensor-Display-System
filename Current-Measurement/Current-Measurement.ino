/******************************************************
Precise AC Current Measurement with Arduino & ACS712
tested with Arduinos based on ATmega 328P

Stefan Thesen, 09/2015
Edited Version to support A1 and A2 as well (March 2016)

This code uses direct programming of ADC registers
to perform high speed sampling of the ACS712 analog
signal connected to A0 pin. 
The code samples >160 times per AC period (@50Hz)
and calculated the effective current/apparent power.
This sampling is run for 100ms so that we effectively
improve the ADC resolution beyond 10Bit by 
oversampling.
For oversampling background see:
http://www.atmel.com/Images/doc8003.pdf
Exact accuracy to be determined - should be better
than 1 percent.

Code is set-up for the 20A ACS712 version.
                             
Copyright: public domain -> do what you want  
For details visit http://blog.thesen.eu 

******************************************************/

float gfLineVoltage = 230.0f;               // Spannung in V
float gfACS712_Factor = 60.0f;              // 50.0f für 20A Sensor, 75.76f für 30A Sensor; 27.03f für 5A Sensor
unsigned long gulSamplePeriod_us = 20000;   // 50ms is 10 cycles at 50Hz and 12 cycles at 60Hz
int giADCOffset = 512;                      // offset für "0" Sensor WSC1800

void setup()
{
 Serial.begin(9600);
 
}

void loop()
{
  Serial.println("A0: " + String(measureCurrent(A0)));
  Serial.println("A1: " + String(measureCurrent(A1)));
  Serial.println("A2: " + String(measureCurrent(A2)));
  delay(1000);
}


float measureCurrent(byte pin) {
 long lNoSamples=0;
 long lCurrentSumSQ = 0;
 long lCurrentSum=0;

  // set-up ADC
  ADCSRA = 0x87;  // turn on adc, adc-freq = 1/128 of CPU ; keep in min: adc converseion takes 13 ADC clock cycles
  ADMUX = 0x40;   // interne 5V reference

  // choose pin
  switch(pin) {
    case A0:
      break;
    case A1:
      ADMUX |= 0B1;
      break;
    case A2: 
      AXMUX |= 0B10;
      break;
  }
    
  
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
    lValue -= giADCOffset;

    lCurrentSum += lValue;
    lCurrentSumSQ += lValue*lValue;
    lNoSamples++;
  }
  
  // stop sampling
  ADCSRA = 0x00;

  if (lNoSamples>0)  // if no samples, micros did run over
  {  
    // correct quadradic current sum for offset: Sum((i(t)+o)^2) = Sum(i(t)^2) + 2*o*Sum(i(t)) + o^2*NoSamples
    // sum should be zero as we have sampled 5 cycles at 50Hz (or 6 at 60Hz)
    float fOffset = (float)lCurrentSum/lNoSamples;
    lCurrentSumSQ -= 2*fOffset*lCurrentSum + fOffset*fOffset*lNoSamples;
    if (lCurrentSumSQ<0) {lCurrentSumSQ=0;} // avoid NaN due to round-off effects
    
    float fCurrentRMS = sqrtf((float)lCurrentSumSQ/(float)lNoSamples) * gfACS712_Factor / 1024;
  
    // correct offset for next round
    giADCOffset=(int)(giADCOffset+fOffset+0.5f);
    
    return fCurrentRMS;
  }
  return 0;
}
