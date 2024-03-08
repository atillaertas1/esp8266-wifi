#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"
#include <EEPROM.h>

#define RELAY_NO true
#define EEPROM_ADDRESS 0

const int relayGPIO = 5;

String ssid;
String password;
String currentSsid;
const char* PARAM_INPUT_1 = "state";
bool tryingToConnect = false;
bool isConnected = false;
unsigned long lastAttemptTime = 0;
const unsigned long attemptInterval = 5000; // 5 saniye

AsyncWebServer server(80);


void saveWifiCredentials(String ssid, String password) {
  EEPROM.begin(512);
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(EEPROM_ADDRESS + i, ssid[i]);
  }
  EEPROM.write(EEPROM_ADDRESS + ssid.length(), '\0');
  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(EEPROM_ADDRESS + ssid.length() + 1 + i, password[i]);
  }
  EEPROM.write(EEPROM_ADDRESS + ssid.length() + 1 + password.length(), '\0');
  EEPROM.commit(); 
  EEPROM.end();  
}

void readWifiCredentials(String& ssid, String& password) {
  EEPROM.begin(512); 
  char buffer[50];
  int i;
  for (i = 0; i < 50; ++i) {
    buffer[i] = EEPROM.read(EEPROM_ADDRESS + i);
    if (buffer[i] == '\0') break;
  }
  buffer[i] = '\0';
  ssid = String(buffer);
  int j;
  for (j = 0; j < 50; ++j) {
    buffer[j] = EEPROM.read(EEPROM_ADDRESS + i + 1 + j);
    if (buffer[j] == '\0') break;
  }
  buffer[j] = '\0';
  password = String(buffer);
  EEPROM.end();  
}

void clearEEPROM() {
  EEPROM.begin(512); 
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(i, 0); 
  }
  EEPROM.commit();
  EEPROM.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(relayGPIO, OUTPUT);
  digitalWrite(relayGPIO, !RELAY_NO); 
  readWifiCredentials(ssid, password);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP8266-Config", "password");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid") && request->hasParam("password")) {
      ssid = request->getParam("ssid")->value();
      password = request->getParam("password")->value();
      tryingToConnect = false;
      isConnected = false;
      request->send(200, "text/plain", "Bağlantı isteği alındı. Bağlanıyor...");
    } else {
      request->send(400, "text/plain", "Hata: Eksik parametreler.");
    }
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1)) {
      String state = request->getParam(PARAM_INPUT_1)->value();
      digitalWrite(relayGPIO, (RELAY_NO ? !state.toInt() : state.toInt()));
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "No message sent");
    }
  });

  server.on("/wifiName", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(request->hasParam("ssid")){
    currentSsid = request->getParam("ssid")->value();
    Serial.print("Gelen SSID: ");
    Serial.println(currentSsid);
      if(currentSsid.equals(ssid)) {
        request->send(200, "text/plain", "Wifi ismi alın...");
      }
    }
    else{
      Serial.println("SSID parametresi eksik.");
      request->send(400, "text/plain", "Hata: SSID parametresi eksik.");
    }
  });

  server.begin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && !isConnected) {
    if (!tryingToConnect || millis() - lastAttemptTime > attemptInterval) {
      clearEEPROM();
      saveWifiCredentials(ssid, password);
      Serial.println("Bağlanıyor: " + ssid);
      WiFi.begin(ssid.c_str(), password.c_str());
      lastAttemptTime = millis();
      tryingToConnect = true;
    }
  } else if (tryingToConnect) {
    Serial.println("Bağlantı başarılı: " + WiFi.localIP().toString());
    tryingToConnect = false;
    isConnected = true;
    clearEEPROM();
    saveWifiCredentials(ssid, password);
    WiFi.softAPdisconnect(true);
  }
}
