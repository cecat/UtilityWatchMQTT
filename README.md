# UtilWatch2020
A [Particle Photon](https://docs.particle.io/photon/)-based utility monitoring system, [Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch), this one using MQTT to connect with
[Home Assistant (HASS)](https://www.home-assistant.io/). The code has been tested on
the [Particle Electron](https://docs.particle.io/electron) (cellular) but you may wish to throttle
back on the messaging frequency to manage cellular costs.

There are some tips for setting up webhooks/thingspeak at the
[Photon-Util-Watch](https://github.com/cecat/Photon-Util-Watch) repo.

## Contents

**DEV/** - device info - what hardware I use for the different sensors.

**HASS/** - how to integrate this with Home Assistant and the Mosquitto MQTT broker integration.

**src/** - code

**project.properties** - version numbers of the libraries at last update

