#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BME280I2C.h>

//Przygotował dla Was Piotrek - piotrek@stirling.fc.pl

int progres = 0;
bool errorOccured = false;
unsigned long pomiarTime = 0;
int sensorStatus = 0;
float temp(NAN), hum(NAN), pres(NAN);



   /////////////////////////////////////////
  ////////   SENSOR
 /////////////////////////////////////////

BME280I2C bme; 

   /////////////////////////////////////////
  ////////   USTAWIENIA OLED
 /////////////////////////////////////////

#define SCREEN_WIDTH 128 //szerokosc ekranu
#define SCREEN_HEIGHT 48 //wysokosc ekranu

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display(OLED_RESET);



   /////////////////////////////////////////
  ////////   USTAWIENIA WIFI I SERWERA
 /////////////////////////////////////////
 
typedef struct {
  char APssid[64];
  char APpassword[64];
  char STAmyssid[64];
  char STAmypassword[64];
} settings_t __attribute__ ((packed));

settings_t settings
{
    "StacjaPogodowa",
    "12345678",
    "a",
    "a"
};

bool APConnected = false;
//ESP.reset()  
ESP8266WebServer server(80); //Server on port 80


   /////////////////////////////////////////
  ////////   SETUP
 /////////////////////////////////////////
void setup(void){
  Serial.begin(9600);
  Serial.println("");
  WiFi.mode(WIFI_AP_STA); //tryb AP + STA

  //readInitSettings(); //czytanie ustawien wifi
  initOledDisplay(); //startowanie wyswietlacza
  //startWifiServices(); //polaczenie z wifi 
  displaySettings(); //pokazanie danych na ekranie 
  handleRequests(); //start serwera www
  initSensors(); //inicjacja sensorów
}

   /////////////////////////////////////////
  ////////   LOOP
 /////////////////////////////////////////
 
void loop(void){
  server.handleClient();          //Handle client requests
  handleOledTimer();
}



    /////////////////////////////////////////
   /////////////////////////////////////////
  ////////   SIEĆ I SERWER WWW
 /////////////////////////////////////////
/////////////////////////////////////////

   /////////////////////////////////////////
  ////////   START SERWISÓW WIFI
 /////////////////////////////////////////

void startWifiServices() {


  WiFi.softAP(settings.APssid, settings.APpassword);  //Start HOTspot removing password will disable security
  WiFi.begin(settings.STAmyssid, settings.STAmypassword); //polaczenie z AP
  
  int countWifi = 0;
  displayOledInfo("WiFi start","prosze","czekac");
  progres = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(330);
    Serial.print(".");
    progressBarAdd();
    countWifi+=1;
    if (countWifi>60) {
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    APConnected = true;  
    IPAddress STAIP = WiFi.localIP();
    Serial.print("HotSpt IP:");
    Serial.println(STAIP);  
    displayOledInfo("Status Wifi","OK","polaczony...");  
  } else {
    Serial.print("HotSpt NOT CONNECTED");
    displayOledInfo("BLAD Wifi","polacz sie","z HotSpot");
    errorOccured = true;
    delay(6000);
  }
 
  IPAddress APIP = WiFi.softAPIP();
  
  Serial.print("HotSpt IP:");
  Serial.println(APIP);  
}

   /////////////////////////////////////////
  ////////  START SERWERA WWW I OBSLUGA REQUESTOW
 /////////////////////////////////////////

void handleRequests() {
  server.on("/", handleRequests_readings); 
  server.on("/wifiList", handleRequests_WiFi); 
  server.on("/connectToNetwork", handleRequests_connect); 
  server.on("/data", handleRequests_json); 
 
  server.begin();                  //Start server
  Serial.println("HTTP server started");  
}

   /////////////////////////////////////////
  ////////   JSON
 /////////////////////////////////////////
 
