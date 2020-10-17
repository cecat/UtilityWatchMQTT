# Integrating with Home Assistant

## MQTT
Add and set up MQTT Mosquitto broker MQTT integration in HA following the [provided documentation](https://github.com/home-assistant/hassio-addons/blob/master/mosquitto/DOCS.md).

In the Particle code (in this repo - src/UtilWatch2020.ino) you'll find several places where you have to customize for your installation.

Username and password (the ones you configured into your Mosquitto integration):

```
const char *HA_USR = "mqtt";
const char *HA_PWD = "mqtt";
```

You will also find down around lines 60-65 a place to plug in the IP address of your Home Assistant server:

`byte server[] = { 192, 168, 7, 157 };`

(You'll want to make a DNS reservation for your HA server so it always gets the same IP address.)

## TOPICS and Automation

Each MQTT message has a "topic" that you will use in your HA automation triggers.  I've defined the ones I use in src/UtilWatch2020.ino around lines 35-40, e.g.:

`const char *TOPIC_A = "ha/util/sumpON";`

You want to first edit your config/configuration.yaml to add a (virtual) sensor for
each of your topics.  The value of those (virtual) sensors will be the payload. For instance:

```
sensor:                            
  - platform: mqtt                 
    name: "temperature"           
    state_topic: "your/label/scheme/temp" 
    scan_interval: 20                
    unit_of_measurement: 'F'     
  - platform: mqtt
    name: "FluxCapCharge"
    state_topic: "your/label/scheme/fluxCapacitorCharge"
    scan_interval: 20
    unit_of_measurement: 'GW'
...and so on...
```


You can trigger an automation by watching for a particular topic. The yaml code for the trigger within your auomation looks like this:

```
trigger:
- platform: mqtt
  topic: ha/util/sumpON
```

Though you can configure the trigger via the UI - select Trigger type "MQTT" and
specify your topic (ignore the optional payload field).

## Graphing stuff

HA will create a sensor entity associated with the values you pass along with these topics.  You can use these to graph the sensor values you're using to determine the state of your devices (in my case, water heater, sump, and hvac fan ~= heating or cooling).

In the HA dashboard I am using sensor cards to graph these, and if you want to get a nice, (more) detailed line graph by clicking on the card you want to specify the units (optional) in the card config.

For example, here is the code for my sensor card watching the sump current:

```
entity: sensor.sump_current
graph: line
name: Sump Current
type: sensor
hours_to_show: 12
unit: mA
```

I mostly track on/off for these utilities so I get a nice timeline that tells me how often things are running (or not).  For the dashboard I use a history graph card.
