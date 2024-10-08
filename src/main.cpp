#include <Arduino.h>

#include "config.h"
#include "log.h"
#include "wifiCommunication.h"
#include "mqtt.h"
#include "sensorBME280.h"

// Watchdog timer parameters
#define WATCHDOG_TIMEOUT 45000  // Set the watchdog timeout in milliseconds (15 seconds)

// Initialize watchdog timer
hw_timer_t *watchdogTimer = NULL;



bool   Button_SW1 = false;
bool   Button_SW2 = false;

const int Button1 = 26;
const int Button2 = 27;

unsigned long lastDebounceTime1 = 0; // the last time the output pin was toggled
unsigned long lastDebounceTime2 = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 200;    // the debounce time; increase if the output flickers



#include "fanPWM.h"
#include "fanTacho.h"
#include "temperatureController.h"
#include "tft.h"


#if defined(useOTAUpdate)
  // https://github.com/SensorsIot/ESP32-OTA
  #include "OTA.h"
  #if !defined(useOTA_RTOS)
    #include <ArduinoOTA.h>
  #endif
#endif
#if defined(useTelnetStream)
#include "TelnetStream.h"
#endif

unsigned long previousMillis1000Cycle = 0;
unsigned long interval1000Cycle = 1000;
unsigned long previousMillis10000Cycle = 0;
unsigned long interval10000Cycle = 10000;
unsigned long previousMillis5000Cycle = 0;
unsigned long interval5000Cycle = 3000;

// Watchdog timer task
void IRAM_ATTR watchdogTask() {
  // This function will be called when the watchdog timer times out
  Serial.println("Watchdog timer reset");
  ESP.restart();
}

#include <Preferences.h>
Preferences preferences;


void saveTemperature(float temp) {
  preferences.begin("myApp", false);  // Open Preferences in read-write mode
  preferences.putFloat("Set_TEMP", temp);  // Save the new temperature

  preferences.end();  // Close Preferences
   Set_target_temp(temp);
}


void setup(){
  Serial.begin(115200);
  Serial.println("");
  Log.printf("Setting things up ...\r\n");

  // Open Preferences with a namespace and a name
  preferences.begin("myApp", false); // false: read-write access
  
  // Read a float value stored with the key "Set_TEMP", defaulting to 30.00 if not found
  float temperature = preferences.getFloat("Set_TEMP", 30.00);

  Set_target_temp(temperature);

// Optionally, you can write a new value using putFloat
// preferences.putFloat("Set_TEMP", 99.5);

  // Close the Preferences
  preferences.end();




    pinMode(Button1,INPUT);
    pinMode(Button2,INPUT);
  #ifdef useWIFI
  wifi_setup();
  wifi_enable();
  #endif
  #if defined(useOTAUpdate)
  OTA_setup("ESP32fancontroller");
  // Do not start OTA. Save heap space and start it via MQTT only when needed.
  // ArduinoOTA.begin();
  #endif
  #if defined(useTelnetStream)
  TelnetStream.begin();
  #endif
  #ifdef useTFT
  initTFT();
  #endif
  #ifdef useTouch
  initTFTtouch();
  #endif
  initPWMfan();
  initTacho();
  #ifdef useTemperatureSensorBME280
  initBME280();
  #endif
  #ifdef useAutomaticTemperatureControl
  initTemperatureController();
  #endif
  #ifdef useMQTT
  mqtt_setup();
  #endif

  Log.printf("Settings done. Have fun.\r\n");
  // Setup and start the watchdog timer
 watchdogTimer = timerBegin(0, 80, true);
 timerAttachInterrupt(watchdogTimer, &watchdogTask, true);
 timerAlarmWrite(watchdogTimer, WATCHDOG_TIMEOUT * 1000, false);
 timerAlarmEnable(watchdogTimer);
 
}
int average_rpm = 0;
void loop(){
  // functions that shall be called as often as possible
  // these functions should take care on their own that they don't nee too much time
  updateTacho_NEW();

  #if defined(useOTAUpdate) && !defined(useOTA_RTOS)
  // If you do not use FreeRTOS, you have to regulary call the handle method
  ArduinoOTA.handle();
  #endif
  // mqtt_loop() is doing mqtt keepAlive, processes incoming messages and hence triggers callback
  #ifdef useMQTT
  mqtt_loop();
  #endif

  unsigned long currentMillis = millis();

  // functions that shall be called every 1000 ms
  if ((currentMillis - previousMillis1000Cycle) >= interval1000Cycle) {
    previousMillis1000Cycle = currentMillis;

    #ifdef useTemperatureSensorBME280
    updateBME280();
    #endif
    #ifdef useAutomaticTemperatureControl
    setFanPWMbasedOnTemperature();
    #endif
    #ifdef useHomeassistantMQTTDiscovery
    if (((currentMillis - timerStartForHAdiscovery) >= WAITAFTERHAISONLINEUNTILDISCOVERYWILLBESENT) && (timerStartForHAdiscovery != 0)) {
      mqtt_publish_hass_discovery();
    }
    #endif
  }
  // Call the watchdog reset function in the loop to prevent the ESP32 from resetting
 timerWrite(watchdogTimer, 0); // reset the timer
 timerAlarmEnable(watchdogTimer);
//functions that shall be called every 10000 ms
  if ((currentMillis - previousMillis10000Cycle) >= interval10000Cycle) {
    previousMillis10000Cycle = currentMillis;

    #ifdef useMQTT
    mqtt_publish_tele();
    #endif
    doLog();
  }
boolean DO_I_needtoupdatescreen = GetScreenStatus();
  // functions that shall be called every 5000 ms
  if ((currentMillis - previousMillis5000Cycle) >= interval5000Cycle || DO_I_needtoupdatescreen ) {
    previousMillis5000Cycle = currentMillis;

   Guage_test();

   if(DO_I_needtoupdatescreen==true){
     SetScreenStatus(false);
   }
  }


  // Read the state of the buttons
  int Push_button_state1 = digitalRead(Button1);
  int Push_button_state2 = digitalRead(Button2);

  // Check if Button1 is pressed and debounce for Button1 (Increase temp)
  if (millis() - lastDebounceTime1 > debounceDelay) {
    if (Push_button_state1 == HIGH) { // Button1 is pressed
      lastDebounceTime1 = millis();   // Update the debounce time
      float actT = getTargetTemperature();
      actT += 1.00;      // Increase temperature by 1.00
      saveTemperature(actT);
     // Serial.print("Increased target temperature: ");
     // Serial.println(actT);
      SetScreenStatus(true);
    }
  }

  // Check if Button2 is pressed and debounce for Button2 (Decrease temp)
  if (millis() - lastDebounceTime2 > debounceDelay) {
    if (Push_button_state2 == HIGH) { // Button2 is pressed
      lastDebounceTime2 = millis();   // Update the debounce time
        float actT = getTargetTemperature();
      actT -= 1.00;      // Decrease temperature by 1.00
      saveTemperature(actT);
     // Serial.print("Decreased target temperature: ");
     // Serial.println(actT);
    SetScreenStatus(true);
    }
  }




if (Getsavestatus()==true){
float actT = getTargetTemperature();
saveTemperature(actT);
SETsavestatus();// Sets to false so we save one time
}


}
