# UtilWatch2020

If you are not doing much with Particle devices the quickest path is to copy/paste *UtilWatch2020.ino*
into the Particle.io
[Web IDE](https://build.particle.io/build/) and then *add the libraries* (MQTT, OneWire, DS18B20).
You can use a secrets.h file
(see *template_secrets.h*) or copy/paste *template_secrets.h* into your code.
In either case you need to fill in your particulars.

## Contents

**UtilWatch2020.ino** - Sketch that runs on the Photon.

**template_secrets.h** - Use to create a *secrets.h* file to hold your mqtt username, password, server address.

**topics.h** - If you have lots of topics this keeps them from cluttering your code.

**ThingSpeakReport.h** - This is code you can use if you prefer to send your data to ThingSpeak.com
(An alternative if you just want to monitor things and don't run HASS).

## Change log

*12-October-2020:* Monitor home utilities (sump pump, hvac, water heater...) using a Particle Photon.
It uses MQTT to send data to HASS. Using QoS=0 (i.e., none) is simplest and seems
to work fine unless you are trying to send from a cellular-connected Electron with weak signal
(where I've had a bit of trouble with lost messages).

*14-December-2020:* The code has been simplified, removing lots of delays and debug assist code. The long list of mqtt topics is now pushed out to topics.h. Added template_secrets.h for mqtt user/passwd/server info.
Code to use Particle's Webhooks to graph things at Thingspeak.com removed; saved in the src directory for future use.

*23-Jan-2021:* Fixed intermittent wedging triggered in tellHASS(). Photon will wedge if you try to publish after client.connect fails.
