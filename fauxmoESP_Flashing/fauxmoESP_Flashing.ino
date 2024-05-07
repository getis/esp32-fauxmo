#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include "fauxmoESP.h"

// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your router configuration
#include "credentials.h"

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200

#define LED_YELLOW          16
#define LED_GREEN           4
#define LED_BLUE            0
#define LED_PINK            2
#define LED_WHITE           15

#define ID_YELLOW           "yellow lamp"
#define ID_GREEN            "green lamp"
#define ID_BLUE             "blue lamp"
#define ID_PINK             "pink lamp"
#define ID_WHITE            "white lamp"

int lamp_states[5] = {0,0,0,0,0}; // will hold on/off states
int lamp_values[5] = {0,0,0,0,0}; // will hold values
int lamp_toggle_state[5] = {0,0,0,0,0}; // will hold current lamp toggle state
int lamp_counters[5] = {0,0,0,0,0}; // will hold timer count
unsigned long lamp_timer_last = millis();
int lamp_timer_period = 10; // count every 100 millis

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

void setup() {

    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();

    // LEDs
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_PINK, OUTPUT);
    pinMode(LED_WHITE, OUTPUT);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_PINK, LOW);
    digitalWrite(LED_WHITE, LOW);


    // Wifi
    wifiSetup();

    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)

    // Add virtual devices
    fauxmo.addDevice(ID_YELLOW);
    fauxmo.addDevice(ID_GREEN);
    fauxmo.addDevice(ID_BLUE);
    fauxmo.addDevice(ID_PINK);
    fauxmo.addDevice(ID_WHITE);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        if (strcmp(device_name, ID_YELLOW)==0) {
            digitalWrite(LED_YELLOW, state ? HIGH : LOW);
            lamp_states[0] = state;
            lamp_values[0]  =value;
        } else if (strcmp(device_name, ID_GREEN)==0) {
            digitalWrite(LED_GREEN, state ? HIGH : LOW);
            lamp_states[1] = state;
            lamp_values[1]  =value;
        } else if (strcmp(device_name, ID_BLUE)==0) {
            digitalWrite(LED_BLUE, state ? HIGH : LOW);
            lamp_states[2] = state;
            lamp_values[2]  =value;
        } else if (strcmp(device_name, ID_PINK)==0) {
            digitalWrite(LED_PINK, state ? HIGH : LOW);
            lamp_states[3] = state;
            lamp_values[3]  =value;
        } else if (strcmp(device_name, ID_WHITE)==0) {
            digitalWrite(LED_WHITE, state ? HIGH : LOW);
            lamp_states[4] = state;
            lamp_values[4]  =value;
        }

    });

}


void loop() {

    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();

    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);

    // handle lamp toggles

    // int[5] lamp_states = {0,0,0,0,0}; // will hold on/off states
    // int[5] lamp_values = {0,0,0,0,0}; // will hold values
    // int[5] lamp_toggle_state = {0,0,0,0,0}; // will hold current lamp toggle state// int[5] lamp_counters = {0,0,0,0,0}; // will hold timer count
    // unsigned long lamp_timer_last = millis();
    // int lamp_timer_period = 100; // count every 100 millis
    
    if(millis() - lamp_timer_last > lamp_timer_period) {
      // do lamp toggle count
      lamp_timer_last = millis();

      // yellow
      lamp_counters[0] = lamp_counters[0] - 1;
      if(lamp_counters[0] <= 0){
        lamp_toggle_state[0] = !lamp_toggle_state[0]; // toggle lamp state
        lamp_counters[0] = lamp_values[0]; // reset toggle count
        if(lamp_states[0]){
          // set output if lamp is on
          digitalWrite(LED_YELLOW, lamp_toggle_state[0] ? HIGH : LOW);
        }
        else {
          // turn it off
          digitalWrite(LED_YELLOW, LOW);
        }
      }

      // green
      lamp_counters[1] = lamp_counters[1] - 1;
      if(lamp_counters[1] <= 0){
        lamp_toggle_state[1] = !lamp_toggle_state[1]; // toggle lamp state
        lamp_counters[1] = lamp_values[1]; // reset toggle count
        if(lamp_states[1]){
          // set output if lamp is on
          digitalWrite(LED_GREEN, lamp_toggle_state[1] ? HIGH : LOW);
        }
        else {
          // turn it off
          digitalWrite(LED_GREEN, LOW);
        }
      }

      // blue
      lamp_counters[2] = lamp_counters[2] - 1;
      if(lamp_counters[2] <= 0){
        lamp_toggle_state[2] = !lamp_toggle_state[2]; // toggle lamp state
        lamp_counters[2] = lamp_values[2]; // reset toggle count
        if(lamp_states[2]){
          // set output if lamp is on
          digitalWrite(LED_BLUE, lamp_toggle_state[2] ? HIGH : LOW);
        }
        else {
          // turn it off
          digitalWrite(LED_BLUE, LOW);
        }
      }

      // pink
      lamp_counters[3] = lamp_counters[3] - 1;
      if(lamp_counters[3] <= 0){
        lamp_toggle_state[3] = !lamp_toggle_state[3]; // toggle lamp state
        lamp_counters[3] = lamp_values[3]; // reset toggle count
        if(lamp_states[3]){
          // set output if lamp is on
          digitalWrite(LED_PINK, lamp_toggle_state[3] ? HIGH : LOW);
        }
        else {
          // turn it off
          digitalWrite(LED_PINK, LOW);
        }
      }

      // white
      lamp_counters[4] = lamp_counters[4] - 1;
      if(lamp_counters[4] <= 0){
        lamp_toggle_state[4] = !lamp_toggle_state[4]; // toggle lamp state
        lamp_counters[4] = lamp_values[4]; // reset toggle count
        if(lamp_states[4]){
          // set output if lamp is on
          digitalWrite(LED_WHITE, lamp_toggle_state[4] ? HIGH : LOW);
        }
        else {
          // turn it off
          digitalWrite(LED_WHITE, LOW);
        }
      }
      
    }

}
