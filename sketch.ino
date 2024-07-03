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
  WiFi.begin(SSID, PASSWORD);
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

  if (get_lux() > 50) {
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, HIGH);
  }
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
  Serial.println(analogRead(MOISTURE_PIN));
}

void watertank() {
  Serial.print("Tank: ");
  Serial.println(hc.dist());

<<<<<<< HEAD
  if (hc.dist() < 40)
      digitalWrite(rele_pin, HIGH);
  else
      digitalWrite(rele_pin, LOW);
=======
  if (hc.dist() < 40) {
    digitalWrite(RELE_PIN, HIGH);
  } else {
    digitalWrite(RELE_PIN, LOW);
  }
>>>>>>> 4fc63a3914b2f9e989a7b18068bac916ccee9b5c
}

void mqtt() {
  // Gather sensor data
  float lux = get_lux();
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int moisture = analogRead(MOISTURE_PIN);
  float tankDistance = hc.dist();

  // Create JSON document
  StaticJsonDocument<256> doc;
  doc["lux"] = lux;
  doc["humidity"] = humidity;
  doc["temperature"] = temperature;
  doc["moisture"] = moisture;
  doc["tankDistance"] = tankDistance;

  // Serialize JSON document to a string
  char buffer[256];
  size_t n = serializeJson(doc, buffer);

  // Publish the JSON string to the MQTT topic
  client.publish(TOPIC, buffer, n);
}

void loop() { 
  led();
  temperature();
  servo();
  moisture();
  watertank();
  mqtt();
  delay(5000);
}
