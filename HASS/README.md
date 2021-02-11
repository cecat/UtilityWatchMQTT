# Integrating with Home Assistant

## MQTT
Add and set up *MQTT Mosquitto broker* integration in HASS following its
[documentation](https://github.com/home-assistant/hassio-addons/blob/master/mosquitto/DOCS.md).
This will include creating an MQTT *username* and *password* --- which will go into the secrets.h
file (../README.md) or alternately the contents of secrets.h can be put into UtilWatch2020.ino.
There you also specify your HASS server (i.e. the MQTT broker) by
hostname or IP address.  If the latter, make a DNS reservation for your HASS server
so it always gets the same IP address. Finally, for each device you want to connect to HASS
you need to have a unique CLIENT_NAME or the broker will get confused.

## TOPICS and Automation

Each MQTT message has a *topic* that creates a sensor variable and is used for HASS automation triggers.
I have so many that I pulled them out and created a topics.h file (src/topics.h). You can just put these
into UtilWatch2020.ino using this form:

`const char *TOPIC_A = "ha/util/sumpON";`

To use these you must edit your HASS *config/configuration.yaml* file to add a (virtual) sensor for
each topic.  The value of those (virtual) sensors will be taken from the mqtt payload. Example:

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

This example above defines two HASS entities *sensor.temperature* and *sensor.FluxCapCharge.*
When HASS receives a mqtt message with topic *your/label/scheme/temp* it updates sensor.temperature
with the value you sent in the HASS message (see *tellHASS()* routine in *src/UtilWatch2020.ino*).

In the above example, the *scan_interval* and *unit_of_measurement* are optional, but if you want a nice graph
in your HASS dashboard you need to specify *unit_of_measurement* here. (see Graphing stuff below)
Without the *unit_of_measurement* you will see a bar-graph type display in HASS (if you add a panel
to your dashboard) with different colors horizontally as the variable changes (not very
useful).  With *unit_of_measurement* you'll get a graph.

You can also trigger an automation by watching for a particular topic. The yaml code for
the trigger within your auomation looks like this:

```
trigger:
- platform: mqtt
  topic: ha/util/sumpON
```

You can configure the trigger via the UI - select Trigger type "MQTT" and
specify your topic (ignore the optional payload field).

## Graphing stuff

HASS will create a sensor entity associated with the values you pass along with these
topics.  You can use these to graph the sensor values you're using to determine
the state of your devices (in my case, water heater, sump, and hvac fan ~= heating
or cooling).

In the HASS dashboard I am using sensor cards to graph these, and if you want to
get a nice, (more) detailed line graph by clicking on the card you want to specify
a unit_of_measurement in your configuration.yaml file as noted above.

For example, here is the code for my sensor card watching the sump current:

```
entity: sensor.sump_current
graph: line
name: Sump Current
type: sensor
hours_to_show: 12
unit: mA
```
I mostly track on/off for these utilities so I get a nice timeline that tells
me how often things are running (or not).  A history graph card works nicely for this
in the dashboard.
