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

##Old School##

$10 [Small breadboard](https://www.amazon.com/DEYUE-breadboard-Set-Prototype-Board/dp/B07LFD4LT6/ref=sr_1_3?dchild=1&keywords=small+breadboard&qid=1602615796&sr=8-3)
(they are actually about $1-2 but not sold individually so you would typically buy a handful)

For the above, you might also consider a [kit like this](https://www.verical.com/pd/particle-industries-----misc-kits-and-tools-photonkit-3439409) that includes Photon and breadboard)

$9 [OneWire Temperature Sensor](https://www.amazon.com/DS18B20-Temperature-Waterproof-Stainless-Raspberry/dp/B087JQ6MCP/ref=sr_1_3?dchild=1&keywords=ds18b20&qid=1602616146&sr=8-3) - this one includes a little breakout board with the pull-up resistor so is an easy on ramp.  The sensors themselves are $5-6 (cheaper for just the sensor without the wire or waterproof packaging) 
but they are typically [not sold individually](https://www.amazon.com/Gikfun-DS18B20-Temperature-Waterproof-EK1083x3/dp/B012C597T0/ref=sr_1_5?dchild=1&keywords=ds18b20&qid=1602363368&sr=8-5). 

$16 Hall Effect current sensor- [ModernDevice](https://moderndevice.com/product/current-sensor/).
This is pretty expensive as sensors go but it's convenient in that
you just zip-tie to the wire, no cutting into dangerous AC wiring..
There are lower cost sensors that clamp on [like these](https://www.amazon.com/SCT-013-000-Non-invasive-Current-Sensor-Transformer/dp/B07FZZZ62L/ref=sr_1_4?dchild=1&keywords=Current+Sensor&qid=1602619691&sr=8-4)
that are worth a try, but for this project I would avoid any that require you to
(cut the wire and) place them in-line like [these](https://www.amazon.com/Gikfun-Current-Sensor-Arduino-EK1181x2/dp/B00RBHOLUU/ref=sr_1_3?dchild=1&keywords=Current+Sensor&qid=1602619691&sr=8-3).

$6 [3.3v power supply](https://www.amazon.com/3-3V-Adapter-Power-5-5-2-1/dp/B07BGW2VXV/ref=sr_1_3?dchild=1&keywords=3.3v+power+supply&qid=1602616592&sr=8-3) if your furnace has an outlet that only activates when the fan is on (saving you having to build a step-down circuit or use a voltage regulator).


