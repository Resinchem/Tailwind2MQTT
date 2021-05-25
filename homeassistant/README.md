## Home Asssistant Sample Files

These files are provided as examples on how to issue commands and create state sensors.

**Be sure to update the MQTT topics in these samples:**

Replace ```MQTT_TOPIC_PUB``` and ```MQTT_TOPIC_SUB``` with the topics you defined in the Settings.h files.

The Lovelace example just shows how to create a button to open/close the door and show the state of the door.  
![tailwind_lovelace_closed_new](https://user-images.githubusercontent.com/55962781/119518115-3c2eea00-bd46-11eb-8e3d-beafb1837a4f.jpg)

![tailwind_lovelace_open_new](https://user-images.githubusercontent.com/55962781/119518228-57015e80-bd46-11eb-9f89-ce5abce41ad1.jpg)

Note that this example does make use of two custom components from HACS:
* [Custom Button Card](https://github.com/custom-cards/button-card)
* [Custom Text Divider Row](https://github.com/iantrich/text-divider-row)

See the [Wiki](https://github.com/Resinchem/Tailwind2MQTT/wiki/MQTT-%5C-Home-Assistant) for more details.
