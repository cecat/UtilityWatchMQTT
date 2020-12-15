/*
 * Project UtilWatch2020
 * Description: Fresh implementation of utility watch using MQTT and targeting Home Assistant
 * Author: C. Catlett
 * Date: October 2020
 * 
 * This code watches three sensors and periodically sends values to both Home Assistant and to
 * a nice free graphing site, ThingSpeak.com.  For the latter you have to set up webhooks in
 * the Particle.io cloud. 
 */

#include <MQTT.h>
#include <OneWire.h>
#include <DS18B20.h>
#include "math.h"

// create a secrets.h file for your username, password, and servername 
// or IP address
#include "secrets.h"

// our mqtt topics
#include "topics.h"

// use particle console messages to debug, watch values to calibrate, etc.
bool    DEBUG   = FALSE;

// sensor pins (what pins on the Photon go to what sensors)
#define waterPin    D2                      // pin for ds18b20-A to water heater chimney
#define hvacPin     A0                      // pin for HVAC fan current sensor
#define sumpPin     A4                      // pin for sump pump current sensor
#define ambientPin  D4                      // another ds18b20 for ambient basement temperature

// ds18b20 temperature sensor instance
DS18B20  sensor(waterPin, true);
#define MAXRETRY    4                       // max times to poll temperature pin before giving up
// ds18b20 for ambient temperature
DS18B20 ambient(ambientPin, true);

/*
 * MQTT parameters
 */
#define MQTT_KEEPALIVE 30 * 60              // 30 minutes but afaict it's ignored...

const char *CLIENT_NAME = "photon";

// MQTT functions
void timer_callback_send_mqqt_data();    
 // MQTT callbacks implementation
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
     char p[length + 1];
     memcpy(p, payload, length);
     p[length] = 0; // was = NULL but that threw a warning
     Particle.publish("mqtt", p, 3600, PRIVATE);
 }

MQTT client(MY_SERVER, 1883, MQTT_KEEPALIVE, mqtt_callback);

// global constant parameters
int     WINDOW          = 900000;           // check to see if sump is in danger every 15 minutes
int     PUMP_ON         = 100;              // need to calibrate - resting state ~40-50 for my sump pump
int     FLAME_ON        = 190;              // same here - your chimney temp may vary 
int     MOTOR_ON        = 2500;             // is hvac running?
// interrupt timers all prime #s to minimize collisions, since I don't trust the Photon to handle colliding interrupts well
int     sumpCheckFreq   = 2003;             // check sump every ~2 seconds since it typically runs only for 20s or so
int     allCheckFreq    = 17351;            // check hvac and water heater less often as they have longer duty cycles
double  reportFreq      = 23431;            // report a var to ThingSpeak every ~23 seconds
double  mqttFreq        = 119993;           // report a var to HASS via MQTT every ~2 minutes
double  lastMQTT        = 0;
int     reportCount     = 0;

// global variables
bool    sumpOn          = false;            // state variables
bool    hvacOn          = false;
bool    heaterOn        = false;
int     sumpCur         = 0;                // sensor values
int     hvacCur         = 0;
double  waterTemp       = 70;
double  lastTemp        = 70;
double  ambientTemp     = 50;
String  tString;                            // string buffer to hold timestamp

// statistics variables
bool    sumpEvent       = false;
bool    hvacEvent       = false;
int     sumpStart       = 0;                // start time of sump event
int     sumpDuration    = 0;                // duration of sump event
int     hvacStart       = 0;                // start time of hvac event
int     hvacDuration    = 0;                // duration of hvac event

// sump duty cycle variables
#define DANGER		      6		           // number of times sump runs in 30 min before I worry
#define SMAX              16                // maximum we might ever see the sump run in a window
int     sumpRuns        [SMAX];             // keep track of how many time sump runs in a given window of time
double  dutyWindow      = 1800000;          // set the sump duty cycle of interest window to 30 minutes
int     dutyPtr         = 0;                // pointer into dutyWindow array
int     runCount        = 0;                // sump runcount past WINDOW ms

// interrupt timers
Timer sumpTimer(sumpCheckFreq, checkSump);  // every checkFreq ms check the sump current
Timer allTimer(allCheckFreq,    checkAll);  // every allCheckFreq ms check the others
Timer alertTimer(WINDOW, siren);            // let's blow the wistle every 15 min if needed

/*
  prelims
*/

void setup() {
    Time.zone             (-5);             // sweet home, Chicago
    Particle.syncTime       ();
    pinMode(sumpPin,    INPUT);
    pinMode(hvacPin,    INPUT);
    pinMode(waterPin,   INPUT);
    Serial.begin      (115200);
    for (int i=0; i<SMAX; i++) sumpRuns[i] = millis()-dutyWindow; // make all of sump run entries >>30 min ago on startup
    // set timers
    sumpTimer.start();
    allTimer.start();
    alertTimer.start();
    // connect to mqtt broker
    client.connect(CLIENT_NAME, HA_USR, HA_PWD);
    if (client.isConnected()) {
        Particle.publish("mqtt", "Connected to HA", 3600, PRIVATE);
        } else {
            Particle.publish("mqtt", "Failed to connect to HA - check IP address", 3600, PRIVATE);
        }
}
/*
 All the action (checking sensors, tracking duty cycles) happens via interrupts so 
 we just spinwait until it's time to report and since ThingSpeak has throttles we just 
 report one thing at a time.  So checking stuff and reporting it are totally decoupled. 
 */
 
