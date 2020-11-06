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

// comment this out if not using a secrets.h file for your username, password, and servername 
// or IP address
#include "secrets.h"

// use particle console messages to debug, watch values to calibrate, etc.
bool    DEBUG   = FALSE;

// sensor pins (what pins on the Photon go to what sensors)
#define waterPin    D0                      // pin for ds18b20-A to water heater chimney
#define hvacPin     A0                      // pin for HVAC fan current sensor
#define sumpPin     A1                      // pin for sump pump current sensor

// ds18b20 temperature sensor instance
DS18B20  sensor(waterPin, true);
#define MAXRETRY    4                       // max times to poll temperature pin before giving up

/*
 * MQTT parameters
 */
#define MQTT_KEEPALIVE 30 * 60              // 30 minutes but afaict it's ignored...

/* 
 * When you configure Mosquitto Broker MQTT in HA you will set a
 * username and password for MQTT - plug these in here if you are not
 * using a secrets.h file.
 */
//const char *HA_USR = "your_ha_mqtt_usrname";
//const char *HA_PWD = "your_ha_mqtt_passwd";
//uncomment this line and fill in w.x.y.z if you are going by IP address,:
//  byte MY_SERVER[] = { 73, 246, 85, 17 };
// OR this one if you are doing hostname (filling in yours)
//  #define MY_SERVER "your.mqtt.broker.tld"

const char *CLIENT_NAME = "photon";

// Topics - these are what you watch for as triggers in HA automations
const char *TOPIC_A = "ha/util/sumpON";
const char *TOPIC_B = "ha/util/sumpOFF";
const char *TOPIC_C = "ha/util/hvacON";
const char *TOPIC_D = "ha/util/hvacOFF";
const char *TOPIC_E = "ha/util/whON";
const char *TOPIC_F = "ha/util/whOFF";
const char *TOPIC_G = "ha/util/sumpALRM";
const char *TOPIC_H = "ha/util/sumpCount";
const char *TOPIC_I = "ha/util/sumpCur";
const char *TOPIC_J = "ha/util/hvacCur";
const char *TOPIC_K = "ha/util/waterT";
const char *TOPIC_L = "ha/util/sumpWrn";
const char *TOPIC_M = "ha/util/sumpOK";

// MQTT functions
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void timer_callback_send_mqqt_data();    // tbh I don't know what this is as it seems not used and spells mqtt wrong

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
int     MOTOR_ON        = 100;              // need to calibrate - resting state ~40-50 for my sump pump
int     FLAME_ON        = 190;              // same here - your chimney temp may vary 
// interrupt timers all prime #s to minimize collisions, since I don't trust the Photon to handle colliding interrupts well
int     sumpCheckFreq   = 2003;             // check sump every ~2 seconds since it typically runs only for 20s or so
int     allCheckFreq    = 17351;            // check hvac and water heater less often as they have longer duty cycles
double  reportFreq      = 23431;            // report a var to ThingSpeak every ~23 seconds
double  mqttFreq        = 119993;           // report a var to HASS via MQTT every ~2 minutes
double  lastReport      = 0;
double  lastMQTT        = 0;
int     reportCount     = 0;
#define HIST              32               // number of sump checks to keep in short term memory
#define DANGER		      6		           // number of times sump runs in 30 min before I worry

// global variables
bool    sumpOn          = false;            // state variables
bool    hvacOn          = false;
bool    heaterOn        = false;
int     sumpCur         = 0;                // sensor values
int     hvacCur         = 0;
double  waterTemp       = 70;
double  lastTemp        = 70;
String  tString;                            // string buffer to hold timestamp

// statistics variables
int     sumpHistory      [HIST];            // all sump readings between reports
int     sumpPointer     = 0;
bool    sumpEvent       = false;
bool    hvacEvent       = false;
int     sumpStart       = 0;                // start time of sump event
int     sumpDuration    = 0;                // duration of sump event
int     hvacStart       = 0;                // start time of hvac event
int     hvacDuration    = 0;                // duration of hvac event

// sump duty cycle variables
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
    // set up vars to export (tip- compiler barfs if the variable alias (in quotes) is longer than 12 characters)
    // get rid of these if you're using cellular (Electron) - they devour bandwidth staying in sync
    Particle.variable("sumpCount",  runCount);
    Particle.variable("sumpCur",    sumpCur);
    Particle.variable("hvacCur",    hvacCur);
    Particle.variable("heatTemp",   waterTemp);
    // zero out the sump history
    for (int i=0; i<HIST; i++) sumpHistory[i] = 0;
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
    if (DEBUG) { Particle.publish("DEBUG", "ON", 3600, PRIVATE); 
        } else {
            Particle.publish("DEBUG", "OFF", 3600, PRIVATE);
        }

    }

/*
 All the action (checking sensors, tracking duty cycles) happens via interrupts so 
 we just spinwait until it's time to report and since ThingSpeak has throttles we just 
 report one thing at a time.  So checking stuff and reporting it are totally decoupled. 
 */
 
