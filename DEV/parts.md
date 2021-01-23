# Parts List

If you already do stuff with electronics you probably have many of the parts sitting around.
Below is a parts list optimized for someone who maybe just has a soldering iron but wants to
mostly plug and play. In this case you're looking at about $50-60 in parts.
In several cases below you have to buy multiple pieces so this
artificially inflates the cost. (I really really miss Radio Shack) There are
electronics sites on the web that do sell things in units rather than batches, but these
also typically charge a lot for shipping so for a one-off project like this you'd
pay more there once you account for shipping.

The reason you need to solder is that you want a wire to get from the thing you are
sensing to your Photon, ideally staying below about 6' of wire (old telephone wire is handy).
That means either soldering wires to terminals or putting connectors on them.

$20 [Particle Photon (WiFi)](https://store.particle.io/collections/wifi/products/photon)
(buy with headers so you can plug into a breadboard)

$18 [Grove Photon Shield](https://www.amazon.com/gp/product/B071LCPX7P/ref=ppx_yo_dt_b_search_asin_title)

You can go with pre-packaged Grove sensors but typically the grove connectors are only a few inches long
One way or another you need to make longer wires to reach your furnace, sump pump, water heater, etc. 
That being the case, you'll solder the sensors below to grove 4-wire connectors.

$13 [OneWire Temperature Sensor](https://www.amazon.com/IZOKEE-Temperature-Stainless-Waterproof-Resistor/dp/B082WVWC3T/ref=sr_1_4). 
This one includes pull-up resistors so is an easy on-ramp.
The sensors themselves are $5-6 (cheaper for just the sensor without the wire or waterproof packaging) 
but they are not typically sold individually.

$16 Hall Effect current sensor- [ModernDevice](https://moderndevice.com/product/current-sensor/).
This is pretty expensive as sensors go but it's convenient in that
you just zip-tie to the wire, no cutting into dangerous AC wiring.

$6 [3.3v power supply](https://www.amazon.com/3-3V-Adapter-Power-5-5-2-1/dp/B07BGW2VXV/ref=sr_1_3) if your furnace has an outlet that only activates when the fan is on (saving you having to build a step-down circuit or use a voltage regulator).

