# HomeSpan Sun Simulator Wake Up Light

This project uses an ESP32, NeoPixels (or similar addressable LED strips), and HomeSpan to create a HomeKit enabled sun simulation wake up light. In Home, two accessories are added where the first one controls the LED strip as any other RGB enabled light, and the other is an on/off switch if the sun simulation should be active or not. When the sun simulation switch is toggled, nothing visible happens to the LED strip until half an hour before the set alarm time.

## Setting an Alarm

Since HomeKit does not have an easy to use way of slowly dimming a light on over the course of half an hour, this is handled by the ESP32. Due to this, the desired alarm time must be set in the ESP32 using other means. In this project, the ESP32 accepts HTTP GET requests with the alarm time as a parameter in the URL, for example visiting the following URL (with the IP address of the ESP32) would the alarm to 07:30, with the sun simulation starting at 07:00:

```
192.168.1.32/set?time=07:30
```

An iOS Shortcut has been created to simplify the process. When the shortcut is run, it will ask for a desired wake up time and then set both an alarm in the iOS alarm app and send an HTTP GET request to the ESP32. The shortcut is available on the following link:

[https://www.icloud.com/shortcuts/2e953cb21c0745d8855f180ee2dba349](https://www.icloud.com/shortcuts/2e953cb21c0745d8855f180ee2dba349)

Even if the ESP32 is doing the sun simulation by itself, all LED strip values are being reported back to Home so the Home app always show the current state of the accessory even when the simulation is running. When the simulation is done (at the set alarm time), the Sun Simulation accessory is automatically turned off.

## Running the project

This project is based on the great [HomeSpan](https://github.com/HomeSpan/HomeSpan) library and would not have been possible without it.

Clone or download this project, run Arduino IDE, and install the ESP32 boards by Espressif and the HomeSpan library by Gregg Berman. Set the pin your LED strip is connected to, and set your time zone according to the Proleptic Format for TZ (see [https://sourceware.org/glibc/manual/latest/html_node/Proleptic-TZ.html](https://sourceware.org/glibc/manual/latest/html_node/Proleptic-TZ.html) for examples).

## Known Limitations

- In the current version, alarms set between 00:00 and 00:30 will never trigger the sun simulation.
- The strip can be controlled manually while the simulation is running, however any manual changes will be overridden every new minute for as long as the simulation is running (until the alarm time). The simulation can be disabled at any time by toggling off the "Sun Simulation" accessory in the Home app.
- There is currently no verification if the entered alarm time is a real time. For example if the value "25:73" is entered as the desired alarm time via URL, the system will fail. So for the moment keep to real time values when using URL. If using the Shortcut to set time, this becomes a non-issue as the alarm time is selected from a time picker.

## Further Development

As this was developed as a simple home project, no further improvements or changes are planned or committed to. Feel free to download and modify to your own needs.
