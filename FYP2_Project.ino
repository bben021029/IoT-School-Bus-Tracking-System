#include <Wire.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi credentials
const char* ssid = "syc";
const char* password = "1234567890";

// Google API Key
const char* GOOGLE_API_KEY = "AIzaSyCISu7FdaM-KEndErSKTdyzk8e4wFolcE0";

// ThingSpeak
unsigned long channelID1 = 2933268;
const char* writeAPIKey1 = "J7PPV4XU4P4FC12I";
unsigned long channelID2 = 2972145;
const char* writeAPIKey2 = "JPK6BGA6030FITL3";
unsigned long channelID3 = 2978167;
const char* writeAPIKey3 = "H2YSTVH7ML3ZR76G";
unsigned long channelID4 = 2978232;
const char* writeAPIKey4 = "OQL1QGZDQQ0TXU8I";

// Coordinates
const float HOME_LAT_STUDENT1 = 2.25349;
const float HOME_LNG_STUDENT1 = 102.27736;
const float HOME_LAT_STUDENT2 = 2.23894;
const float HOME_LNG_STUDENT2 = 102.26033;
const float SCHOOL_LAT = 2.24978;
const float SCHOOL_LNG = 102.27610;

// GPS
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
HardwareSerial mySerial(1);
TinyGPSPlus gps;

// RFID
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// UIDs
String student1UID = "D3:3E:34:16";
int student1ScanCount = 0;
String student2UID = "24:0D:8F:02";
int student2ScanCount = 0;

// Time
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 8 * 3600, 60000);

// Networking
WiFiClient client;

// Twilio
const char* accountSID = "ACcf50793f9512642c9d7ea32a489b5162";
const char* authToken = "9174a075e4006f9dc882dfef0f6de2b5";
const char* fromNumber = "+17163562990";
const char* toNumber = "+601116165100";

// Timers
unsigned long lastGpsUpload = 0; 
unsigned long lastRfidScanTime = 0;
const unsigned long gpsUploadInterval = 30000;

float lastValidLat = 0.0;
float lastValidLng = 0.0;

String getMalaysiaTimeString() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&rawTime);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
  return String(buffer);
}

