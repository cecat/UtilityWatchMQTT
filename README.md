# UtilWatch2020
A [Particle Photon](https://docs.particle.io/photon/)-based utility monitoring system, [Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch), this one using MQTT to connect with
[Home Assistant (HASS)](https://www.home-assistant.io/). The code has been tested on
the [Particle Electron](https://docs.particle.io/electron) (cellular) but you may wish to throttle
back on the messaging frequency to manage cellular costs.

There are some tips for setting up webhooks/thingspeak at the
[Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch) repo.

## Contents

If you are not doing much with Particle devices the quickest path is to copy/paste src/UtilWatch2020.ino
into the Particle.io
[Web IDE](https://build.particle.io/build/) and then
add the libraries (MQTT, OneWire, DS18B20). You can use a secrets.h file 
(see src/template_secrets.h) or copy/paste template_secrets.h into your code.
In either case you need to fill in your particulars.

**DEV/** - device info - what hardware I use for the different sensors.

**HASS/** - how to integrate this with Home Assistant and the Mosquitto MQTT broker integration.

**src/** - code

**project.properties** - version numbers of the libraries at last update

## Change log

*12-October-2020:* Monitor home utilities (sump pump, hvac, water heater...) using a Particle Photon.
It uses MQTT to send data to HASS. Using QoS=0 (i.e., none) is simplest and seems
to work fine unless you are trying to send from a cellular-connected Electron with weak signal
(where I've had a bit of trouble with lost messages).

*14-December-2020:* The code has been simplified, removing lots of delays and debug assist code. The long list of mqtt topics is now pushed out to topics.h. Added template_secrets.h for mqtt user/passwd/server info.
Code to use Particle's Webhooks to graph things at Thingspeak.com removed; saved in the src directory for future use.

*23-Jan-2021:* Fixed intermittent wedging triggered in tellHASS(). Photon will wedge if you try to publish after client.connect fails.
