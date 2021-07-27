#### IoTConnect GPS SDK Sample

This sample is based on the basic sample application and includes sensors and GPS support.

Following are the sensors used depending on the target device:
* for DK: SENSOR_SIM
* for Thingy:91: ADXL362, BH1749, BME680
* for AVT9152-EVB: LIS2DH, LPS22HB, TE23142771

For more information, see the README.md in ../iotc-basic

### LED Color (Thingy 91) and Button Behavior

In addition to the LED states and button behavior described in the basic example, long-pressing the button will disconnect
MQTT and attempt to acquire a GPS fix. Once the fix is obtained, the LED color will be blue briefly and reconnect MQTT.

Short or long pressing the button will stop the GPS search and reconnect the application to the cloud.

If GPS coordinates are acquired, they will be recorded and sent in telemetry until the unit is powered cycled. 