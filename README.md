# Weather-Station
A weather station using esp32 microcontroller which you can access it by just click on a link. You can see datas such as temperature 

The web part explanation: 
When the user presses the "Read the weather now" on the website the web sends a "read" message to the broker which is HiveMQ and when HiveMQ receives the "read" it passes it to the ESP32 because the ESP32 already subscribed to that topic.
According to the code the ESP32 collects the data from the sensors, calculates the dew point, then sends the data to the broker as a JSON message then the broker passes the JSON to the website. And if more than one person has the website open the broker sends it to all of them at the same time.

## Hardware

- ESP32 (WROOM-32 module or any DevKit)
- DHT22 / AM2302 — temperature and humidity
- BMP280 on an HW-611 breakout — pressure and temperature
- One 5.1 kΩ resistor

## Notes:

The callback only raises a flag. `onMessage()` sets `requestPending = true` and returns.
Reading a DHT22 takes a quarter of a second and doing that inside the MQTT library's own
callback will crash the ESP32. The work happens in `loop()`, which is a safe place.

No `delay()` anywhere in `loop()`. During a delay the board can't call `mqtt.loop()` so
it stops receiving and the broker may drop it. Timing uses `millis()` instead.

Certificate checking is off (`net.setInsecure()`). The traffic is still encrypted what's
skipped is verifying the broker's identity. The strict alternative embeds a root certificate
that expires and silently breaks the board a year later.

A five-second rate limit in the firmware stops a stuck browser tab, or a shared link, from
making the board thrash.

DHT22 quirks.It needs its pull-up resistor refuses to be read more than once every two
seconds and fails occasionally because Wi-Fi interrupts disturb its microsecond timing. A
failed read is normal retry rather than treating it as a fault.




