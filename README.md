## Goto: Test branch

# Project
Description: 
This project consists of 4 components - Android Remote Controller, Algorithms, RPi Comms + Image Processing and Robot Hardware. Current git repo only consists of the Robot Hardware component.

## Hardware used:
1. STM32F407VET6
2. Hall Encoder Motors MG513P3012V
3. Sharp GP2Y0A21YK IR Sensor
4. Ultrasonic Sensor HCSR04SEN0001

## Software and external libraries used:
1. STM32CubeIDE
2. PeripheralDrivers

## Objective of Robot Hardware:
Using the STM32 microcontroller to create serial communication with the RPi, break down robot movement into modular commands, ensure robot maintains the correct distance, straightness and speed, and user the various sensors to avoid object collision.
