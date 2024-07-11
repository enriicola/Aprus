#include <ESP32Servo.h>
#include <MKL_HCSR04.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Pin definitions
#define SERVO_PIN1 33
#define SERVO_PIN2 32
#define LDR_PIN 34
#define LED_PIN 22
#define MOISTURE_PIN 35
#define HC_TRIGGER 12
#define HC_ECHO 14
#define DHT_PIN 4
#define RELAY_PIN 15
#define CONNECTION_SUCCESS_LED 23
#define CONNECTION_FAILURE_LED 19

// MQTT details
#define CLIENT_ID "Aprus_ESP32_clientID"
#define TOPIC "Aprus"
#define SSID "Wokwi-GUEST"
#define PASSWORD ""
#define MQTT_SERVER "mqtt.eclipseprojects.io"
#define MQTT_PORT 1883

// Device objects
Servo servo1;       
Servo servo2;
MKL_HCSR04 hc(HC_TRIGGER, HC_ECHO);
DHT dht(DHT_PIN, DHT22);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Initialize Serial
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
  client.setServer(MQTT_SERVER, MQTT_PORT);

  // Pin modes
  pinMode(LDR_PIN, INPUT); // initialize LDR (Light Dependent Resistor)
  pinMode(LED_PIN, OUTPUT); // initialize LED to check sunlight
  pinMode(MOISTURE_PIN, INPUT); // Initialize moisture sensor
  pinMode(CONNECTION_SUCCESS_LED, OUTPUT); // Initialize connection success LED
  pinMode(CONNECTION_FAILURE_LED, OUTPUT); // Initialize connection failure LED
  pinMode(RELAY_PIN, OUTPUT); // Initialize ultrasonic sensor

  digitalWrite(LED_PIN, LOW); // turn off LED, initially
  digitalWrite(RELAY_PIN, LOW); // turn off relay, initially
  
  servo1.attach(SERVO_PIN1); // attach servo to pin
  servo2.attach(SERVO_PIN2); // attach servo to pin

  // Initialize DHT sensor
  dht.begin(); 

  connect_mqtt();
}

void connect_mqtt() {
  if (client.connect(CLIENT_ID)) {
    client.subscribe(TOPIC);
    client.publish(TOPIC, "Connection succesfull");
    digitalWrite(CONNECTION_SUCCESS_LED, HIGH);
    digitalWrite(CONNECTION_FAILURE_LED, LOW);
  } 
  else {
    digitalWrite(CONNECTION_FAILURE_LED, HIGH);
    reconnect();
  }
}

// virtual infinite loop to reconnect to MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.publish(TOPIC, "Nodemcu connected to MQTT");
      client.subscribe(TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

float get_lux() {
  const float GAMMA = 0.7;
  const float RL10 = 50;

  int analogValue = analogRead(LDR_PIN);
  analogValue = map(analogValue, 4095, 0, 1024, 0);

  float voltage = analogValue / 1024.0 * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  return lux;
}

void alert_if_low_sunlight() {
  // Serial.print("lux: ");
  // Serial.println(get_lux());

  if (get_lux() > 50) {
    digitalWrite(LED_PIN, LOW);
  }
  else {
    digitalWrite(LED_PIN, HIGH);
  }
}

void adjust_roof_if_high_sunlight() {
  Serial.print("lux for servo: ");
  Serial.println(get_lux());
  if (get_lux() > 1500.0) {
    servo1.write(90);
    servo2.write(90);
  }
  else {
    servo1.write(0);
    servo2.write(0);
  }
}

void temperature() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("ºC ");
  Serial.print("Humidity: ");
  Serial.println(humidity);
}

void moisture() {
  Serial.print("Moisture: ");
  Serial.println(analogRead(MOISTURE_PIN));
}

void watertank() {
  Serial.print("Tank: ");
  Serial.println(hc.dist());

  if (hc.dist() < 40) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
  }
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

  Serial.println("Sended ;)");
}

void loop() {
  Serial.println();
  alert_if_low_sunlight();
  temperature();
  adjust_roof_if_high_sunlight();
  moisture();
  watertank();
  mqtt();
  delay(5000);
}
