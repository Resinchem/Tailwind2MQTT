## Home Asssistant Sample Files

These files are provided as examples on how to issue commands and create state sensors.

### If you have enabled Home Assistant MQTT Discovery in the settings, you do not need to create the sensors!
MQTT Discovery will automatically create the following sensors in your Home Assistant upon boot of the bridge:
Entity Name|Reported Values|
-----------|---------------|
```binary_sensor.tailwind_door1```|Open or Closed|
```binary_sensor.tailwind_door2```|Open or Closed|
```binary_sensor.tailwind_door3```|Open or Closed|
```sensor.tailwind_mqtt_status```|'connected' if connected to MQTT|
```sensor.tailwind_last_status```|OK, FAILED or INVALID|
```sensor.tailwind_last_result```|0-7 - Last status code from API|

Note: Any unused doors (not connected) will always return a state of 'Open'. See the [Wiki](https://github.com/Resinchem/Tailwind2MQTT/wiki/MQTT-%5C-Home-Assistant) for more info on MQTT payloads.

**Be sure to update the MQTT topics in these samples:**

Replace ```MQTT_TOPIC_PUB``` and ```MQTT_TOPIC_SUB``` with the topics you defined in the Settings.h files.

The Lovelace example just shows how to create a button to open/close the door and show the state of the door.  
![tailwind_lovelace_closed_new](https://user-images.githubusercontent.com/55962781/119518115-3c2eea00-bd46-11eb-8e3d-beafb1837a4f.jpg)

![tailwind_lovelace_open_new](https://user-images.githubusercontent.com/55962781/119518228-57015e80-bd46-11eb-9f89-ce5abce41ad1.jpg)

Note that this example does make use of two custom components from HACS:
* [Custom Button Card](https://github.com/custom-cards/button-card)
* [Custom Text Divider Row](https://github.com/iantrich/text-divider-row)

See the [Wiki](https://github.com/Resinchem/Tailwind2MQTT/wiki/MQTT-%5C-Home-Assistant) for more details.
