// sensor pins (what pins on the Photon go to what sensors)
#define waterPin    D2                      // pin for ds18b20-A to water heater chimney
#define hvacPin     A0                      // pin for HVAC fan current sensor
#define sumpPin     A4                      // pin for sump pump current sensor
#define ambientPin  D4                      // another ds18b20 for ambient basement temperature
#define MAXRETRY    4                       // max times to poll temperature pin before giving up
DS18B20  sensor(waterPin, true);            // water heater
DS18B20 ambient(ambientPin, true);          // ambient temp

// global constant parameters
int     WINDOW          = 900000;           // check to see if sump is in danger every 15 minutes
int     PUMP_ON         = 100;              // need to calibrate - resting state ~40-50 for my sump pump
int     FLAME_ON        = 190;              // same here - your chimney temp may vary 
int     MOTOR_ON        = 2500;             // is hvac running?
// timer intervals
int     sumpCheckFreq   = 2003;             // check sump every ~2 seconds since it typically runs only for 20s or so
int     allCheckFreq    = 17351;            // check hvac and water heater less often as they have longer duty cycles
double  mqttFreq        = 300007;           // report vars to HASS via MQTT every ~5 minutes
bool    reportNow       = TRUE;             // time to report yet?
// keep track of how mqtt is doing seeing as we ain't using QoS
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
double  ambientTemp     = 60;
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

