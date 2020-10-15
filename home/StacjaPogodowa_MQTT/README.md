# Wheather station with OLED display

It measures temperature, humidity and presure by using BME280. Displays values on OLED display and sends measurements to MQTT server.

## Parts
1. WeMos D1 Mini with ESP8266
![WeMos D1 Mini](parts/WeMos_D1_mini_esp8266.png)
2. OLED display 64x48
![OLED](parts/OLED_SSD1306_I2C.png)
3. BME280 sensor
![BME280 sensor](parts/BME280_I2C_3_3V.png)

## Connections
OLED display sits on top of WeMos module connected by gold pins sockets.
BME280 is directly soldered to WeMos by wires.
![BME280 to WeMos connection](parts/BME280_to_WeMos_connection.png)

Link to the original project:
[Whather station for 25 PLN](https://youtu.be/UA-aKUzlbEM)
