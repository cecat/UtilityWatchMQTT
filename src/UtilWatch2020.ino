/*
 * Project UtilWatch2020
 * Description: Fresh implementation of utility watch using MQTT and targeting Home Assistant
 * Author: C. Catlett
 * Date: October 2020
 * Last major update January 2021
 * 
 * This code watches several sensors and periodically sends values to both Home Assistant and to
 * a nice free graphing site, ThingSpeak.com.  For the latter you have to set up webhooks in
 * the Particle.io cloud. 
 */

#include <MQTT.h>
#include <OneWire.h>
#include <DS18B20.h>
#include "math.h"

// secrets.h file to hold username, password, and servername or IP address
#include "secrets.h"

// our mqtt topics (must also define in hass configuration.yaml)
#include "topics.h"

retained bool    DOGGED  = FALSE; // did watchdog rest me?
ApplicationWatchdog *wd;
void watchdogHandler(){
    DOGGED = TRUE;
    System.reset(RESET_NO_WAIT);
}

// sensor pins (what pins on the Photon go to what sensors)
#define waterPin    D2                      // pin for ds18b20-A to water heater chimney
#define hvacPin     A0                      // pin for HVAC fan current sensor
#define sumpPin     A4                      // pin for sump pump current sensor
#define ambientPin  D4                      // another ds18b20 for ambient basement temperature
#define MAXRETRY    4                       // max times to poll temperature pin before giving up
DS18B20  sensor(waterPin, true);            // water heater
DS18B20 ambient(ambientPin, true);          // ambient temp

/*
 * MQTT 
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
retained int mqttFailCount = 0; // keep track of mqtt reliability
retained int mqttCount     = 0;

// global constant parameters
int     WINDOW          = 900000;           // check to see if sump is in danger every 15 minutes
int     PUMP_ON         = 100;              // need to calibrate - resting state ~40-50 for my sump pump
int     FLAME_ON        = 190;              // same here - your chimney temp may vary 
int     MOTOR_ON        = 2500;             // is hvac running?
int     sumpCheckFreq   = 2003;             // check sump every ~2 seconds since it typically runs only for 20s or so
int     allCheckFreq    = 17351;            // check hvac and water heater less often as they have longer duty cycles
double  mqttFreq        = 240007;           // report a var to HASS via MQTT every ~4 minutes
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
#define DANGER		        6		              // number of times sump runs in 30 min before I worry
#define SMAX              16                // maximum we might ever see the sump run in a window
int     sumpRuns        [SMAX];             // keep track of how many time sump runs in a given window of time
double  dutyWindow      = 1800000;          // set the sump duty cycle of interest window to 30 minutes
int     dutyPtr         = 0;                // pointer into dutyWindow array
int     runCount        = 0;                // sump runcount past WINDOW ms

// interrupt timers
Timer sumpTimer(sumpCheckFreq, checkSump);  // every checkFreq ms check the sump current
Timer allTimer(allCheckFreq,    checkAll);  // every allCheckFreq ms check the others
Timer alertTimer(WINDOW, siren);            // let's blow the wistle every 15 min if needed

/* ********************************************************************************* */

void setup() {
    Time.zone             (-5);             // sweet home, Chicago
    Particle.syncTime       ();
    pinMode(sumpPin,    INPUT);
    pinMode(hvacPin,    INPUT);
    pinMode(waterPin,   INPUT);
    Serial.begin      (115200);
        
    Particle.publish("MQTT", String("Fail rate " + String(mqttFailCount) + "/" + String(mqttCount)),3600, PRIVATE);
    mqttFailCount = 0;
    mqttCount = 0;

    if (DOGGED) { // hmmm, the application watchdog reset us
       Particle.publish("DOGGIES", "App Watchdog Reset", PRIVATE);
       DOGGED = FALSE;
    }
    for (int i=0; i<SMAX; i++) sumpRuns[i] = millis()-dutyWindow; // make sump run records >>30 min ago
    // set timers
    sumpTimer.start();
    allTimer.start();
    alertTimer.start();
    // make sure mqtt is working
    client.connect(CLIENT_NAME, HA_USR, HA_PWD);
    if (client.isConnected()) { Particle.publish("MQTT", "Connected to HA", 3600, PRIVATE);
    } else {  Particle.publish("MQTT", "Failed to connect to HA - check IP address", 3600, PRIVATE); }

    wd = new ApplicationWatchdog(60000, watchdogHandler, 1536);
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
      client.connect(CLIENT_NAME, HA_USR, HA_PWD);
      if (client.isConnected()) {
        if (sumpOn)     { streamHASS(TOPIC_A, String(sumpCur)); }
                  else  { streamHASS(TOPIC_B, String(sumpCur)); }
        if (hvacOn)     { streamHASS(TOPIC_C, String(hvacCur)); }
                  else  { streamHASS(TOPIC_D, String(hvacCur)); }
        if (heaterOn)   { streamHASS(TOPIC_E, String(waterTemp)); }
                  else  { streamHASS(TOPIC_F, String(waterTemp)); }
                              
        streamHASS(TOPIC_H, String(runCount));  
        streamHASS(TOPIC_I, String(sumpCur));    
        streamHASS(TOPIC_J, String(hvacCur));    
        streamHASS(TOPIC_K, String(waterTemp));  
        streamHASS(TOPIC_N, String(ambientTemp)); 
        streamHASS(TOPIC_TH, String(mqttCount));
        streamHASS(TOPIC_MF, String(mqttFailCount));
        client.disconnect();
        } else {
          Particle.publish("mqtt", "Failed to connect", 3600, PRIVATE);
          mqttFailCount++;
        }
    }
    wd->checkin(); //reset application watchdog timer count
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
    if (millis() > dutyWindow) {
      for (int i=0; i<SMAX; i++)                              // dutyWindow ms prior to now?
        if ((millis() - sumpRuns[i]) < dutyWindow) runCount++;
      }
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
        if (!heaterOn) { //   hvacEvent = true;
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

  do { _temp = sensor.getTemperature();
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
  do { _temp = ambient.getTemperature();
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

  mqttCount++;
  delay(100); // dunno if needed but just to slow the barrage
  client.connect(CLIENT_NAME, HA_USR, HA_PWD);
  if(client.isConnected()) {
    client.publish(ha_topic, ha_payload);
    client.disconnect();
  } else {
    Particle.publish("mqtt", "Failed to connect", 3600, PRIVATE);
    mqttFailCount++;
  }
}

//
// send  messages via an already-open connection
//

void streamHASS(const char *ha_topic, String ha_payload) {

  mqttCount++;
  delay(100); // take it easy on the server
  if(client.isConnected()) {
    client.publish(ha_topic, ha_payload);
  } else {
    Particle.publish("mqtt", "Failed to connect", 3600, PRIVATE);
    mqttFailCount++;
  }
}