#include <ESP8266WiFi.h>

// Network SSID
const char* ssid = "SHIRO_2G";
const char* password = "Paddy_55";
const int connectionWaitTime = 500;
const int connectionTimeout = 10000;

// I/O consts
int relayPin = D3;

void initSerial() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
}

void initIO() {
  Serial.println("Initializing I/O");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(D3, OUTPUT);
  digitalWrite(D3, LOW);
}

void ledOn() {
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void blinkLed() {
  ledOn();
  delay(200);
  ledOff();
}

void connectWiFi() {
  int totalWait = 0;
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("Name");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && totalWait < connectionTimeout) {
    ledOn();
    delay(connectionWaitTime);
    ledOff();
    delay(100);
    Serial.print(".");
    totalWait += connectionWaitTime;
  }
  if (totalWait >= connectionTimeout) {
     Serial.println("");
     Serial.println("!!! Error connecting to WiFi !!!");
  } else {
    Serial.println("");
    Serial.println("WiFi connected");
    // Print the IP address
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
  }
}

void pulseIO(int ioName) {
  digitalWrite(D3, HIGH);
  delay(300);
  digitalWrite(D3, LOW);
  //delay(500); //throttling
}


void setup() {
  initSerial();
  initIO();
  connectWiFi();
}


// the loop function runs over and over again forever
void loop() {
  pulseIO(D3);
  delay(2000);
}
