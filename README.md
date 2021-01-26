#### IoTConnect nRF SDK

This repository contains IoTConnect nRF SDK and samples intended for use with Avnet's IoTConnect platform.

The sample shows how to make use of the IoTConnect SDK to connect your DK or Thingy 91 devices to IoTConnect

Note iotc-basic demo can be run on the DK or Thingy91. The iotc-sensors-gps sample is intended for Thingy91.

### Building

* Install the nRF SDK v1.4.1 for your operating system. On linux, you can make use of the automated scripts
in the scripts directory. 
* Clone this repo into a directory on your system. This directory does not have to be in the ZEPHYR_BASE.
* nRF SDK 1.3.x is not directly supported. If you wish to use the 1.3.X nRF SDK version, 
search for **1.3.X** in prj.conf and nrf_fota.c and make appropriate source modification. 
* On linux, run scripts/pull-cjson-lib.sh in this directory or clone the cJSON library from 
github (see the file contents). The cJSON libraries need to be at later revision than what's available in the SDK.
* On linux, run scripts/setup-iotc-c-lib.sh in this directory or follow these steps:
  * Clone the https://github.com/Avnet/iotc-c-lib repo into this directory
  * Copy common/iotc-c-lib-overlay/CMakeLists.txt into the root of the cloned repo
* If on Lunux, run patch-gettimeofday.sh with defined ZEPHYR_BASE or apply the same changes, if on another OS. 
The default implementation of the gettimeofday in the nRF SDK needs to be declared as `weak` in order for us 
to have proper time integration.
* Open the project using the Segger Embedded Studio with *File->Open nRF Connect SDK Project* using:
  * CmakeLists.txt from one of the samples
  * Board Directory: for DK: **NCS_ROOT**/zephyr/boards/arm/nrf9160dk_nrf9160, 
  for Thingy91: **NCS_ROOT**/nrf/boards/arm/thingy91_nrf9160
  * Board Name: nrf9160dk_nrf9160ns or thingy91_nrf9160ns, for DK or Thingy91 respectively
* Configure the project with *Project->Configure nRF SDK Project*
  * Select *menuconfig*
  * Expand the *IoTConnect Sample* tree
  * Enter your Company ID (CPID) and Environment name. You can access this information via *Settings->KeyVault* 
  at the IoTConnect Web Site
* If you wish to build the project on linux without the Segger Embedded Studio, 
you can follow the steps in .github/workflows/main.yml and run appropriate scripts in the scripts directory.

### Provisioning the Board and adding it to IoTConnect

Before the project can be run, the certificates need to be uploaded onto the board's modem and and your 
CA Certificate and device needs to be added to IoTConnect.

Follow these instructions to set up the board:

* The device Unique ID (DUID) will be generated automatically for your nRF board based on IMEI of the board. 
You need to obtain the device unique ID for your board first. Build and run the project initially and capture the 
printout on the USB console where the message is printed "DUID: nrf-xxxxxx". Your device ID is 
your IMEI prefixed with "nrf-". Alternatively, you can run AT+CGSN AT command to obtain the IMEI.
* Follow the instructions at https://github.com/Avnet/iotc-c-lib in the tools/ecc-certs directory 
to create the certificates for your board. The instructions also contain steps to add your CA Certificate to IoTConnect.
* Create a new CA Certificate based template in IoTConnect and select the uploaded certificate. Add at least one 
telemetry field "cpu". For the nrf-sensors-gps sample, see the values in the publish_telemetry() function in main.c.
* Create a new device with Unique ID described in the first tep of this guide and assign the newly created template to it. 
* Download the Baltimore Cyber Trust Root certificate from https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem 
* Install nRF Connect from Nordic's web site and install the the LTE Link Monitor. 
* Run the LTE Link Monitor.
* Power on your device, connect the USB cable to your PC and select your device from the pulldown on the top left.
* Click the Certificate Manager tab in the  top right corner of the window.
* Follow the instructions displayed at the top of the window to bring the modem into offline mode with AT+CFUN=4.
* Copy and paste the contents of the following certificates (including the BEGIN and END lines):
  * CA Certificate: BaltimoreCyberTrustRoot.crt.pem
  * Client Certificate: the generated nrf-**YourIMEI**-crt.pem
  * Private Key: the generated nrf-**YourIMEI**-key.pem
* Enter 10701 into the Security Tag field.
* Click the Update Certificates button in the bottom right of the window.

Alternatively you can set PROVISION_TEST_CERTIFICATES in menuconfig and provide your certs in src/test_certs.c of your 
project. This method is not recommended for production.

### Running the Sample Projects

Once the project is configured, built and flashed onto your board, the application will run for several minutes. 
You can interrupt it by pressing the middle button. Pressing the button again will connect the app to IoTConnect again.

The iotc-sensors-gps sample will trigger obtaining the GPS location when the Thingy91 button is long-pressed. It will 
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

### AGPS Support

There is support for AGPS support on the gps-support branch in this repo.

Because of antenna sharing between LTE and GPS, AGPS has to be used in LTE+GPS mode 
(rather than exclusive modes, which is the current model). This may cause slight perfromance
degfreadataion and unknown behavior with some carrier DRX mode support etc. For those reasons, 
the prototype AGPS code is currently on a separate branch and may not support all future enhancements.

In order to build this demo on the gsp-support branch, ensure that download the SUPL library from 
NRF 9160 web page and agree to its license. Enable SUPL_CLIENT_LIB and AGPS in KConfig
and compile the gps demo.