void handleRequests_json() {

  int numberOfNetworks = WiFi.scanNetworks();
  String page = "{\"odczyty\": {";
  page.concat("\"temperatura\": \""+getTemperatureCelcius()+"\",");
  page.concat("\"wilgotnosc\": \""+getHumidityPercent()+"\",");
  page.concat("\"cisnienie\": \""+getPressurePa()+"\"");
  page.concat("}}");

  server.send(200, "text/json", page);
}

   /////////////////////////////////////////
  ////////   STRONA GLOWNA
 /////////////////////////////////////////
 
void handleRequests_readings() {

  int numberOfNetworks = WiFi.scanNetworks();
  String page = pageHeader("Odczyty");
  page.concat("temperatura: "+getTemperatureCelcius()+"°C<br>");
  page.concat("wilgotność: "+getHumidityPercent()+"%<br>");
  page.concat("ciśnienie: "+getPressurePa()+"hPa<br>");
  page.concat(pageFooter());

  server.send(200, "text/html", page);
}



   /////////////////////////////////////////
  ////////   STRONA Z WYBOREM AP
 /////////////////////////////////////////
 
void handleRequests_WiFi() {

  int numberOfNetworks = WiFi.scanNetworks();
  String wifiList = pageHeader("Lista sieci WiFi");
  for(int i =0; i<numberOfNetworks; i++){ 
      wifiList.concat("<br><br><br><span style=\"color:gray\">Nazwa sieci:</span> <span style=\"font-weight:bold\">"+WiFi.SSID(i)+"</span><br>");
      wifiList.concat("<br><a class=\"button\" href=\"/connectToNetwork?ssid="+WiFi.SSID(i)+"\">połącz z tą siecią</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"+paintSignalStrength(WiFi.RSSI(i))); 
  }
  wifiList.concat(pageFooter());

  server.send(200, "text/html", wifiList);
}

   /////////////////////////////////////////
  ////////   STRONA Z WYBOREM AP
 /////////////////////////////////////////

void handleRequests_connect() {
  String body = "";
  for (int i = 0; i < server.args(); i++) { 
    if(server.argName(i)=="ssid") {
      Serial.println("Podaj formularz");
      body += pageHeader("Łączenie z siecią: "+server.arg(i));  
      body += " <form method=\"post\" action=\"/connectToNetwork?connecttossid=1\"> nazwa sieci:<br><input type=\"text\" name=\"newssid\" value=\""+server.arg(i)+"\"><br>hasło:<br><input type=\"password\" name=\"password\" value=\"\"><br><input type=\"submit\" value=\"Połącz\"></form>";
      body += pageFooter();
      server.send(200, "text/html", body);
      return;
    } else if(server.argName(i)=="connecttossid") {
      Serial.println("Inicjacja zmiany parametrów wifi: ");
      body += pageHeader("Łączenie z siecią: "+server.arg(i));
      String ssid = "";
      String pass = "";  
      for (int j = 0; j < server.args(); j++) { 
        if(server.argName(j)=="newssid") ssid = server.arg(j);
        else if(server.argName(j)=="password") pass = server.arg(j);
      }
      Serial.println("Paramsy: "+ssid+" oraz "+pass);

      
      char buf1[64];
      char buf2[64];
      ssid.toCharArray(settings.STAmyssid, 64);
      pass.toCharArray(settings.STAmypassword, 64);

      //settings.STAmyssid = buf1;
      //settings.STAmypassword = buf2;

      Serial.println("Nowy ssid z paramsow: |"+(String)settings.STAmyssid+"|");
      storeSettings(&settings, sizeof(settings));
      loadSettings(&settings, sizeof(settings));
      Serial.println("Po reloadzie: "+(String)settings.STAmyssid+"|");
      
      
      body += "storing "+ssid+" oraz "+pass+" ok" ;
      body += pageFooter();
      server.send(200, "text/html", body);
      return;      
    }
  } 
}

   /////////////////////////////////////////
  ////////   del
 /////////////////////////////////////////

