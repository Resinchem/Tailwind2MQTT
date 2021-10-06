![tailwind2mqtt_logo](https://user-images.githubusercontent.com/55962781/119289900-80709c00-bc19-11eb-8ef0-04480b86ff2c.jpg)

Bridge providing an MQTT interface to Tailwind iQ3 Smart Garage Opener

### Update: Tailwind can also be integrated via the HomeKit Controller integration in Home Assistant, whether you have HomeKit or not.

See [Using HomeKit Controller Integration](https://github.com/Resinchem/Tailwind2MQTT/wiki/Using-HomeKit-Controller-Integration) in the wiki for an alternative to using this bridge.

### Second Update:  A native Home Assistant integration is now available via HACS, developed by pauln.  

See [tailwind-home-assistant](https://github.com/pauln/tailwind-home-assistant) for more details.

Due to these additional integration options that no longer require additional hardware, there will likely not be any future development of this bridge.

# IMPORTANT:  THIS BRIDGE IS STILL IN ALPHA
### *Your Tailwind device must be running the beta firmware v9.87 or later*

## Now supports Home Assistant MQTT Discovery!!

### Please see the [Wiki](https://github.com/Resinchem/Tailwind2MQTT/wiki) for information on installation, configuration and use of the bridge.

As with most alpha versions, you should expect frequent and possible breaking changes.  Use at your own risk. It is recommended that you do not rely solely on this bridge for your garage door control and remove or disable the current Tailwind app.  Use care in automations to avoid unplanned door openings.  The author claims no liability for damage to your devices, doors or unintended access to your residence or business via use of the bridge.

**You must have a properly configured MQTT broker for this bridge to work and Home Assistant must be configured to talk to the MQTT broker.

### Please use the [Discussions](https://github.com/Resinchem/Tailwind2MQTT/discussions) to provide feedback, ideas, feature requests or questions.

This bridge is not officially endorsed nor supported by Tailwind.
