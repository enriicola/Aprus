// TODO add something to work on https://thingspeak.com

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

   // initially roof is closed
   servo1.write(90);
   servo2.write(90);

   // Initialize DHT sensor
   dht.begin(); 

   connect_mqtt();
}

// getter functions
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

float get_external_humidity() {
   return dht.readHumidity();
}

float get_external_temperature() {
   return dht.readTemperature();
}

float get_moisture() {
   return analogRead(MOISTURE_PIN);
}

float get_water_tank_level() {
   return hc.dist();
}
float get_soil_ph() { // TODO check if it makes sense
   Value = analogRead(potPin);
   float voltage = Value*(3.3/4095.0);
   return 3.3*voltage;
   /*
   int analogValue = analogRead(pHSensorPin);
   float voltage = analogValue * (3.3 / 4095.0); // 3.3V reference, 12-bit ADC
   float pHValue = (voltage * 14.0) / 3.3; // Declare pHValue here
   
   */
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

// if there is too much sunlight, turn on the white LED
void check_sunlight() {
   if (get_lux() > 50)
      digitalWrite(LED_PIN, LOW);
   else
      digitalWrite(LED_PIN, HIGH);
}

// if there is too much sunlight, close the roof
void manage_roof() {
   if (get_lux() > 80000) {
      servo1.write(0);
      servo2.write(0);
   }
   else {
      servo1.write(90);
      servo2.write(90);
   }
}

void manage_watering() {
   // digitalWrite(RELAY_PIN, (get_water_tank_level() > 70 && get_moisture() < 80) ? HIGH : LOW);
   
   // se l'acqua dista più di 70 cm e l'umidità è minore dell'80% ...
   if (get_water_tank_level()<70 && get_moisture()<80)
      digitalWrite(RELAY_PIN, HIGH); // Attiva l'irrigazione
   else
      digitalWrite(RELAY_PIN, LOW); // Disattiva l'irrigazione
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
   doc["humidity"] = get_external_humidity();
   doc["temperature"] = get_external_temperature();
   doc["moisture"] = moisture;
   doc["tankDistance"] = tankDistance;

   // Serialize JSON document to a string
   char buffer[256];
   size_t n = serializeJson(doc, buffer);

   // Publish the JSON string to the MQTT topic
   client.publish(TOPIC, buffer, n);

   // Serial.println("Sended ;)");
}

// TODO: - [ ] Ph soil sensor (to alert the user if the soil is too acidic or too basic w.r.t. the specific plant's needs)
// implement it to send information and alerts to the dashboard

void loop() {
   check_sunlight();

   manage_roof();
   
   manage_watering();

   float pH = get_soil_ph();
   // Display pH recommendation
  if (pH < 5.5) {
    display.println("pH: Too acidic. Add lime.");
  } else if (pH > 7.5) {
    display.println("pH: Too alkaline. Add sulfur.");
  } else {
    display.println("pH: Optimal range.");
  }

   mqtt();

   delay(1000);
}