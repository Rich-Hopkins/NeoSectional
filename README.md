# NeoSectional
This repo contains the hardware and Arduino code portion of the NeoSectional project that our EAA Chapter put together. See <a href="https://github.com/bsmichael/aerie" target="_blank">Aerie</a> for the weather service that this sketch fetches weather data from.

![NeoSectional picture](https://github.com/Rich-Hopkins/NeoSectional/blob/master/NeoSectional.jpg)

The code uses the following hardware (see the Fritzing schematic in the repo):
- <a href="https://smile.amazon.com/gp/product/B081CSJV2V" target="_blank">ESP8266 NodeMCU Arduino device</a>
- <a href="https://smile.amazon.com/gp/product/B01AG923GI" target="_blank">Alitove WS2811 LED string</a>
- <a href="https://smile.amazon.com/gp/product/B07BDFMQF6" target="_blank">Standard 5v regulator</a>
- <a href="https://smile.amazon.com/gp/product/B07JZ463K8" target="_blank">9V power supply</a>
- Resistors
- Capacitors

### Libraries
I used the following libraries:
- <a href="https://github.com/ropg/ezTime" target="_blank">ezTime</a>
  - This time library and its associated service allow the sketch to determine the age of the data, as well as the time of day. Data that is more than two hours old will not be displayed (pixel turned off). Time of day is used to turn off the pixels between 10pm and 6am Eastern time (you can adjust this to your time zone in code), which is optional based on a switch.
- <a href="https://arduinojson.org/" target="_blank">ArduinoJSON</a>
  - This is used to parse the JSON from the weather service provided by Brian Michael (<a href="https://github.com/bsmichael/aerie" target="_blank">Aerie</a>)
- <a href="https://github.com/adafruit/Adafruit_NeoPixel" target="_blank">Adafruit NeoPixel</a>
  - This library helps with the LED string
- <a href="https://arduino-esp8266.readthedocs.io/en/latest/" target="_blank">ESP8266 libraries</a> for this particular Arduino device, which has built-in WiFi, and is cheap - the two major reasons for choosing it

### Service
- <a href="https://github.com/bsmichael/aerie" target="_blank">Aerie</a> is the weather service provided by Brian Michael, which caches weather data from a more limited service

### How it Works
The sketch starts out setting up the NeoPixels and the Wifi. It does a scan of WiFi and if there are open WiFi connections found, it ranks them by highest signal strength and connects to the strongest one. If it does not find an open WiFi, it looks for my phone hotspot and my home SSID. You can update to your own secure SSID/Password in the sketch.

In the loop, it first looks for Wifi status, and connects if necessary (in case it lost connection after the last update). Then it syncs the time with the time service provided by ezTime. It sets local time to New York time, as this is where we are located.

It then checks to see if either it is within the time window, or the all night switch is on, and if so, calls the weather API. If it is nighttime and the switch is off, it turns all the pixels off. Once it gets the response, it loops through and sets each pixel to the appropriate color, provided the information for that pixel is not stale (more than two hours old). If stale, the pixel is turned off. The loop then has a 10 minute delay.