void todeletehandleArgs() {

  String message = pageHeader("Test parametrów");
  message += server.args();            //Get number of parameters
  message += "\n";                            //Add a new line
  
  for (int i = 0; i < server.args(); i++) { 
    message += "Arg nr" + (String)i + " –> ";   //Include the current iteration value
    message += server.argName(i) + ": ";     //Get the name of the parameter
    message += server.arg(i) + "\n";              //Get the value of the parameter
  } 
  message += pageFooter();

  server.send(200, "text/html", message);
}

   /////////////////////////////////////////
  ////////   RYSOWANIE PASKA Z SILA WIFI
 /////////////////////////////////////////

String paintSignalStrength(int rssi) {
  if (rssi>-40) rssi=-40;
  else if (rssi<-100) rssi=-100;
  rssi=-rssi-40;
  int redColor = rssi*255/60;
  int greenColor = (60-rssi)*255/60;
  int widthpx = (60-rssi)+20;
  String strip = "<span style=\"font-size:9px;display:inline-block;width:"+(String)widthpx+"px;height:25px;background-color:rgb("+(String)redColor+","+(String)greenColor+",0); \"></span>";
  return strip;
}

   /////////////////////////////////////////
  ////////   RYSOWANIE NAGLOWKA STRONY
 /////////////////////////////////////////

String pageHeader(String title) {
  String message = "<!doctype html>";
  message += "<head><meta charset=\"UTF-8\"><title>Stacja pogodowa - "+title+"</title>";
  message += "<style>body{font-family:Verdana,Arial,Helvetica;color:black;background:white;margin:0;}.button{text-decoration:none; color:black; font-weight:bold; display:inline-block;padding:3px;margin:2px; border:1px solid gray; background-color:oldlace}h1{margin:0; display:block;font-size:20px; padding:20px; background-color:gainsboro; border-width:0 0 2px 0; border-style:dotted; border-color:lightslategray}.content{margin:20px 10px 10px 10px; font-size:12px;}";
  message += ".footer{margin:0; display:block;padding:20px; background-color:gainsboro; border-width:2px 0 0 0; border-style:dotted; border-color:lightslategray}.footer a{text-decoration:none; color:black; font-weight:bold; display:inline-block;padding:3px;margin:2px; border:1px solid gray; background-color:gray}";
  message += "</style>";
  message += "</head>";
  message += "<body>";
  if (WiFi.status() != WL_CONNECTED) {
    message += "<h3 style=\"font-weight:bold; display:block; margin:0; padding:10px; font-size:10px; color:white; background-color:red\">Uwaga! Nie jesteś połączony z żadną siecią WiFi, przejdź do wyboru sieci. Możesz jednak również korzystać ze stacji w trybie AP, tak jak teraz, choć nie wydaje się to zbyt wygodnym rozwiązaniem.</h3>";  
  }
  message += "<h1>"+title+"</h1><p class=\"content\">";
  return message;
}

   /////////////////////////////////////////
  ////////   RYSOWANIE STOPKI STRONY
 /////////////////////////////////////////

String pageFooter() {
  String message = "<div class=\"footer\"><a href=\"/\">strona główna</a><a href=\"/wifiList\">połącz się z siecią WiFi</a></div>";
  message += "</p></body></html>";
  return message;
}







    /////////////////////////////////////////
   /////////////////////////////////////////
  ////////   EEPROM I SETTINGSY
 /////////////////////////////////////////
/////////////////////////////////////////


   /////////////////////////////////////////
  ////////   POZYSKANIE USTAWIEN ZE STORAGE
 /////////////////////////////////////////

