// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/hw-reference/esp32/get-started-devkitc.html
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
#define connection_success_led 23
#define connection_failure_led 19
#define client_id "Aprus_ESP32_clientID"
#define topic "Aprus"

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

WiFiClient espClient;
PubSubClient client(espClient); //lib required for mqtt

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
  // client.setCallback(callback); //TODO later
  

  // LightSensor
  pinMode(ldr_pin, INPUT);
  // Light Bulb
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // Servo
  Servo1.attach(servo_pin1);
  Servo2.attach(servo_pin2);

  // Temperature and Humidity Sensor
  dht.begin();
  
  pinMode(rele_pin, OUTPUT);
  digitalWrite(rele_pin, LOW);

  pinMode(moisture_pin, INPUT);


  pinMode(connection_success_led, OUTPUT);
  pinMode(connection_failure_led, OUTPUT);
  connect_mqtt();
}

void connect_mqtt()
{
  client.connect(client_id);  // ESP will connect to mqtt broker with clientID
  // Serial.println("connected to MQTT");
  // Once connected, publish an announcement...

  // ... and resubscribe
  client.subscribe(topic); // name of the channel ("subscription" to a topic)
  client.publish(topic,  "dani puzza"); // topic, payload

  if (!client.connected()){
    digitalWrite(connection_failure_led, HIGH);
    reconnect();
  }
  digitalWrite(connection_failure_led, LOW);
  digitalWrite(connection_success_led, HIGH);
    
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(client_id)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic, "Nodemcu connected to MQTT");
      // ... and resubscribe
      client.subscribe(topic);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*  //TODO later
void callback(char* topic, byte* payload, unsigned int length) {   //callback includes topic and payload ( from which (topic) the payload is comming)
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
    Serial.print((char)payload[i]);

  if ((char)payload[0] == 'O' && (char)payload[1] == 'N') //on
  {
    digitalWrite(LED, HIGH);
    Serial.println("on");
    client.publish("esp32/out", "LED turned ON");
  }
  else if ((char)payload[0] == 'O' && (char)payload[1] == 'F' && (char)payload[2] == 'F') //off
  {
    digitalWrite(LED, LOW);
    Serial.println(" off");
    client.publish("esp32/out", "LED turned OFF");
  }
  Serial.println();
}
*/

float get_lux(){
  // These constants should match the photoresistor's "gamma" and "rl10" attributes
  const float GAMMA = 0.7;
  const float RL10 = 50;

  // Convert the analog value into lux value:
  int analogValue = analogRead(ldr_pin);
  float voltage = analogValue / 1024. * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  return lux;
}

void led() {
  if (get_lux() > 500)
      digitalWrite(led_pin, LOW);
  else
      digitalWrite(led_pin, HIGH);
}

void temperature (){
  
  float humi = dht.readHumidity();
  float temp = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("ºC ");
  Serial.println("Humidity: ");
  Serial.print(humi);

}

void servo() {
  
  if (get_lux() > 500) {
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

  if (hc.dist() < 40)
      digitalWrite(rele_pin, HIGH);
  else
      digitalWrite(rele_pin, LOW);
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
