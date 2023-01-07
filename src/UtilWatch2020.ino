/*
 * Project UtilWatch2020
 * Description: Fresh implementation of utility watch using MQTT and targeting Home Assistant
 * Author: C. Catlett
 * Date: October 2020
 * Last major update February 2021
 * Update 9/7/22 new hvac system doesn't have a handy AC outlet that is on only when system is on, so 
 * Removing relevant hvac monitoring code for the moment. (lines 201, 112-114, 180-195)
 * Also commented out lines 208-211 which were incorrect
 * 
 * This code watches several sensors and periodically sends values to Home Assistant using MQTT
 */

#include <DS18B20.h>
#include "math.h"
#include <MQTT.h>
#include <OneWire.h>


// secrets.h file to hold username, password, and servername or IP address
#include "secrets.h"
// our mqtt topics (must also define in hass configuration.yaml)
#include "topics.h"
// bunch of variables we use, pin assignments, etc.
#include "vars.h"


// MQTT 
#define MQTT_KEEPALIVE 30 * 60              //  sec 
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

// failsafe and housekeeping stuff

retained int  mqttFailCount  = 0; // keep track of mqtt reliability
retained int  mqttCount      = 0;
retained bool REBORN         = FALSE; // did application watchdog revive us?
// Application watchdog - sometimes MQTT wedges things
ApplicationWatchdog *wd;
void watchdogHandler() {
  REBORN = TRUE;
  System.reset(RESET_NO_WAIT);
}

// interrupt timers
Timer sumpTimer   (sumpCheckFreq, checkSump);   // every checkFreq ms check the sump current
Timer allTimer    (allCheckFreq,    checkAll);  // every allCheckFreq ms check the others
Timer alertTimer  (WINDOW, siren);              // let's blow the wistle every 15 min if needed
Timer reportTimer (mqttFreq, timeToReport);     // report via MQTT

/* ********************************************************************************* */

void setup() {
    Time.zone             (-5);             // sweet home, Chicago
    Particle.syncTime       ();
    pinMode(sumpPin,    INPUT);
    pinMode(hvacPin,    INPUT);
    pinMode(waterPin,   INPUT);
        
    wd = new ApplicationWatchdog(60000, watchdogHandler, 1536); // restart after 60s no pulse

    if (REBORN) {
      Particle.publish("****WEDGED****", "app watchdog restart", 3600, PRIVATE);
      REBORN = FALSE;
    }

    Particle.publish("MQTT", String("Previous Fail rate " + String(mqttFailCount) + "/" + String(mqttCount)),3600, PRIVATE);
    mqttFailCount = 0;
    mqttCount = 0;
    delay(1000);

    for (int i=0; i<SMAX; i++) sumpRuns[i] = millis()-dutyWindow; // make sump run records >>30 min ago
    // set timers
    alertTimer.start();
    allTimer.start();
    reportTimer.start();
    sumpTimer.start();

    // make sure mqtt is working
    client.connect(CLIENT_NAME, HA_USR, HA_PWD); //see secrets.h
    if (client.isConnected()) { Particle.publish("MQTT", "Connected to HA", 3600, PRIVATE);
      } else {  Particle.publish("MQTT", "Failed connect HA - check secrets.h", 3600, PRIVATE); }
}
/*
 All the action (checking sensors, tracking duty cycles) happens via interrupts so 
 we just spinwait until it's time to report.  So checking and reporting are totally decoupled. 
 */
 
void loop() {

    if (reportNow) {
      reportNow = FALSE;
      wd->checkin();
      if (client.isConnected()) {
    //    Particle.publish("mqtt", "connected OK", 3600, PRIVATE); delay(100);
        } else { 
          delay(1000);
          Particle.publish("mqtt", "reconnecting", 3600, PRIVATE); delay(100);
          client.connect(CLIENT_NAME, HA_USR, HA_PWD);
        }

      if (client.isConnected()){
        //Particle.publish("DEBUG", "firing multiple mqtt", 3600, PRIVATE); delay(100);
        //if (sumpOn)     { tellHASS(TOPIC_A, String(sumpCur)); }
        //          else  { tellHASS(TOPIC_B, String(sumpCur)); }
/* rm hvac for now
        if (hvacOn)     { tellHASS(TOPIC_C, String(hvacCur)); }
                  else  { tellHASS(TOPIC_D, String(hvacCur)); } */
        if (heaterOn)   { tellHASS(TOPIC_E, String(waterTemp)); }
                  else  { tellHASS(TOPIC_F, String(waterTemp)); }
                              
        tellHASS(TOPIC_H, String(runCount));  
        tellHASS(TOPIC_I, String(sumpCur));    
/* rm hvac for now
        tellHASS(TOPIC_J, String(hvacCur));    */
        tellHASS(TOPIC_K, String(waterTemp));  
        tellHASS(TOPIC_N, String(ambientTemp)); 
        tellHASS(TOPIC_TH, String(mqttCount));
        tellHASS(TOPIC_MF, String(mqttFailCount));
        } else {
          delay(1000);
          Particle.publish("mqtt", "Failed to connect", 3600, PRIVATE);
          mqttFailCount++;
        }
      delay(1000);
      Particle.publish("MQTT", String("Fail rate " + String(mqttFailCount) + "/" + String(mqttCount)),3600, PRIVATE);
      void myWatchdogHandler(void); // reset the dog
    }
}
/************************************/
/***        TIMER FUNCTIONS       ***/
/************************************/
//
// time to report
//
void timeToReport() {
  reportNow = TRUE;
}

// timed check of sump - this happens more frequently than HVAC or water heater because 
// a sump run is typically only a few tens of seconds whereas the others run for many minutes
//
void checkSump () {
    sumpCur = analogRead(sumpPin);
    if (sumpCur > PUMP_ON) {
        if (!sumpOn) {
            //sumpEvent = true;
            sumpStart = millis();
            tellHASS(TOPIC_A, String(sumpCur)); 
            tellHASS(TOPIC_I, String(sumpCur));   
        }
        sumpOn = true;
    } else {
        if (sumpOn) {
            sumpEvent = true;
            sumpDuration = (millis() - sumpStart)/1000;     // sump event duration in seconds
            sumpRuns[dutyPtr] = millis();                   // record the event in the duty cycle counter buffer
            dutyPtr = (dutyPtr + 1) % SMAX;                 // advance pointer in the circular cycle counter buffer
            tellHASS(TOPIC_B, String(sumpCur));
            tellHASS(TOPIC_I, String(sumpCur));   
            sumpOn = false;
        }
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
   /* disable hvac after new system installed
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
    } */
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
        delay(1000);
        Particle.publish("Danger", "sump", PRIVATE);
        tellHASS(TOPIC_L, tString);
    } else {
        tellHASS(TOPIC_M, tString);
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


// mqtt comms

void tellHASS(const char *ha_topic, String ha_payload) {
  int returnCode = 0;
  delay(100); // take it easy on the server
  if(client.isConnected()) {
    returnCode = client.publish(ha_topic, ha_payload);
    if (returnCode != 1) {
      delay(1000);
      Particle.publish("mqtt return code = ", String(returnCode), 3600, PRIVATE);
      client.disconnect();
    } else mqttCount++;
  } else {
    delay(1000);
    Particle.publish("mqtt", "Connection dropped", 3600, PRIVATE);
    client.connect(CLIENT_NAME, HA_USR, HA_PWD);
    returnCode = client.publish(ha_topic, ha_payload);
    if (returnCode !=1) {
      delay(500);
      mqttFailCount++;
    } else {
      mqttCount++;
    }
  }
}
