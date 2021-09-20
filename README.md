#### IoTConnect nRF SDK

This repository contains IoTConnect nRF SDK and samples intended for use with Avnet's IoTConnect platform.

The sample shows how to make use of the IoTConnect SDK to connect your DK, Thingy:91 or AVT9152-EVB devices to IoTConnect.

### Building

* Install the nRF SDK v1.6.1 for your operating system. On linux, you can make use of the automated scripts
in the scripts directory.
* Download the sources from the Releases of this repository.
* The default nRF SDK implementation of the gettimeofday in the nRF SDK needs to be declared as `weak` in order for us 
to have proper time integration. If on Lunux, you can run patch-gettimeofday.sh in the scripts directory 
with defined ZEPHYR_BASE.
* Open the project using the Segger Embedded Studio with *File->Open nRF Connect SDK Project* using:
  * CmakeLists.txt from one of the samples
  * Board Directory: 
    * for DK: *<NCS_ROOT>/zephyr/boards/arm/nrf9160dk_nrf9160*
    * for Thingy:91: *<NCS_ROOT>/nrf/boards/arm/thingy91_nrf9160*
    * for AVT9152-EVB: */boards/arm/nrf9160_avt9152*
  * Board Name: 
    * for DK: *nrf9160dk_nrf9160ns*
    * for Thingy:91: *thingy91_nrf9160ns*
    * for AVT9152-EVB: *nrf9160_avt9152ns*
* Configure the project with *Project->Configure nRF SDK Project*
  * Select *menuconfig*
  * Expand the *IoTConnect Sample* tree
  * Enter your Company ID (CPID) and Environment name. You can access this information via *Settings->KeyVault* 
  at the IoTConnect Web Site
* If you wish to build the project on linux without the Segger Embedded Studio, 
you can follow the steps in .github/workflows/main.yml and run appropriate scripts in the scripts directory.

### Integrating the SDK Into your own project.

The samples should contain most of the code that you can re-use and add to your application. 

In order to use the SDK in your own project, ensure that version of cJSON required by the 
IoTConnect C Library is included in your build. Also follow the instructions above to 
modify gettimeofday function so that the SDK can can use time properly.

Ensure that your application obtains and sets the current time before telemetry messages are sent.

In your CMakeLists.txt include the iotconnect-sdk library from this repo.

Before the library is initialized, set up the HTTP SEC_TAGs 10702 and 10703 on the modem per nrf_cert_store.h, 
or call:

```c
    err = NrfCertStore_ProvisionApiCerts();
    if (err) {
        printk("Failed to provision API certificates!\n");
    }

    err = NrfCertStore_ProvisionOtaCerts();
    if (err) {
        printk("Failed to provision OTA certificates!\n");
    }
```

Follow the certificates section to set up SEC_TAG 10701 with CA Cert, your device certificate and key.  

In your application code, initialize the SDK:

```editorconfig
    IOTCONNECT_CLIENT_CONFIG *config = IotConnectSdk_InitAndGetConfig();
    config->cpid = "Your CPID";
    config->duid = "Your Cevice Unique ID";
    config->env = "Your Environment";
    config->cmd_cb = on_command;
    config->ota_cb = on_ota;
    config->status_cb = on_connection_status;

    int result = IotConnectSdk_Init();
    if (0 != result) {
        printk("Failed to initialize the SDK\n");
    }

```

You can assign callbacks to NULL or implement on_command, on_ota, and on_connection_status depending on your needs. 

Ether from a task or your main code, call *IotConnectSdk_Loop()* periodically. The function  will 
call the MQTT loop to receive messages. Calling this function more frequently will ensure 
that your commands and OTA mesages are received quicker. Call the function more frequently than CONFIG_MQTT_KEEPALIVE
configured in KConfig.

Set send telemtery messages by calling the iotc-c-lib the library telemetry message functions and send them with 
*IotConnectSdk_SendPacket()*:

```editorconfig
    IOTCL_MESSAGE_HANDLE msg = IOTCL_TelemetryCreate(IotConnectSdk_GetLibConfig());
    IOTCL_TelemetrySetString(msg, "your-name", "your value");
    // etc.
        const char *str = IOTCL_CreateSerializedString(msg, false);
    IOTCL_TelemetryDestroy(msg);
    IotConnectSdk_SendPacket(str);
    IOTCL_DestroySerialized(str);

``` 

