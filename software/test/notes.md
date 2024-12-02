# Software Notes

For the pressure sensors, I seem to like [Rob Tillaart's library](https://github.com/RobTillaart/HX710AB). 

Pressure sensor requires two digital pins, one is in and the other is out. The clk pin is cycled to get data out. There is no bus.

Each sensor requires two pins. This device will only need one, so thats fine.

May need to take a few calibration readings while starting up. 

Sensor starts returning some 43..... arbitrary high value after i was blowing in it a few times. Another sensor seems stuck on infinitely negative value. May be broken. May have been due to using the syringe. How high is 40kpa? That is about 5 psi. It is very likely i broke it with the syringe.

### Capacitive touch

Note, needs 3.3v. This machine also seems to need some calibration. Setting constants manually seemed to work. Needs to know wire lengths and such though.

Address can be set, this is on the I2C bus. See docs in the wiring diagram

