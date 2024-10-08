#include <Arduino.h>
#include <esp32-hal.h>
#include <pins_arduino.h>
#include "config.h"
#include "log.h"

static volatile int counter_rpm = 0;

unsigned long millisecondsLastTachoMeasurement = 0;

// Interrupt counting every rotation of the fan
// https://desire.giesecke.tk/index.php/2018/01/30/change-global-variables-from-isr/
void IRAM_ATTR rpm_fan() {
  counter_rpm++;
}

int last_rpm =0;
void initTacho(void) {
  pinMode(39, INPUT_PULLUP);
  digitalWrite(TACHOPIN, HIGH);
  attachInterrupt(digitalPinToInterrupt(39), rpm_fan, FALLING);
  Log.printf("  Fan tacho detection sucessfully initialized.\r\n");
}

void updateTacho(void) {
  // start of tacho measurement
  if ((unsigned long)(millis() - millisecondsLastTachoMeasurement) >= TACHOUPDATECYCLE)
  { 
    // detach interrupt while calculating rpm
    detachInterrupt(digitalPinToInterrupt(39)); 
    // calculate rpm
    last_rpm = counter_rpm * ((float)60 / (float)NUMBEROFINTERRUPSINONESINGLEROTATION) * ((float)1000 / (float)TACHOUPDATECYCLE);
    Log.printf("fan rpm = %d\r\n", last_rpm);

    // reset counter
    counter_rpm = 0; 
    // store milliseconds when tacho was measured the last time
    millisecondsLastTachoMeasurement = millis();

    // attach interrupt again
    attachInterrupt(digitalPinToInterrupt(39), rpm_fan,FALLING);
  }
}
#define BUFFER_SIZE 20
int rpm_buffer[BUFFER_SIZE]; // buffer to store the last 10 rpm values
int buffer_index = 0; // index for the current position in the buffer

// Function to calculate the average of the RPM buffer
int calculateAverageRPM() {
  int sum = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    sum += rpm_buffer[i];
  }
  return sum / BUFFER_SIZE;
}

float alpha = 0.1;  // Smoothing factor for the exponential moving average
float filtered_rpm = 0;  // This will hold the filtered RPM value

void updateTacho_NEW(void) {
  // start of tacho measurement
  if ((unsigned long)(millis() - millisecondsLastTachoMeasurement) >= TACHOUPDATECYCLE) { 
    // detach interrupt while calculating rpm
    detachInterrupt(digitalPinToInterrupt(39)); 
    
    // calculate rpm
    last_rpm = counter_rpm * ((float)60 / (float)NUMBEROFINTERRUPSINONESINGLEROTATION) * ((float)1000 / (float)TACHOUPDATECYCLE);
    
    // Apply exponential moving average filter
    filtered_rpm = (alpha * last_rpm) + ((1 - alpha) * filtered_rpm);
    
    // Print filtered RPM
   // Log.printf("Filtered fan rpm = %d\r\n", (int)filtered_rpm);

    // store the filtered rpm in the ring buffer
    rpm_buffer[buffer_index] = (int)filtered_rpm;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;  // increment index and wrap around
    
    // reset counter
    counter_rpm = 0; 
    // store milliseconds when tacho was measured the last time
    millisecondsLastTachoMeasurement = millis();

    // attach interrupt again
    attachInterrupt(digitalPinToInterrupt(39), rpm_fan, FALLING);
  }
}
