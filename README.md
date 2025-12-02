# UtilWatch2020
A [Particle Photon](https://docs.particle.io/photon/)-based utility monitoring system, [Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch), this one using MQTT to connect with
[Home Assistant (HASS)](https://www.home-assistant.io/). The code has been tested on
the [Particle Electron](https://docs.particle.io/electron) (cellular) but you may wish to throttle
back on the messaging frequency to manage cellular costs.

A previous version of this code - in the
[Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch) repo - used ThingSpeak as a
dashboard, and tips can be found there if you are just looking for a dashboard rather than
integrating into other systems.  This version uses MQTT to report to Home Assistant.

## Contents

**DEV/** - device info - what hardware I use for the different sensors.

**HASS/** - how to integrate this with Home Assistant and the Mosquitto MQTT broker integration.

**src/** - code

**project.properties** - version numbers of the libraries at last update

