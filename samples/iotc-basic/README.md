#### IoTConnect SDK Basic Sample

This sample shows how to make use of the IoTConnect SDK to connect your DK, Thingy:91 or AVT9152-EVB devices to IoTConnect.

For more information, see the README.md in ../../


### Led Color (Thingy:91) and Button Behavior

If you running on Thingy:91 the LED colors will reflect the current state of the application:

* Yellow - Application startup, MQTT connecting, or FOTA in progress 
* Purple - Acquiring HTTP discovery response during SDK startup 
* Green - MQTT Connected 
* Red - SDK Disconnecting

Pressing the button will cycle the application between disconnecting the MQTT connection and bring ther modem offline and
running the application.
