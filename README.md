# ESP Google Fast Pair Service (GFPS) Example

Example application for ESP32 to test integration with google fast pair service
(GFPS) for optimized pairing / onboarding of BLE devices with Android phones. 

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
