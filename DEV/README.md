# Hardware

This project uses a Particle Photon with three sensors attached, each giving me state information about one of my main utilities - sump pump, water heater, and hvac fan (~= heating or cooling).  If you use a Particle Electron you will want to disable the variables  (Particle.variable() lines in the code) as these are continually synchronized with the Particle cloud, unnecessarilyusing up lots of cel bandwidth.

I keep a LiPo battery plugged into the Photon so it won't die if the power goes out. You might also
check out the [parts list](https://github.com/cecat/UtilityWatchMQTT/blob/main/DEV/parts.md).
I happen to have a bunch of parts already and am set up to solder stuff at a moment's notice, so
it was easier for me to quickly make stuff.  But really for this project you just need to do
a bit of soldering for connectors, and most of the parts are easy to order, albeit a bit more
expensive as some of them are only available in batches of 6 or 10 units.


You can see in the src/UtilWatch2020.ino code that I have the sensor data pins connected to pins d0, a0, and a1:

```
// sensor pins (what pins on the Photon go to what sensors)
#define waterPin    D0                      // pin for ds18b20-A to water heater chimney
#define hvacPin     A0                      // pin for HVAC fan current sensor
#define sumpPin     A1                      // pin for sump pump current sensor
```

## Water Heater

I use a one-wire DS18B20 temperature sensor [like these](https://www.amazon.com/Gikfun-DS18B20-Temperature-Waterproof-EK1083x3/dp/B012C597T0/ref=sr_1_5?dchild=1&keywords=ds18b20&qid=1602363368&sr=8-5), duct-taped to the top of my (gas) water heater, sticking into the open space between the water heater and the chimney.  If your water heater is electric this won't work so you'll need to monitor the current (as I do for the sump).

Many people (and the example code in the library) use analog pins for VCC and GND (setting both to output mode and then setting VCC to HIGH and GND to LOW) but I use the native VCC and GND pins on the Photon.  Note for the sensors in the link above you need to add a [pull-up resistor](https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806)
between VCC and DATA, though some claim this is [unnecessary](https://wp.josh.com/2014/06/23/no-external-pull-up-needed-for-ds18b20-temp-sensor/).

## Sump Pump

When I first implemented this system I used an accelerometer on the sump discharge pipe, sampling it at like 16x/second and looking for motion, which was fun.  But now I use a Hall Effect current sensor - I found a nice one from [ModernDevice](https://moderndevice.com/product/current-sensor/) that zip-ties to the power cord. 

The advantage to the vibration-based sensor is that if you have a battery backup pump the sensor will detect that one as well as the main.  This is how I figured out once that my main had failed (it was running more frequently than normal since the battery backup was smaller).  I don't have a battery backup hooked up at the moment.  But I do track how many times the sump runs, and for my situation I have figured out that I should be worried when the sump runs more than about 7 times in 30 minutes, thus I track that and send it to HA where I have an automation that texts me when the sump is running that often.

At some point it would be nice to add some logic to determine that the sump is out (vs. just inactive because it's not needed), such as watching the frequency and detecting that it suddenly goes from running a lot to not running.

## HVAC Fan

On the side of my furnace is an outlet that's only powered when the fan is on (I think this is
quite common, for instance to plug in a humidifier).  I grabbed an old 9v wall wart and stepped
it down to 4v with a couple of resistors
[here's a nice explanation](http://www.learningaboutelectronics.com/Articles/How-to-reduce-voltage-with-resistors.php).
If you have a spare voltage regulator (e.g., one of [these](https://www.amazon.com/6-Pcs-STMicroelectronics-LD1117V33-Voltage-Regulator/dp/B01MQF7D9D/ref=sr_1_4?dchild=1&keywords=voltage+regulator+3.3v&qid=1602614666&sr=8-4)
laying around you can of course use that.
This is connected to my chosen pin and is 0v when the fan is off, then 4v when it turns on (or 3.3v if you use a standard voltage regulator).

