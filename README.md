# Esp8266-Neopixel_Mqtt_Artnet
WS2812 strip with 3 mqtt topics for pixel ranges, native Openhab support and Art-net (DMX) node

Only one configuration line necessary per topic for openhab in the .items file. e.

Color Bedroom_Dresser (Bedroom) {mqtt=">[mqtt:home/bedroom/dresser:command:*:default"]
