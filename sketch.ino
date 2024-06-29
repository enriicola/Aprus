//https://docs.espressif.com/projects/esp-idf/en/stable/esp32/hw-reference/esp32/get-started-devkitc.html
#include <ESP32Servo.h>
#include <MKL_HCSR04.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define servo_pin1 33
#define servo_pin2 32
#define ldr_pin 34
#define led_pin 22
#define moisture_pin 25
#define hc_trigger 12
#define hc_echo 14
#define dht_pin 4
#define rele_pin 15

// Replace with your network credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Replace with your MQTT broker details
const char* mqttServer = "mqtt.eclipseprojects.io";
const int mqttPort = 1883;

Servo Servo1;       
Servo Servo2;

MKL_HCSR04 hc(hc_trigger, hc_echo);

DHT dht(dht_pin, DHT22);

void setup()
{
  // Serial
  Serial.begin(9600);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up MQTT client
  client.setServer(mqttServer, mqttPort);

  // LightSensor
  pinMode(ldr_pin, INPUT);
  // Light Bulb
  pinMode(led_pin, OUTPUT);
  // Servo
  Servo1.attach(servo_pin1);
  Servo2.attach(servo_pin2);

  // Temperature and Humidity Sensor
  dht.begin();
  
  pinMode(rele_pin, OUTPUT);
  digitalWrite(rele_pin, LOW);

}

void led() {
  
  if (analogRead(ldr_pin) > 500)
      digitalWrite(led_pin, 0);
  else
      digitalWrite(led_pin, 1);
}

void temperature (){
  
  float humi = dht.readHumidity();
  float temp = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("ºC ");
  Serial.print("Humidity: ");
  Serial.println(humi);

}

void servo() {
  
  if (analogRead(ldr_pin) > 500) {
      Servo1.write(90);
      Servo2.write(90);
  } else {
      Servo1.write(0);
      Servo2.write(0);
  }
  
}

void moisture() {
  Serial.print("Moisture: ");
  Serial.println(analogRead(moisture_pin));
}

void watertank() {
  Serial.print("Tank: ");
  Serial.println(hc.dist());

  if (hc.dist() < 40) {
      digitalWrite(rele_pin, HIGH);
  } else {
      digitalWrite(rele_pin, LOW);
  }
}

void mqtt() {
  
  client.publish("aprus", "ciao");


}

void loop()
{ 
  Serial.println("\nInit");
  led();
  temperature();
  servo();
  moisture();
  watertank();
  mqtt();
  delay(5000);
}
