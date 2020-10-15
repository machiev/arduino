#include <ESP8266WiFi.h>

const int connectionWaitTimeMs = 500;
const int connectionTimeoutMs = 12000;

void initSerial() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
}

bool connectionWait() {
  int totalWait = 0;
  while (WiFi.status() != WL_CONNECTED && totalWait < connectionTimeoutMs) {
    delay(connectionWaitTimeMs);
    Serial.print(".");
    totalWait += connectionWaitTimeMs;
  }
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Error connecting to WiFi !!!");
    return false;
  }
}

bool connectWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WiFi.SSID());
  WiFi.hostname("Name");
  WiFi.begin();
  return connectionWait();
}

bool startWPS() {
  Serial.println("WPS config start");
  WiFi.mode(WIFI_STA);
  if(WiFi.beginWPSConfig()) {
    return connectionWait();
  }
  return false; 
}

bool checkConnectionAndReconnect() {
  return WiFi.isConnected() ? true : connectWiFi();
}

void setup() {
  initSerial();
  if(connectWiFi() == false) {
    startWPS();
  }
}

// the loop function runs over and over again forever
void loop() {
  delay(2000);
}
