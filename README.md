# UtilWatch2020
Fresh version of a [Particle Photon](https://docs.particle.io/photon/)-based utility monitoring system, [Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch), this one using MQTT to connect with
the (fabulous open source) [Home Assistant](https://www.home-assistant.io/) running on a [Raspberry Pi 4](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/).

12-October-2020  

This code monitors home utilities (sump pump, hvac, water heater) using a Particle Photon.
It uses MQTT to send data to Home Assistant.
The code is also still here to use Particle's Webhooks to graph things at Thingspeak.com as 
the previous version did.
There are some tips for setting up webhooks/thingspeak at the
[Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch) repo.

## Contents

If you just take src/UtilWatch2020.ino and paste that code into the Particle.io's [Web IDE](https://build.particle.io/build/) where you can then
add the libraries (MQTT, OneWire, DS18B20) and decide whether to use a secrets.h
file or fill in mqtt username, password, and server info in the designated spot 
(and comment out the #include line for secrets.h) in src/UtilWatch2020.ino.  The secrets.h
file should look like this:

```
const char *HA_USR = "your mqtt username";
const char *HA_PWD = "your mqtt passwd";
// and use one of the following lines:
//char MY_SERVER[] = "your.server.hostname"
	// or using your IP address w.x.y.z
byte MY_SERVER[] = { w, x, y, z };
```

**DEV/** - device info - what hardware I use for the different sensors.

**HASS/** - how to integrate this with Home Assistant and the Mosquitto MQTT broker integration.

**src/** - code

**project.properties** - version numbers of the libraries at last update

