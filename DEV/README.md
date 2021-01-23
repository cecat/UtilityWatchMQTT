# Hardware

This project uses a *Particle Photon* with four sensors attached, each providing state information about
one of my main utilities - sump pump, water heater, and hvac fan (~= heating or cooling), and the ambient
temperature of the basement.  If you use a *Particle Electron* you may want to disable the variables
(*Particle.variable()* lines in the code) as these are continually synchronized with the Particle cloud,
unnecessarily using up your cel budget.

The [parts list](https://github.com/cecat/UtilityWatchMQTT/blob/main/DEV/parts.md) contains what I
am using.  You'll need to a bit of soldering for connectors, and most of the parts are
easy to order, albeit a bit more expensive if you don't already have a cache of parts from other projects,
as some of them are only available in batches of 6 or 10 units.

## Sensing each Appliance

The explanations below are for direct-wiring your sensors into the Photon, which I did originally. More
recently I switched to a
[Grove sensor shield](https://www.amazon.com/gp/product/B071LCPX7P/ref=ppx_yo_dt_b_search_asin_title),
so the sensor wiring below is applicable to giving them a standard 4-wire Grove connector. The 
advantage to using a shield vs. soldering stuff is that you can readily swap out the Photon, or 
any of the sensors. I would recommend this over soldering things to your photon or, as I did,
soldering the Photon and connectors to a strip-board. [All about Grove](https://wiki.seeedstudio.com/Grove_System/).

### Water Heater

I use a one-wire *DS18B20* temperature sensor
[like these](https://www.amazon.com/Gikfun-DS18B20-Temperature-Waterproof-EK1083x3/dp/B012C597T0/ref=sr_1_5),
duct-taped to the top of my (gas) water heater, sticking into the open space between the
water heater and the chimney.  If your water heater is electric this won't work so you'll
need to monitor the current (See Sump Pump below).

Many people (and the example code in the library) use analog pins for VCC and GND (setting both to
output mode and then setting VCC to HIGH and GND to LOW) but I am using the native VCC and GND pins
on the Photon.  Note for the sensors in the link above you need to add a
[pull-up resistor](https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806)
between VCC and DATA, though [some claim this is unnecessary](https://wp.josh.com/2014/06/23/no-external-pull-up-needed-for-ds18b20-temp-sensor/).

I use a second *DS18B20* to track the basement temperature because, why not.

### Sump Pump

When I first implemented this system I used an accelerometer on the sump discharge pipe,
sampling it at like 16x/second and looking for motion, which was fun.  But now I use
a [Hall Effect current sensor](https://moderndevice.com/product/current-sensor/)
that zip-ties to the power cord. 

### HVAC Fan

On the side of my furnace is an outlet that's only powered when the fan is on (I think this is
quite common, for instance to plug in a humidifier).  The easiest thing to do here is buy a 3.3v
DC wall wart, cut the wire, and solder it onto a Grove connector with the negative wire to GND (pin4/black)
and the positive wire to the primary signal wire (pin1/yellow).  I had an old 9v wall wart and stepped
it down to 4v with a couple of resistors
[(like this)](http://www.learningaboutelectronics.com/Articles/How-to-reduce-voltage-with-resistors.php).

## Connecting to the Photon

You can see in *src/UtilWatch2020.ino* that you need to specify pin assignments:

```
// sensor pins (what pins on the Photon go to what sensors)
#define waterPin    D2                      // pin for ds18b20-A to water heater chimney
#define hvacPin     A0                      // pin for HVAC fan current sensor
#define sumpPin     A4                      // pin for sump pump current sensor
```

Whether you wire directly or use the Grove shield just make sure you update the pin assignments to the ones 
you select.