void loop() {

    if ((millis() - lastReport) > reportFreq) {         // time to report yet? 
    // Report to ThingSpeak (just comment this out unless you're a ThingSpeak user- it's not needed for HA integration)
        ThingSpeakReport();
        lastReport = millis();
    }
    // Report to HA via MQTT
    if ((millis() - lastMQTT) > mqttFreq) {
        lastMQTT = millis();

        Particle.publish("mqtt", "Pushing state data", 3600, PRIVATE); delay(500);
            
        if (sumpOn) {   tellHASS(TOPIC_A, String(sumpCur));   if (DEBUG) Particle.publish("mqtt", TOPIC_A, PRIVATE);  }
                  else  {   tellHASS(TOPIC_B, String(sumpCur));   if (DEBUG) Particle.publish("mqtt", TOPIC_B, PRIVATE);  }
                  
        delay(500);
            
        if (hvacOn) {   tellHASS(TOPIC_C, String(hvacCur));   if (DEBUG) Particle.publish("mqtt", TOPIC_C, PRIVATE);  }
                  else  {   tellHASS(TOPIC_D, String(hvacCur));   if (DEBUG) Particle.publish("mqtt", TOPIC_D, PRIVATE);  }
                  
        delay(500);
            
        if (heaterOn) { tellHASS(TOPIC_E, String(waterTemp)); if (DEBUG) Particle.publish("mqtt", TOPIC_E, PRIVATE); }
                  else    { tellHASS(TOPIC_F, String(waterTemp)); if (DEBUG) Particle.publish("mqtt", TOPIC_F, PRIVATE); }
                  
        delay(500);
            
        Particle.publish("mqtt", "Pushing sensor data", 3600, PRIVATE); delay(500);

        tellHASS(TOPIC_H, String(runCount));  if (DEBUG) Particle.publish("mqtt_sump_ct", String(runCount), PRIVATE);  delay(500); 
        tellHASS(TOPIC_I, String(sumpCur));   if (DEBUG) Particle.publish("mqtt_sump_cur", String(sumpCur), PRIVATE);  delay(500);
        tellHASS(TOPIC_J, String(hvacCur));   if (DEBUG) Particle.publish("mqtt_hvac_cur", String(hvacCur), PRIVATE);  delay(500);
        tellHASS(TOPIC_K, String(waterTemp)); if (DEBUG) Particle.publish("mqtt_whtr_temp", String(waterTemp), PRIVATE);  
            
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

    if (DEBUG) Particle.publish("timerDebug", "checked sump", PRIVATE);
    sumpCur = analogRead(sumpPin);
    sumpPointer = (sumpPointer+1) % HIST;
    sumpHistory[sumpPointer] = sumpCur;
    if (sumpCur > MOTOR_ON) {
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
            runCount = 0;                                   // how many of the past SMAX runs are less than dutyWindow ms prior to now?
            for (int i=0; i<SMAX; i++) 
                if ((millis() - sumpRuns[i]) < dutyWindow) runCount++;
        }
        sumpOn = false;
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
}
// DANGER ~6 is a heuristic - I have observed that when my sump runs 7x in 30 minutes I should be paying attention
// as a failure would be catastrophic under such conditions (ymmv)
// honestly I don't recall why I put this here rather than in checkSump, but hey it ain't broke so... ¯\_(ツ)_/¯
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
// poll the temperature sensor in the water heater chimney
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
// put the mqtt stuff in one place since the error detect/correct
// due to oddly short connection timeouts (ignoring MQTT_KEEPALIVE afaict)
// require recovery code

void tellHASS (const char *ha_topic, String ha_payload) {

  client.connect(CLIENT_NAME, HA_USR, HA_PWD);
  client.publish(ha_topic, ha_payload);
  client.disconnect();
}


/*
 * This code is useful if one wants to use webhooks at the Particle Cloud
 * to send data to ThingSpeak.com - it predates the Home Assistant setup but I've
 * kept it as it now has several years of historical data about my appliance behavior
 */
void ThingSpeakReport() {
    // Report to Particle cloud --> webhooks --> ThingSpeak
        int cases = 5;                                  // hard code case # here, feels like a kluge
        switch ((reportCount++) % cases) {              // round robin reporting one var at a time
            case 0:                                     // sump current
                for (int j=0;j<HIST;j++) {              // report the max we've seen in case we missed a short run
                    sumpCur = max (sumpCur, sumpHistory[j]);
                    sumpHistory[j]=0;
                }
                Particle.publish("sumpCurrent", String(sumpCur),    PRIVATE);
                break;
            case 1:                                     // hvac current
                Particle.publish("hvacCurrent", String(hvacCur),    PRIVATE);
                break;
            case 2:                                     // water heater chimney temperature
                Particle.publish("waterTemp",   String(waterTemp),  PRIVATE);
                break;
            case 3:                                     // duration of last hvac event (if there was one; 0 if not)
                if (hvacEvent) {
                    if (!hvacOn) Particle.publish("hvacEvent", String(hvacDuration), PRIVATE);
                    hvacEvent = false;
                } else {
                    Particle.publish("hvacEvent", "0", PRIVATE);
                }
                break;
            case 4:                                     // duration of last sump event (if there was one; 0 if not)
                if (sumpEvent) {
                    Particle.publish("sumpEvent", String(sumpDuration), PRIVATE);
                    sumpEvent = false;
                } else {
                    Particle.publish("sumpEvent", "0", PRIVATE);
                }
                break;
        }
            
}
