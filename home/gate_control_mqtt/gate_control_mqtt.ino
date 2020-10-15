#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MQTT_SERVER_PORT 1883

const char* ssid = "SHIRO_2G";
const char* password = "Paddy_55";
const char *serverHostname = "raspberrypi";
const char *clientId = "GateController";
const char *logTopic = "home/log";
const char *inGateTopic = "home/gate/full";
const char *inGateHalfTopic = "home/gate/half";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  initSerial();
  initIO();
  connectWifi();
  // connect to MQTT server  
  client.setServer(serverHostname, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  delay(250);
}

void initSerial() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
}

void initIO() {
  Serial.println("Initializing I/O");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void connectWifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.printf("\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected on IP address ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.printf("MQTT connecting as client %s...\n", clientId);
    
    if (client.connect(clientId)) {
      Serial.println("MQTT connected");
      client.publish(logTopic, "Gateway controller connected");
      client.subscribe(inGateTopic);
      client.subscribe(inGateHalfTopic);
    } else {
      Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
      delay(2500);
    }
  }
}

void callback(char *topic, byte *msgPayload, unsigned int msgLength) {
  Serial.printf("topic %s, message received\n", topic);

  if (strcmp(topic, inGateTopic) == 0) {
    ledOn();
    client.publish(logTopic, "Opening gate");
  } else if (strcmp(topic, inGateHalfTopic) == 0) {
    ledOff();
    client.publish(logTopic, "Opening small gate");
  }
}

void ledOn() {
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff() {
  digitalWrite(LED_BUILTIN, HIGH);
}
