# ESP Google Fast Pair Service (GFPS) Example

Example application for ESP32 to test integration with google fast pair service
(GFPS) for optimized pairing / on boarding of BLE devices with Android phones. 

This example is based on the following ESP-IDF examples:

- [gatt_server_service_table](https://github.com/espressif/esp-idf/tree/d2471b11e78fb0af612dfa045255ac7fe497bea8/examples/bluetooth/bluedroid/ble/gatt_server_service_table)
- [gatt_server](https://github.com/espressif/esp-idf/tree/d2471b11e7/examples/bluetooth/bluedroid/ble/gatt_server)

And implements the service and characteristics for GFPS according to the [google
fast pair characteristics specification
page](https://developers.google.com/nearby/fast-pair/specifications/characteristics)

For additional information and to see a reference library for implementing the
nearby/embedded framework, see [google/nearby/embedded
github](https://github.com/google/nearby/tree/main/embedded)

To run this example, you must use `menuconfig` to configure the project and set
the following attributes:

- `BLE Device Name`: this should match the `SKU Name` that you have configured
  in the Fast Pair dashboard.
- `Model ID`: this should match the model id (24 bit) that the Fast Pair
  dashboard generated for your device.
- `Anti-Spoofing Private Key`: this should match the base64 string encoded anti
  spoofing private key that the fast pair dashboard generated for the associated
  SKU+Model ID provided above.

Fast Pair Console:
![CleanShot 2023-08-14 at 11 50 58](https://github.com/finger563/esp-gfps-example/assets/213467/d7c81025-46f9-4c7b-b0f7-8bde0b772426)

Menuconfig:
![CleanShot 2023-08-14 at 11 54 18](https://github.com/finger563/esp-gfps-example/assets/213467/68b81bb2-ab26-456e-a69e-7c7ca98c2327)


## Cloning

Since this repo contains a submodule, you need to make sure you clone it
recursively, e.g. with:

``` sh
git clone --recurse-submodules git@github.com:finger563/esp-gfps-example
```

Alternatively, you can always ensure the submodules are up to date after cloning
(or if you forgot to clone recursively) by running:

``` sh
git submodule update --init --recursive
```

## Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Output

Example screenshot of the console output from this app:

![CleanShot 2023-07-12 at 14 01 21](https://github.com/esp-cpp/template/assets/213467/7f8abeae-121b-4679-86d8-7214a76f1b75)