String getETAFromGoogle(float fromLat, float fromLng, float toLat, float toLng) {
  if (WiFi.status() != WL_CONNECTED) return "N/A";

  HTTPClient http;
  String url = "https://maps.googleapis.com/maps/api/distancematrix/json?origins=" + String(fromLat, 6) + "," + String(fromLng, 6) +
               "&destinations=" + String(toLat, 6) + "," + String(toLng, 6) +
               "&key=" + GOOGLE_API_KEY;
  
  http.begin(url);
  int httpResponseCode = http.GET();
  String eta = "N/A";

  if (httpResponseCode == 200) {
    String payload = http.getString();
    int idx = payload.indexOf("\"duration\"");
    if (idx != -1) {
      int textIdx = payload.indexOf("\"text\"", idx);
      int colonIdx = payload.indexOf(":", textIdx);
      int quoteStart = payload.indexOf("\"", colonIdx + 1);
      int quoteEnd = payload.indexOf("\"", quoteStart + 1);
      if (quoteStart != -1 && quoteEnd != -1) {
        eta = payload.substring(quoteStart + 1, quoteEnd);
      }
    }
  } else {
    Serial.print("Failed to call Google API. HTTP code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return eta;
}

void sendSMS(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. SMS not sent.");
    return;
  }

  HTTPClient http;
  String twilioServer = "https://api.twilio.com/2010-04-01/Accounts/" + String(accountSID) + "/Messages.json";
  String postData = "From=" + String(fromNumber) + "&To=" + String(toNumber) + "&Body=" + message;

  http.begin(twilioServer);
  http.setAuthorization(accountSID, authToken);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.println("SMS sent.");
  } else {
    Serial.print("SMS failed. Code: ");
    Serial.println(httpResponseCode);
  }

  Serial.println("SMS message sent:");
  Serial.println(message);
  http.end();
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Starting system");
  Serial.println("Starting system...");

  // Connect WiFi
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
  Serial.println("\nWiFi connected");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  delay(1500);

  // Initialize NTP Client
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Getting NTP time");
  Serial.println("Getting NTP time...");
  timeClient.begin();
  while (!timeClient.update()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nNTP time obtained: " + getMalaysiaTimeString());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NTP time set");
  delay(1500);

  // Init GPS Serial
  mySerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS serial started");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
  Serial.println("ThingSpeak initialized");

  // Initialize PN532 RFID
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Init PN532...");
  Serial.println("Initializing PN532...");
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PN532 NOT FOUND");
    while (1);
  }
  nfc.SAMConfig();
  Serial.println("PN532 initialized");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  Serial.println("Setup completed. System Ready.");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  while (mySerial.available()) {
    gps.encode(mySerial.read());
    if (gps.location.isValid()) {
      lastValidLat = gps.location.lat();
      lastValidLng = gps.location.lng();
    }
  }

  if (currentMillis - lastGpsUpload >= gpsUploadInterval) {
    lastGpsUpload = currentMillis;
    ThingSpeak.setField(5, lastValidLat);
    ThingSpeak.setField(6, lastValidLng);
    ThingSpeak.writeFields(channelID1, writeAPIKey1);
  }

  if (currentMillis - lastRfidScanTime >= 5000) {
    lastRfidScanTime = currentMillis;

    if (nfc.inListPassiveTarget()) {
      uint8_t uid[7]; uint8_t uidLength;
      if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        String uidStr = "";
        for (uint8_t i = 0; i < uidLength; i++) {
          if (uid[i] < 16) uidStr += "0";
          uidStr += String(uid[i], HEX);
          if (i < uidLength - 1) uidStr += ":";
        }
        uidStr.toUpperCase();

        String now = getMalaysiaTimeString();
        float lat = lastValidLat;
        float lng = lastValidLng;

        // === STUDENT 1 ===
        if (uidStr == student1UID) {
          student1ScanCount++;
          if (student1ScanCount > 4) student1ScanCount = 1;
          Serial.print("Student 1 RFID Scan #"); Serial.println(student1ScanCount);
          Serial.print("GPS: "); Serial.print(lat, 6); Serial.print(", "); Serial.println(lng, 6);

          switch (student1ScanCount) {
            case 1: {
              ThingSpeak.setField(1, lat); ThingSpeak.setField(2, lng);
              ThingSpeak.writeFields(channelID2, writeAPIKey2);
              ThingSpeak.setField(1, now);
              ThingSpeak.writeFields(channelID1, writeAPIKey1);

              String eta = getETAFromGoogle(lat, lng, SCHOOL_LAT, SCHOOL_LNG);

              // NEW: ETA to school in Field 7
              ThingSpeak.setField(7, eta);
              ThingSpeak.writeFields(channelID1, writeAPIKey1);

              sendSMS("Student 1 boarded (morning) at " + now + ".\nETA at school: " + eta);
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu1 AM Boarded");
              lcd.setCursor(0, 1); lcd.print("ETA:" + eta);
              break;
            }
            case 2:
              ThingSpeak.setField(3, lat); ThingSpeak.setField(4, lng);
              ThingSpeak.writeFields(channelID2, writeAPIKey2);
              ThingSpeak.setField(3, now);
              ThingSpeak.writeFields(channelID1, writeAPIKey1);
              sendSMS("Student 1 alighted (school) at " + now + ".");
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu1 At School");
              break;
            case 3: {
              ThingSpeak.setField(5, lat); ThingSpeak.setField(6, lng);
              ThingSpeak.writeFields(channelID2, writeAPIKey2);
              ThingSpeak.setField(2, now);
              ThingSpeak.writeFields(channelID1, writeAPIKey1);

              String eta = getETAFromGoogle(lat, lng, HOME_LAT_STUDENT1, HOME_LNG_STUDENT1);

              // NEW: ETA to home in Field 8
              ThingSpeak.setField(8, eta);
              ThingSpeak.writeFields(channelID1, writeAPIKey1);

              sendSMS("Student 1 boarded (afternoon) at " + now + ".\nETA at home: " + eta);
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu1 PM Boarded");
              lcd.setCursor(0, 1); lcd.print("ETA:" + eta);
              break;
            }
            case 4:
              ThingSpeak.setField(7, lat); ThingSpeak.setField(8, lng);
              ThingSpeak.writeFields(channelID2, writeAPIKey2);
              ThingSpeak.setField(4, now);
              ThingSpeak.writeFields(channelID1, writeAPIKey1);
              sendSMS("Student 1 alighted (home) at " + now + ".");
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu1 At Home");
              break;
          }
        }

        // === STUDENT 2 ===
        else if (uidStr == student2UID) {
          student2ScanCount++;
          if (student2ScanCount > 4) student2ScanCount = 1;
          Serial.print("Student 2 RFID Scan #"); Serial.println(student2ScanCount);
          Serial.print("GPS: "); Serial.print(lat, 6); Serial.print(", "); Serial.println(lng, 6);

          switch (student2ScanCount) {
            case 1: {
              ThingSpeak.setField(1, lat); ThingSpeak.setField(2, lng);
              ThingSpeak.writeFields(channelID4, writeAPIKey4);
              ThingSpeak.setField(1, now);
              ThingSpeak.writeFields(channelID3, writeAPIKey3);

              String eta = getETAFromGoogle(lat, lng, SCHOOL_LAT, SCHOOL_LNG);

              // NEW: ETA to school in Field 7
              ThingSpeak.setField(7, eta);
              ThingSpeak.writeFields(channelID3, writeAPIKey3);

              sendSMS("Student 2 boarded (morning) at " + now + ".\nETA at school: " + eta);
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu2 AM Boarded");
              lcd.setCursor(0, 1); lcd.print("ETA:" + eta);
              break;
            }
            case 2:
              ThingSpeak.setField(3, lat); ThingSpeak.setField(4, lng);
              ThingSpeak.writeFields(channelID4, writeAPIKey4);
              ThingSpeak.setField(3, now);
              ThingSpeak.writeFields(channelID3, writeAPIKey3);
              sendSMS("Student 2 alighted (school) at " + now + ".");
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu2 At School");
              break;
            case 3: {
              ThingSpeak.setField(5, lat); ThingSpeak.setField(6, lng);
              ThingSpeak.writeFields(channelID4, writeAPIKey4);
              ThingSpeak.setField(2, now);
              ThingSpeak.writeFields(channelID3, writeAPIKey3);

              String eta = getETAFromGoogle(lat, lng, HOME_LAT_STUDENT2, HOME_LNG_STUDENT2);

              // NEW: ETA to home in Field 8
              ThingSpeak.setField(8, eta);
              ThingSpeak.writeFields(channelID3, writeAPIKey3);

              sendSMS("Student 2 boarded (afternoon) at " + now + ".\nETA at home: " + eta);
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu2 PM Boarded");
              lcd.setCursor(0, 1); lcd.print("ETA:" + eta);
              break;
            }
            case 4:
              ThingSpeak.setField(7, lat); ThingSpeak.setField(8, lng);
              ThingSpeak.writeFields(channelID4, writeAPIKey4);
              ThingSpeak.setField(4, now);
              ThingSpeak.writeFields(channelID3, writeAPIKey3);
              sendSMS("Student 2 alighted (home) at " + now + ".");
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Stu2 At Home");
              break;
          }
        }

        else {
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("Unknown RFID");
          lcd.setCursor(0, 1); lcd.print(uidStr.substring(0, 16));
          Serial.println("Unknown RFID UID: " + uidStr);
        }

        delay(1000);
      }
    }
  }

  delay(100);
}