# Sensor-Display-System
A Teensy (https://www.pjrc.com/teensy/) based system for a laser show, transmitting and displaying sensor data.

In a nutshell the whole project is about showing and logging transmitted sensor data of laser show hardware. This includes among others currents, voltages, temperatures, water flow and humidity data.

For performance reasons the system uses two Teensy 3.2 micrcontrollers (72 MHz Cortex-M4) instead of the Arduino's ATmega 328. 

This is a private project.
I've developed it for Lucien Tunger (http://www.lsd-lasershowdesign.de/).

## Testing setup
The picture shows both, Sensor- and Display-Teensy. The Sensor-Teensy isn't equipped with any peripheral devices whereas the Display-Teensy is wired up with an LCD display, a real time clock module and an SD card breakout.
![Testing setup image](https://raw.githubusercontent.com/Simsso/Sensor-Display-System/master/Photos/TestingSetup.jpg)
