# ESP8266 Fireplace Controller

This is a simple controller for a gas fireplace with a low voltage DC wall switch.

The controller uses a [Wemos D1 mini Lite](https://wiki.wemos.cc/products:d1:d1_mini_lite) with a [Wemos Relay Shield](https://wiki.wemos.cc/products:d1_mini_shields:relay_shield). The Relay Shield is wired in parallel with the wall switch so that either can turn on the fireplace. Note that this means if one is on, the other switch cannot turn it off.

![Fireplace controller web UI](/doc/images/web-ui.png?raw=true =100px)

## Software

The project should work with the Arduino IDE or Platformio.

1. Clone the repository
```
git clone
```
2. Copy `src/config.h-example` to `src/config.h` and edit it.
- Set `WIFI_SSID` and `WIFI_PASSWORD` to the name and password for the wifi network that the controller will connect to
- Set `MDNS_NAME` to the name you'd like to use for web access - I recommend `fireplace`, which will let you browse to `http://fireplace.local`
- If you want to use IFTTT, set `IFTTT_EVENT_NAME` and `IFTTT_API_KEY` to appropriate values. If you don't want to use IFTTT then delete the `#define` lines for them.

3. Connect your Wemos D1 mini Lite board to your computer using a USB cable and:
- if you're using Platformio run `platformio run -t upload`
- if you're using the Arduino IDE click the build and upload button

## Siri

## Security Considerations

This application is built for convenience.

Absolutely do not run this on the public Internet without blocking it at a firewall.
