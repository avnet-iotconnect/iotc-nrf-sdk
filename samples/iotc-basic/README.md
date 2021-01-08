#### IoTConnect SDK Sample

This sample shows how to make use of the IoTConnect SDK to connect your Thingy 91 devices to IoTConnect

Note that the nRF 9160 DK is not yet fully supported, but can be used with minor changes.

### Building

* Clone this repo into a directory on your system. This directory does not have to be in the ZEPHYR_BASE.
* Clone the NRF SDK v1.4.0
* SDK 1.3.x is not directly supported. Search for **1.3.X** in prj.conf and nrf_fota.c and make appropriate source modification. 
* If on Linux, run pull-cjson-lib.sh or clone the cJSON library from github (see the file contents). The cJSON libraries need to be at later revision than what's available in the SDK.
* If on Lunux, run patch-gettimeofday.sh with defined ZEPHYR_BASE or apply the same changes if on another OS. The default implementation of the gettimeofday needs to be declared as `weak` in order for us to have proper time integration.
* Open the project using the Segger Embedded Studio with *File->Open nRF Connect SDK Project* using:
  * CmakeLists.txt from this directory
  * Board Directory: ncs/nrf/boards/arm/thingy91_nrf9160
  * Board Name: thingy91_nrf9160ns
* Configure the project with *Project->Configure nRF SDK Project*
  * Select *menuconfig*
  * Expand the *IoTConnect Sample* tree
  * Enter your Company ID (CPID) and Environment name. You can access this information via *Settings->KeyVault* at the IoTConnect Web Site

### Things to Note

The file pm_static.yaml in configuration directory is needed to allow you to simply build the project and progam the app_signed.hex when you have the default asset tracker progrmamed onto the board. This file needs to match the asset_tracker's configuration in ordee for OTA to work, but it has no other purpose.  

The sensors code is modified slightly from the asset_tracker application for simplicity.

### Running the Project

Once the project is configured, built and flashed onto your board, the application will run for several minutes. You can interrupt it by pressing the middle button. Pressing the button again will reset the board.

You can increase the APP_VERSION number, rebuild the app, upload app_signed.bin into iotconnect and push an OTA to your board. The board will update itself if the new version is greater (string comparison) than the currently running version. 