void loop() {

    // Report to HA via MQTT
    if ((millis() - lastMQTT) > mqttFreq) {
        lastMQTT = millis();

        if (DEBUG) Particle.publish("mqtt", "Pushing state data", 3600, PRIVATE);
        if (sumpOn)     { tellHASS(TOPIC_A, String(sumpCur)); }
                  else  { tellHASS(TOPIC_B, String(sumpCur)); }
        if (hvacOn)     { tellHASS(TOPIC_C, String(hvacCur)); }
                  else  { tellHASS(TOPIC_D, String(hvacCur)); }
        if (heaterOn)   { tellHASS(TOPIC_E, String(waterTemp)); }
                  else  { tellHASS(TOPIC_F, String(waterTemp)); }
                              
        if (DEBUG) Particle.publish("mqtt", "Pushing sensor data", 3600, PRIVATE);
        tellHASS(TOPIC_H, String(runCount));  
        tellHASS(TOPIC_I, String(sumpCur));    
        tellHASS(TOPIC_J, String(hvacCur));    
        tellHASS(TOPIC_K, String(waterTemp));  
        tellHASS(TOPIC_N, String(ambientTemp)); 
            
    }
}
/************************************/
/***        TIMER FUNCTIONS       ***/
/************************************/
//
// timed check of sump - this happens more frequently than HVAC or water heater because 
// a sump run is typically only a few tens of seconds whereas the others run for many minutes
//
void checkSump () {

    sumpCur = analogRead(sumpPin);
    if (sumpCur > PUMP_ON) {
        if (!sumpOn) {
            //sumpEvent = true;
            sumpStart = millis();
        }
        sumpOn = true;
    } else {
        if (sumpOn) {
            sumpEvent = true;
            sumpDuration = (millis() - sumpStart)/1000;     // sump event duration in seconds
            sumpRuns[dutyPtr] = millis();                   // record the event in the duty cycle counter buffer
            dutyPtr = (dutyPtr + 1) % SMAX;                 // advance pointer in the circular cycle counter buffer
        }
    sumpOn = false;
    runCount = 0;                                           // how many of the past SMAX runs are less than 
    for (int i=0; i<SMAX; i++)                              // dutyWindow ms prior to now?
        if ((millis() - sumpRuns[i]) < dutyWindow) runCount++;
    }
}
//
// timed check of HVAC and water heater 
//
void checkAll () {    
    // hvac
    hvacCur = analogRead(hvacPin);
    if (hvacCur > MOTOR_ON) {
        if (!hvacOn) {
            hvacEvent = true;
            hvacStart = millis();
        }
        hvacOn = true;
    } else {
        if (hvacOn) {
            hvacEvent = true;
            hvacDuration = (millis() - hvacStart)/60000;     // hvac event duration in minutes
        }
        hvacOn = false;
    }
    // water heater
    lastTemp  = waterTemp;
    waterTemp = getTemp();
    if (waterTemp < 50) waterTemp = lastTemp;
    if (waterTemp > FLAME_ON) {
        if (!heaterOn) {
         //   hvacEvent = true;
            hvacStart = millis();
        } 
        heaterOn = true;
    } else {
       if (heaterOn) {
            hvacEvent = true;
            hvacDuration = (millis() - hvacStart)/60000;     // hvac event duration in minutes
        } 
        heaterOn = false;
    }
    // basement temperature
    ambientTemp = getAmbientTemp();
}

// DANGER ~6 is a heuristic - I have observed that when my sump runs 7x in 30 minutes I should be paying attention
// as a failure would be catastrophic under such conditions (ymmv)
void siren(){
    time_t time = Time.now(); // get the current time
    Time.format(time, TIME_FORMAT_DEFAULT); // format the string
    tString = Time.timeStr();  // update the exposed variable (date-time stamp)
    if (runCount > DANGER) {
        Particle.publish("Danger", "sump", PRIVATE);
        client.publish(TOPIC_L, tString);

    } else {
        client.publish(TOPIC_M, tString);
    }
}

/************************************/
/***      ON-DEMAND FUNCTIONS     ***/
/************************************/

//
// poll a ds18b20 temperature sensor 
//

double getTemp() {  // using example code from the DS18B20 library
  float _temp;
  float fahrenheit = 0;
  int   i = 0;

  do {
    _temp = sensor.getTemperature();
  } while (!sensor.crcCheck() && MAXRETRY > i++);

  if (i < MAXRETRY) {
    fahrenheit = sensor.convertToFahrenheit(_temp);
    Serial.println(fahrenheit);
  }
  else {
    Serial.println("Invalid reading");
  }
  return (fahrenheit); // F, because this is Amurica
}

//
// check ambient temperature too
//
double getAmbientTemp() {  // using example code from the DS18B20 library
  float _temp;
  float fahrenheit = 0;
  int   i = 0;

  do {
    _temp = ambient.getTemperature();
  } while (!ambient.crcCheck() && MAXRETRY > i++);

  if (i < MAXRETRY) {
    fahrenheit = ambient.convertToFahrenheit(_temp);
    Serial.println(fahrenheit);
  }
  else {
    Serial.println("Invalid reading");
  }
  return (fahrenheit); // F, because this is Amurica
}
//
// put the mqtt stuff in one place since the error detect/correct
// due to oddly short connection timeouts (ignoring MQTT_KEEPALIVE afaict)
// require recovery code

void tellHASS (const char *ha_topic, String ha_payload) {

  client.connect(CLIENT_NAME, HA_USR, HA_PWD);
  client.publish(ha_topic, ha_payload);
  client.disconnect();
}