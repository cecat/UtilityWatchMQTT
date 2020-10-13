# UtilWatch2020
Fresh version of a [Particle Photon](https://docs.particle.io/photon/)-based utility monitoring system, [Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch), this one using MQTT to connect with Home Assistant

12-October-2020  

This code monitors home utilities (sump pump, hvac, water heater) using a Particle Photon.
It uses Particle.publish() to send data to Home Assistant via MQTT.
The code is still here to use Particle's Webhooks to graph things at Thingspeak.com as well.
There are some tips for setting up webhooks/thingspeak in the 
[Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch) README file.

## Contents

If you just take src/UtilWatch2020.ino and paste that code into the Particle.io's [Web IDE](https://build.particle.io/build/) where you can then
add the libraries (MQTT, OneWire, DS18B20) and it should be good to go.
Unless you intend to use git to track your code that's the most straighforward route. 

**DEV/** - device info - what hardware I use for the different sensors.

**HASS/** - how to integrate this with Home Assistant and the Mosquitto MQTT broker integration.

**src/** - code

**project.properties** - version numbers of the libraries at last update

