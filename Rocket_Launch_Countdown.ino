/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
*********/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


//========================================================================
//----------------------------Change these--------------------------------
//SSID:
const char* ssid = "MyAltice 465709";

//WIFI Password:
const char* password = "6071-grey-92";
//------------------------------------------------------------------------
//========================================================================


const char* launchesHost = "fdo.rocketlaunch.live";
const char* timehost = "http://worldtimeapi.org/api/ip";
String rawLaunchData = "";
String launchName = "";
String launchProvider = "";
String vehicle = "";
long startTimeOffset; //Add this to millis() to find the current time in ms
long long launchTime;
int screenDelay = 5000;
long screenStart = millis();
enum {
  LAUNCHNAME,
  COUNTDOWN,
  COMPANY,
  VEHICLE
} screen;

StaticJsonDocument<3072> doc;

WiFiClient wfc;
WiFiClientSecure client;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Wire.begin(D1,D2);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  connectingWIfi();
  display.display();
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }  
  startTimeOffset = getTimeMS() - millis();

  parseLaunchData(getLaunchDataRaw());
  screen = LAUNCHNAME;
  screenStart = millis();
  //display.startscrollright(0x00,0x07);
  display.setTextWrap(false);
}

void loop() {  
  if(screen == LAUNCHNAME) {
    nextLaunchName();
    if(millis() - screenStart > screenDelay) {
      Serial.println("Countdown Screen");
      screen = COUNTDOWN;
      screenStart = millis();
    }
  } else if(screen == COUNTDOWN) {
    nextLaunchCountdown();
    if(millis() - screenStart > screenDelay) {
      screen = COMPANY;
      screenStart = millis();
      Serial.println("Launch Company Screen");
    }
  } else if(screen == COMPANY) {
    company();
    if(millis() - screenStart > screenDelay) {
      screen = VEHICLE;
      screenStart = millis();
      Serial.println("Launch Vehicle Screen");
    }
  } else if(screen == VEHICLE) {
    vehicleScreen();
    if(millis() - screenStart > screenDelay) {
      parseLaunchData(getLaunchDataRaw());
      screen = LAUNCHNAME;
      screenStart = millis();
      Serial.println("Launch Name Screen");
    }
  }
  
  display.display();
  delay(10);
}

void parseLaunchData(String rawData) {
  String rawLaunchData = getLaunchDataRaw();
  DeserializationError error = deserializeJson(doc, rawLaunchData);

  if(error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }

  launchName = String(doc["result"][0]["name"]);
  launchTime = launchTimeMS(String(doc["result"][0]["win_open"]));
  launchProvider = String(doc["result"][0]["provider"]["name"]);
  vehicle = String(doc["result"][0]["vehicle"]["name"]);
}

String getLaunchDataRaw() {
  String response = "";
  const int httpPort = 443;
  client.setInsecure();
  if(!client.connect(launchesHost, httpPort)) {
    Serial.println("Connection failed");
    return "Error";
  }

  String url = "/json/launches/next/1";
  client.println(String("GET ") + url + " HTTP/1.1");
  client.print("Host: ");
  client.println(launchesHost);
  client.println("");

  while(client.connected()) {
    while(client.available()) {
      char c = client.read();
      //Serial.print(c);
  
      if(c == '\n') {
        c = client.read();
        //Serial.print(c);
        if(c == '\r') {
          c = client.read();
          //Serial.print(c);
          if(c == '\n') {
            c = client.read();
            //Serial.print(c);
            client.readStringUntil('\n');
            response = client.readStringUntil('\n');
            return response;
          }
        }
      }
    }
  }
}

//Converts the zulu timestamp string of the rocket launch into a unix timestamp
long long launchTimeMS(String timestamp) {
  HTTPClient http;
  http.begin(wfc, "http://showcase.api.linx.twenty57.net/UnixTime/tounix?date=" + timestamp);
  int httpCode = http.GET();

  if(httpCode > 0) {
    String payload = http.getString();
    long long ms;
    payload.remove(0,1);
    payload.remove(payload.length()-1, 1);
    ms = payload.toInt();
    //Serial.println("Raw launch time as str: " + payload);
    //Serial.print("Launch time in seconds: ");
    //Serial.println(ms);
    ms = ms * 1000;
    //Serial.print("Launch time in MS: ");
    //Serial.println(ms);
    return ms;
  }

  http.end();
}

long getTimeMS() {
  HTTPClient http;
  http.begin(wfc, timehost);
  int httpCode = http.GET();

  if(httpCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<768> tsDoc;
    DeserializationError error = deserializeJson(tsDoc, payload);
    long nowUnixTs = long(tsDoc["unixtime"]);
    Serial.print("Current time: ");
    Serial.println(nowUnixTs * 1000);
    return nowUnixTs * 1000;
  }

  http.end();
}

void connectingWIfi() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Connecting to WiFi...");
  display.println(ssid);
}

void nextLaunchName() {
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  //display.println("Next Launch:");
  display.println(launchName);
}

void company() {
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  //display.println("Launch Provider:");
  display.println(launchProvider);
}

void vehicleScreen() {
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  //display.println("Vehicle:");
  display.println(vehicle);
}

void nextLaunchCountdown() {
  long tminusMS = launchTime - (millis() + startTimeOffset);
  //Serial.print("Current MS: ");
  //Serial.println(millis() + startTimeOffset);

  int days = 0; int hours = 0; int minutes = 0; int seconds = 0;
  if(tminusMS > 0) {
    days = floor(tminusMS / (8.64 * pow(10,7)));
    tminusMS -= days * (8.64 * pow(10,7));
    hours = floor(tminusMS / (3.6 * pow(10,6)));
    tminusMS -= hours * (3.6 * pow(10,6));
    minutes = floor(tminusMS / 60000);
    tminusMS -= minutes * 60000;
    seconds = floor(tminusMS / 1000);
    tminusMS -= seconds * 1000;
  }

/*
  Serial.print("Days: ");
  Serial.println(days);
  Serial.print("Hours: ");
  Serial.println(hours);
  Serial.print("Minutes: ");
  Serial.println(minutes);
  Serial.print("Seconds: ");
  Serial.println(seconds);
  Serial.print("Leftover MS: ");
  Serial.println(tminusMS);
*/
  String tMinus = "";
  if(days < 1) {
    tMinus = "T-" + String(hours) + ":" + String(minutes) + ":" + String(seconds);
  } else if(days < 10) {
    tMinus = "T-" + String(days) + "d, " + String(hours) + "h";
  } else {
    tMinus = "T-" + String(days) + " Days";
  }
  
  
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println(tMinus);
}
