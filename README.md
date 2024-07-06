# esp8266_p1meter

Software for the ESP2866 that sends P1 smart meter data to an mqtt broker (with OTA firmware updates)

## about this fork
This fork (tries) to add support for the `Landys and Gyr E360` smartmeter (DSMR5.0)

The ![original source](https://github.com/fliphess/esp8266_p1meter) has issues with DSMR5.0 meters who like to send telegrams every 1 second at a high 115200 baud rate. 
This causes the used SoftwareSerial to struggle to keep up and thus only receives corrupted messages. This fork switches to using the main Hardware serial port (RX) for communication with the meter.

# Getting started
This setup requires:
- An esp8266 (nodeMcu and Wemos d1 mini have been tested)
- A 10k ohm resistor
- A 4 pin RJ11 or [6 pin RJ12 cable](https://www.tinytronics.nl/shop/nl/kabels/adapters/rj12-naar-6-pins-dupont-jumper-adapter) Both cables work great, but a 6 pin cable can also power the esp8266 on most DSMR5+ meters.

Compiling up using Arduino IDE:
- Ensure you have selected the right board
- Using the Tools->Manage Libraries... install `PubSubClient` and `IotWebConf`
- Flash the software

Compiling up using PlatformIO:
- Ensure the correct board type is selected in project configuration
- Upload the software.

Finishing off:
- You should now see a new wifi network `ESP******` connect to this wifi network, a popup should appear, else manually navigate to `192.168.4.1`
- Configure your wifi and Mqtt settings
- To check if everything is up and running you can listen to the MQTT topic `hass/status`, on startup a single message is sent.

## Connecting to the P1 meter
Connect the esp8266 to an RJ11 cable/connector following the diagram.

| P1 pin   | ESP8266 Pin |
| ----     | ---- |
| 2 - RTS  | 3.3v |
| 3 - GND  | GND  |
| 4 -      |      |
| 5 - RXD (data) | RX (gpio3) |

On most Landys and Gyr models a 10K resistor should be used between the ESP's 3.3v and the p1's DATA (RXD) pin. Many howto's mention RTS requires 5V (VIN) to activate the P1 port, but for me 3V3 suffices.

![Wiring](https://raw.githubusercontent.com/daniel-jong/esp8266_p1meter/master/assets/esp8266_p1meter_bb.png)

### Optional: Powering the esp8266 using your DSMR5+ meter 
<details><summary>Expand to see wiring description</summary>
<p>
  
When using a 6 pin cable you can use the power source provided by the meter.
  
| P1 pin   | ESP8266 Pin |
| ----     | ---- |
| 1 - 5v out | 5v or Vin |
| 2 - RTS  | 3.3v |
| 3 - GND  | GND  |
| 4 -      |      |
| 5 - RXD (data) | RX (gpio3) |
| 6 - GND  | GND  |

![Wiring powered by meter](https://raw.githubusercontent.com/daniel-jong/esp8266_p1meter/master/assets/esp8266_p1meter_bb_PoweredByMeter.png)

</p>
</details>

## Data Sent

All metrics are send to their own MQTT topic.
The software sends out to the following MQTT topics:

```
sensors/power/p1meter/consumption_low_tarif 2209397
sensors/power/p1meter/consumption_high_tarif 1964962
sensors/power/p1meter/returndelivery_low_tarif 2209397
sensors/power/p1meter/returndelivery_high_tarif 1964962
sensors/power/p1meter/actual_consumption 313
sensors/power/p1meter/actual_returndelivery 0
sensors/power/p1meter/l1_instant_power_usage 313
sensors/power/p1meter/l2_instant_power_usage 0
sensors/power/p1meter/l3_instant_power_usage 0
sensors/power/p1meter/l1_instant_power_current 1000
sensors/power/p1meter/l2_instant_power_current 0
sensors/power/p1meter/l3_instant_power_current 0
sensors/power/p1meter/l1_voltage 233
sensors/power/p1meter/l2_voltage 0
sensors/power/p1meter/l3_voltage 0
sensors/power/p1meter/gas_meter_m3 968922
sensors/power/p1meter/actual_tarif_group 2
sensors/power/p1meter/short_power_outages 3
sensors/power/p1meter/long_power_outages 1
sensors/power/p1meter/short_power_drops 0
sensors/power/p1meter/short_power_peaks 0
```

## Home Assistant Configuration

Use this [example](https://raw.githubusercontent.com/daniel-jong/esp8266_p1meter/master/assets/p1_sensors.yaml) for home assistant's `sensor.yaml`

The automatons are yours to create.
And always remember that sending alerts in case of a power outtage only make sense when you own a UPS battery :)

## Thanks to

This sketch is mostly copied and pasted from several other projects.
Standing on the heads of giants, big thanks and great respect to the writers and/or creators of:

- https://github.com/jantenhove/P1-Meter-ESP8266
- https://github.com/neographikal/P1-Meter-ESP8266-MQTT
- http://gejanssen.com/howto/Slimme-meter-uitlezen/
- https://github.com/rroethof/p1reader/
- http://romix.macuser.nl/software.html
- http://blog.regout.info/category/slimmeter/
- http://domoticx.com/p1-poort-slimme-meter-hardware/