Call *IotConnectSdk_Disconnect()* when done.

### Provisioning the Board and adding it to IoTConnect

Before the project can be run, the certificates need to be uploaded onto the board's modem and and your 
CA Certificate and device needs to be added to IoTConnect.

Follow these instructions to set up the board:

* You can assign a custom device Unique ID (DUID) in menuconfig (see below). If left blank, 
the DUID will be generated automatically for your nRF board based on IMEI of the board. 
If you are not using a custom DUID, you will need to obtain the device unique ID for your board first.
Build and run the project initially and capture the 
printout on the USB console where the message is printed "DUID: nrf-xxxxxx". Your device ID is 
your IMEI prefixed with "nrf-". Alternatively, you can run AT+CGSN AT command to obtain the IMEI.
* If using Self Signed authentication type, the device certificate and private key can be generated using 
the IoTConnect web UI during the device. You should not be supplying a password in the UI. 
Copy the Client Certificate and the Private Key into newly created files: *DUID*-crt.pem and *DUID*-key.pem 
anywhere on your pc. You will use these files in the steps below.
* If using CA Cert authentication, follow the instructions at https://github.com/Avnet/iotc-c-lib in the tools/ecc-certs directory 
to create the certificates for your board. The instructions also contain steps to add your CA Certificate to IoTConnect.
* Create a new Self Signed or CA Certificate based template in IoTConnect. Add at least one telemetry field 
"cpu". For the nrf-sensors-gps sample, see the values in the publish_telemetry() function in main.c.
* Create a new device with Unique ID described in the first tep of this guide and assign the newly created template to it. 
* Download the Baltimore Cyber Trust Root certificate from https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem 
* Install nRF Connect from Nordic's web site and install the the LTE Link Monitor. 
* Run the LTE Link Monitor.
* Power on your device, connect the USB cable to your PC and select your device from the pulldown on the top left.
* Click the Certificate Manager tab in the top right corner of the window:
  * Follow the instructions displayed at the top of the window to bring the modem into offline mode with AT+CFUN=4.
  * Copy and paste the contents of the following certificates (including the BEGIN and END lines):
    * CA Certificate: BaltimoreCyberTrustRoot.crt.pem
    * Client Certificate: the generated *DUID*-crt.pem
    * Private Key: the generated *DUID*-key.pem
  * Enter 10701 into the Security Tag field.
  * Click the Update Certificates button in the bottom right of the window.

Alternatively you can set PROVISION_TEST_CERTIFICATES in menuconfig and provide your certs in src/test_certs.c of your 
project. This method is not recommended for production.

### Running the Sample Projects

Once the project is configured, built and flashed onto your board, the application will run for several minutes. 
You can interrupt it by pressing the middle button for Thingy:91 or pressing Button 1 for DK or momentary shorting D11 
of AVT9152-EVB to GND to simulate short button press. Repeating the same action will connect the app to IoTConnect again. 
(Please refer to [AVT9152 EVB User Manual][evb_user_manual_link] on how to enable and locate Arduino D11 pin.)

The iotc-sensors-gps sample will trigger obtaining the GPS location when button is long-pressed. It will 
stay in this mode until the GPS fix is obtained or until the button is pressed again. 

You can increase the MAIN_APP_VERSION number, rebuild the app, upload app_signed.bin into IoTConnect and push an 
OTA to your board. The board will update itself if the new version is greater (string comparison) 
than the currently running version. 

### Other Things to Note

The files mcuboot_overlay-rsa.conf and pm_static.yaml in configuration directory is needed to 
allow you to simply build the project and program the app_signed.hex with over USB  
MCUBOOT when you have the default asset tracker programmed onto the board. 
This file needs to match the asset_tracker's configuration in order for OTA to work. If you intend to use the code
by programming all of the the boards with JTAG, these files are not needed.

Only the Thingy91 (asset_tracker) and AVT9152 evb (demo_IoTConnect) has mcuboot serial recovery enabled 
in their respective default application.

[evb_user_manual_link]:
https://www.avnet.com/wps/wcm/connect/onesite/3788e3c1-a386-4196-a88f-21307ff28984/AVT9152+EVB+User+Manual+v1.0.pdf?MOD=AJPERES&CACHEID=ROOTWORKSPACE.Z18_NA5A1I41L0ICD0ABNDMDDG0000-3788e3c1-a386-4196-a88f-21307ff28984-nvGFXpQ
