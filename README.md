# Discord Spycam
This project uses a ESP32-Cam connected to a motion sensor. When the motion sensor is activated, a photo will be submitted to a desired Discord channel via a webhook.

## Parts needed
- [ESP32-Cam](https://amzn.to/3oX2T48)
- [PIR Motion Sensor](https://amzn.to/2N6Itsb)

## Installation
1. Connect PIR sensor as referenced in the diagram below.
1. Copy `arduino_secrets.h.example` to `arduino_secrets.h`.
1. Create a Discord certification, using `CertToESP32.py` from [SensorsIot/HTTPS-for-Makers](https://github.com/SensorsIot/HTTPS-for-Makers).
1. Update `arduino_secrets.h` with ssid name, password, certificate from the previous step, and your webhook from Discord.
   - _Note: Be sure to remove the domain from the webhook URL. Reference to the example value._

## Wiring
![Wiring Diagram for ESP32CAM to PIR motion Sensor](/assets/discord-spycam.jpg)


