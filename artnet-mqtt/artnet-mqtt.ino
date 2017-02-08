#include <EEPROM.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <PubSubClient.h>
//needed for library
#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArtnetWifi.h>
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>

//for LED status
//#include <Ticker.h>
#define WLAN_SSID       "SSID"
#define WLAN_PASS       "PASSWORD"
const char* mqtt_server = "192.168.0.100";
WiFiClient espClient;
PubSubClient client(espClient);
const char* outTopic = "debug";
const char* inTopic = "home/bedroom/#";
const int numLeds = 34; // change for your setup
const int numberOfChannels = numLeds * 3;
//CRGB leds[numLeds];)*/
const uint16_t PixelCount = 34; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 2;
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor black(0, 0, 0);
RgbColor color(255, 255, 255);
RgbColor color1(255, 255, 255);
RgbColor color2(255, 255, 255);
RgbColor color3(255, 255, 255);
uint8_t brightness = 0; //0 = high
NeoPixelBus<NeoBrgFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);

ArtnetWifi artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  sendFrame = 1;
  // set brightness of the whole strip
  if (universe == 15)
  {
    brightness = 255 - (data[0]);
    //Darken(brightness);
    strip.Show();
  }

  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (universesReceived[i] == 0)
    {
      //Serial.println("Broke");
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length / 3; i++)
  {
    int led = i + (universe - startUniverse) * (previousDataLength / 3);
    if (led < numLeds)
      color = RgbColor(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    strip.SetPixelColor(led, color);
    //Darken(brightness);
  }
  previousDataLength = length;

  if (sendFrame)
  {
    strip.Show();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}



void initTest()
{
  strip.ClearTo(red);
  strip.Show();
  delay(500);
  strip.ClearTo(green);
  strip.Show();
  delay(500);
  strip.ClearTo(blue);
  strip.Show();
  delay(500);
  strip.ClearTo(black);
  strip.Show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
  //client.publish(outTopic, (char*)payload);
  if ((String)topic == "home/bedroom/dresser") {
    color1 = payload2rgb(message);
    strip.ClearTo(color1, 0, 15);
    EEPROM.write(1, color1.R);
    EEPROM.write(2, color1.G);
    EEPROM.write(3, color1.B);
  }
  if ((String)topic == "home/bedroom/master") {
    color2 = payload2rgb(message);
    strip.ClearTo(color2, 16, 24);
    EEPROM.write(4, color2.R);
    EEPROM.write(5, color2.G);
    EEPROM.write(6, color2.B);
  }
  if ((String)topic == "home/bedroom/slave") {
    color3 = payload2rgb(message);
    strip.ClearTo(color3, 25, 33);
    EEPROM.write(7, color3.R);
    EEPROM.write(8, color3.G);
    EEPROM.write(9, color3.B);
  }


  EEPROM.commit();
  strip.Show();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  strip.Begin();
  strip.Show();
  /*//set led pin as output
    FastLED.addLeds<WS2812, 13, BRG>(leds, 0, 16);
    FastLED.addLeds<WS2812, 14, BRG>(leds, 16, 9);
    FastLED.addLeds<WS2812, 12, BRG>(leds, 25, 9);*/
  // strip.SetBrightness(60);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  for (int i = 0; i < 16; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    initTest();
    Serial.print(".");
    delay(500);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setPassword((const char *)"1234");
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  artnet.begin();
  artnet.setArtDmxCallback(onDmxFrame);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  EEPROM.begin(16);
  strip.ClearTo(black);
  strip.Show();
  //  strip.SetBrightness(255);
}


void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  artnet.read();
  if (!client.connected()) {
    //Try to connect
    if (client.connect("ESP8266Client")) {
      // Once connected, publish an announcement...
      client.publish(outTopic, "Bedroom ESP8266 Booted");
      color1 = RgbColor (EEPROM.read(1), EEPROM.read(2), EEPROM.read(3));
      color2 = RgbColor (EEPROM.read(4), EEPROM.read(5), EEPROM.read(6));
      color3 = RgbColor (EEPROM.read(7), EEPROM.read(8), EEPROM.read(9));
      strip.ClearTo(color1, 0, 15);
      strip.ClearTo(color2, 16, 24);
      strip.ClearTo(color3, 25, 33);
      strip.Show();
      // ... and resubscribe
      client.subscribe(inTopic);
    }

  }
  client.loop();
}

RgbColor payload2rgb(String payload) {
  int firstindex = payload.indexOf(',');
  int secondindex = payload.indexOf(',', firstindex + 1);
  String h = payload.substring(0, firstindex);
  String s = payload.substring(firstindex + 1, secondindex);
  String b = payload.substring(secondindex + 1); // To the end of the string
  HsbColor newcolor (((float) h.toInt()) / 360, ((float) s.toInt()) / 100, ((float) b.toInt()) / 100);
  //testing: client.publish(outTopic, (char*) b.c_str());
  return RgbColor(newcolor);
}