void readInitSettings() {
  settings_t settings_original;
  memcpy(&settings_original, &settings, sizeof(settings));
  //storeSettings(&settings, sizeof(settings));
  loadSettings(&settings, sizeof(settings));
  Serial.print("new: "+(String)settings.APssid);
  Serial.print("orginal: "+(String)settings_original.APssid);
  if((String)settings.APssid != (String)settings_original.APssid) {
    memcpy(&settings, &settings_original, sizeof(settings));  
    Serial.print("Pierwsze uruchomienie, brak danych o WiFi");
  } else {
    Serial.print("Odczytano dane WiFi z EEPROM");
  }
}

   /////////////////////////////////////////
  ////////   ZAPISANIE DANYCH W EEPROM
 /////////////////////////////////////////

void storeSettings(void *data_source, size_t size)
{
  EEPROM.begin(size * 2);
  for(size_t i = 0; i < size; i++)
  {
    char data = ((char *)data_source)[i];
    EEPROM.write(i, data);
  }
  EEPROM.commit();
}

   /////////////////////////////////////////
  ////////   WCZYTANIE DANYCH Z EEPROM
 /////////////////////////////////////////

void loadSettings(void *data_dest, size_t size)
{
    EEPROM.begin(size * 2);
    for(size_t i = 0; i < size; i++)
    {
        char data = EEPROM.read(i);
        ((char *)data_dest)[i] = data;
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
  ////////   DODANIE ZNAKU (PROGRES)
 /////////////////////////////////////////

void progressBarAdd() {
  display.drawLine(progres, 38, progres, 47, WHITE);
  display.display();
  progres+=1;
  
}

   /////////////////////////////////////////
  ////////   SHOW INIT SETTINGS
 /////////////////////////////////////////

 void displaySettings() {
  display.clearDisplay(); // Clear display buffer

  display.setTextSize(1);             // Normal 1:1 pixel scale

  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Ustawienia sieci:"));
  
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,11);             // Start at top-left corner
  display.print(F("SSID:"));
  display.println((String)settings.APssid);
  display.print(F("haslo:"));
  display.println((String)settings.APpassword);
  display.print(F("adres:"));
  display.println(WiFi.softAPIP());
  display.setCursor(0,38);  
  display.print(F("adres2:"));
  display.println(WiFi.localIP());
  //display.println(F("haslo: "+(String)settings.APpassword));
  //display.println(F("strona: "+(String)WiFi.softAPIP()));

  display.display();
  display.startscrollleft(0x00, 0x0F);

  if (errorOccured) delay(20000);
  else delay(8000);
  display.stopscroll();
 }

   /////////////////////////////////////////
  ////////   WYPISANIE ODCZYTOW
 /////////////////////////////////////////

 void paintOledReadings() {
  display.clearDisplay(); 
  display.setTextSize(2); 

  display.setTextColor(WHITE);
  display.setCursor(6,0);  
  display.println(getTemperatureCelcius());
  display.drawCircle(38, 3, 3, WHITE);
  display.setCursor(45,0);  
  display.println("C");

  display.setTextSize(1); 
  display.setCursor(12,26);
  display.println(getHumidityPercent()+" %");

  display.setTextSize(1); 
  display.setCursor(12,40);  
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

 int getTemperatureCelciusNumber() {
  return (int)temp; 
 }

 int getPressurePaNumber() {
  return pres; 
 }

 int getHumidityPercentNumber() {
  return hum; 
 }
 String getTemperatureCelcius() {
  return (String)getTemperatureCelciusNumber();
 }

 String getPressurePa() {
  return (String)getPressurePaNumber();
 }

 String getHumidityPercent() {
  return (String)getHumidityPercentNumber();
 }

     /////////////////////////////////////////
   /////////////////////////////////////////
  ////////   GLOBALNE
 /////////////////////////////////////////
/////////////////////////////////////////

   /////////////////////////////////////////
  ////////   TIMER
 /////////////////////////////////////////

 void handleOledTimer() {   
    if (millis() - pomiarTime >= 10000UL) {
      pomiarTime = millis();
      readValuesFromSensor();
      paintOledReadings();
    }
 }
