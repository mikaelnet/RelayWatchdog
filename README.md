# RelayWatchdog
Watchdog for physical devices, such as routers, switches cameras etc

This is a simple watchdog system that I'm using on a remote location
to ensure the router, wifi etc is always online and gets a power cycle
reset if any device stops working.

To simplify things, I used a four channel relay board, but virtually 
any relays will do. An ATmega328 is used as a control chip, but a 
smaller one will do just fine as well, or an Arduino. 

The idea is that a Raspberry PI is testing the functionality of the 
surrounding devices and toggles a pin for each device that is operating
normally. If a pin isn't toggled within a certain period, the MPU 
power cycles the device by pulling the relay. 

In my setup, the RPI pings the router and toggles channel 2. It also
connects to an external site to check network connectivity. If it's
lost, it represents channel 1, the fiber media converter. Channel 3 is 
the RPI itself and channel 4 is a camera.

If no watchdog reset signals are recieved at all from the RPI, the 
MPU performs a complete power cycle, by resetting the media converter,
wait for a minute, then does the same for the router, waits a minute,
resets the RPI and so on.
