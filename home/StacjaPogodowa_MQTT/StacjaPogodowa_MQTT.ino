#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BME280I2C.h>

#define MQTT_SERVER_PORT 1883

const char* ssid = "SHIRO_2G_EXT";
const char* password = "Paddy_55";
const char* serverHostname = "raspberrypi";
const char* clientId = "TempSensor2";
const char* logTopic = "home/log";
const char* outTopicReadings = "home/sensor2/readings";
const char* inGetReadingsTopic = "home/sensor2/read";

bool errorOccured = false;
unsigned long pomiarTime = 0;
int sensorStatus = 0;
float temp(NAN), hum(NAN), pres(NAN);

BME280I2C bme;  // sensor 
WiFiClient espClient;
PubSubClient client(espClient);

   /////////////////////////////////////////
  ////////   USTAWIENIA OLED
 /////////////////////////////////////////

#define SCREEN_WIDTH 128 //szerokosc ekranu
#define SCREEN_HEIGHT 48 //wysokosc ekranu

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display(OLED_RESET);


   /////////////////////////////////////////
  ////////   SETUP
 /////////////////////////////////////////
void setup(void){
  initSerial();
  connectWifi();
  client.setServer(serverHostname, MQTT_SERVER_PORT); // connect to MQTT server
  client.setCallback(callback);
  initOledDisplay(); //startowanie wyswietlacza
  displaySettings();
  initSensors(); //inicjacja sensorów
  readAndPublish(); // initial readings
}

   /////////////////////////////////////////
  ////////   LOOP
 /////////////////////////////////////////
 
void loop(void){
  handleTimer();
}

void initSerial() {
    Serial.begin(115200);
    delay(10);
    Serial.println();
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
            String cc = String(clientId);
            client.publish(logTopic, String(cc + " connected").c_str());
            client.subscribe(inGetReadingsTopic);
        }
        else {
            Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
            delay(2500);
        }
    }
}

void callback(char* topic, byte* msgPayload, unsigned int msgLength) {
    Serial.printf("topic %s, message received\n", topic);

    if (strcmp(topic, inGetReadingsTopic) == 0) {
        readAndPublish();
    }
}

    /////////////////////////////////////////
   /////////////////////////////////////////
  ////////   WYSWIETLACZ
 /////////////////////////////////////////
/////////////////////////////////////////

   /////////////////////////////////////////
  ////////   INICJALIZACJA OLEDA
 /////////////////////////////////////////

void initOledDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}


   /////////////////////////////////////////
  ////////   WYSWIETLENIE INFO
 /////////////////////////////////////////

void displayOledInfo(String header, String line1, String line2) {
  display.clearDisplay(); // Clear display buffer
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(header);
  display.setTextColor(WHITE);        // Draw white text

  display.setCursor(0,16);   
  display.println(line1);
  display.setCursor(0,25);   
  display.println(line2);
  display.display();
}

   /////////////////////////////////////////
  ////////   SHOW INIT SETTINGS
 /////////////////////////////////////////

 void displaySettings() {
  display.clearDisplay(); // Clear display buffer

  display.setTextSize(1);             // Normal 1:1 pixel scale

  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Ustawienia"));
  
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0, 11);             // Start at top-left corner  
  display.println(F("adres IP:"));
  display.setCursor(0, 22);
  display.print(WiFi.localIP());
  
  display.display();
  //display.startscrollleft(0x00, 0x04);

  if (errorOccured) {
      delay(20000);
  } else {
      delay(8000);
  }
  //display.stopscroll();
 }

   /////////////////////////////////////////
  ////////   WYPISANIE ODCZYTOW
 /////////////////////////////////////////

 void paintOledReadings() {
  display.clearDisplay(); 
  display.setTextSize(2); 

  display.setTextColor(WHITE);
  display.setCursor(2, 0);  
  display.println(getTemperatureCelcius());
  display.drawCircle(56, 4, 4, WHITE);
  display.drawCircle(56, 4, 3, WHITE);
  //display.setCursor(52, 0);  
  //display.println("C");

  display.setTextSize(1); 
  display.setCursor(12, 26);
  display.println(getHumidityPercent()+" %");

  display.setTextSize(1); 
  display.setCursor(8, 40);  
  display.println(getPressurePa()+" hPa");

  display.display();
   
 }

 
    /////////////////////////////////////////
   /////////////////////////////////////////
  ////////   CZUJNIKI
 /////////////////////////////////////////
/////////////////////////////////////////

   /////////////////////////////////////////
  ////////   INICJALIZACJA CZUJNIKOW
 /////////////////////////////////////////

 void initSensors() {
  if(!bme.begin())
  {
    Serial.println("Nie odnaleziono czujnika BME280!");
    sensorStatus = 0;
  }
  else {
    switch(bme.chipModel())
    {
       case BME280::ChipModel_BME280:
         Serial.println("Odnaleziono czujnik BME280.");
         sensorStatus = 3;
         break;
       case BME280::ChipModel_BMP280:
         Serial.println("Odnaleziono czujnik BMP280. Odczyt wilgotnosci nie bedzie dostepny.");
         sensorStatus = 2;
         break;
       default:
         Serial.println("Blad! Odnaleziono nieznany czujnik.");
         sensorStatus = 1;
    }
  }
  if (sensorStatus<2) {
    displayOledInfo("BLAD1","brak","sensora!");
    errorOccured = true;
    delay(6000);
  }
 }

   /////////////////////////////////////////
  ////////   POBRANIE DANYCH Z CZUJNIKOW
 /////////////////////////////////////////
 
 void readValuesFromSensor() {
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);

   bme.read(pres, temp, hum, tempUnit, presUnit);
 }

 String getTemperatureCelcius() {
     return String(temp, 1);
 }

 String getPressurePa() {
     return String(pres, 1);
 }

 String getHumidityPercent() {
  return String(hum, 1);
 }

   /////////////////////////////////////////
  ////////   TIMER
 /////////////////////////////////////////

 void handleTimer() {
     if (millis() - pomiarTime >= 60000UL) {
         pomiarTime = millis();
         readAndPublish();
     }
     clientLoop();
     delay(300);
 }

 void readAndPublish() {
     readValuesFromSensor();
     paintOledReadings();
     sendSensorReadings();
 }

 void clientLoop() {
     if (!client.connected()) {
         connectMQTT();
     }
     client.loop();
 }

 void sendSensorReadings() {
     Serial.printf("\nSending readings: %s\n", getTemperatureCelcius().c_str());
     if (!client.connected()) {
         connectMQTT();
     }
     client.publish(outTopicReadings, getTemperatureCelcius().c_str(), true);
     client.loop();
 }
