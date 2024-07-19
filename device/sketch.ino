// TODO add something to work on https://thingspeak.com
// TODO implement a id for every id (define id etc)
// TODO implement receiving of various threshold from mqtt server and use them as variables
  // Handle Incoming MQTT Messages
// TODO Use Thresholds given by mqtt server in Your Logic
// TODO additional improvements (	•	Error Handling: Ensure error handling for sensor readings and MQTT operations is robust. •	Power Management: If power consumption is a concern, consider implementing sleep modes or optimizing sensor polling intervals. •	Comments and Documentation: Add more comments to clarify the purpose of each function and variable.)



#include <ESP32Servo.h>
#include <MKL_HCSR04.h> // for ultrasonic sensor
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h> // for DS18B20
#include <DallasTemperature.h> // for DS18B20

// Pin definitions
#define SERVO_PIN1 33
#define SERVO_PIN2 32
#define LDR_PIN 34
#define LED_PIN 22
#define MOISTURE_PIN 35
#define SOIL_TEMP_PIN 25

#define PH_PIN 2 // TODO: does not read anything

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

// implement struct for threshold values
struct Thresholds
{
  float lux;
  float moisture;
  float tankDistance;
};


// Device objects
Servo servo1;
Servo servo2;
MKL_HCSR04 hc(HC_TRIGGER, HC_ECHO);
DHT dht(DHT_PIN, DHT22);
WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(SOIL_TEMP_PIN); // for DS18B20
DallasTemperature ds(&oneWire);  // for DS18B20

void setup()
{
  // Initialize Serial
  Serial.begin(9600);

  // Connect to Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up MQTT client
  client.setServer(MQTT_SERVER, MQTT_PORT);

  // Pin modes
  pinMode(LDR_PIN, INPUT);                 // initialize LDR (Light Dependent Resistor)
  pinMode(LED_PIN, OUTPUT);                // initialize LED to check sunlight
  pinMode(MOISTURE_PIN, INPUT);            // Initialize moisture sensor
  pinMode(CONNECTION_SUCCESS_LED, OUTPUT); // Initialize connection success LED
  pinMode(CONNECTION_FAILURE_LED, OUTPUT); // Initialize connection failure LED
  pinMode(RELAY_PIN, OUTPUT);              // Initialize ultrasonic sensor
  pinMode(SOIL_TEMP_PIN, INPUT);           // Initialize soil temperature sensor
  pinMode(PH_PIN, INPUT);                  // Initialize pH sensor

  digitalWrite(LED_PIN, LOW);   // turn off LED, initially
  digitalWrite(RELAY_PIN, LOW); // turn off relay, initially

  servo1.attach(SERVO_PIN1); // attach servo to pin
  servo2.attach(SERVO_PIN2); // attach servo to pin

  // initially roof is closed
  servo1.write(90);
  servo2.write(90);

  // Initialize DHT and DS18B20 sensors
  dht.begin();
  ds.begin();

  connect_mqtt();
}

// getter functions
float get_lux()
{
  const float GAMMA = 0.7;
  const float RL10 = 50;

  int analogValue = analogRead(LDR_PIN);
  analogValue = map(analogValue, 4095, 0, 1024, 0);

  float voltage = analogValue / 1024.0 * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  return lux;
}
float get_external_humidity()
{
  return dht.readHumidity();
}
float get_external_temperature()
{
  return dht.readTemperature();
}
float get_moisture()
{
  return analogRead(MOISTURE_PIN);
}
float get_soil_temp()
{
  ds.requestTemperatures();
  return ds.getTempCByIndex(0);
}
float get_water_tank_level()
{
  return hc.dist();
}
float get_soil_ph()
{
  // TODO: If the pH sensor isn’t reading anything, verify its connection and calibration. You may need to calibrate or replace the sensor. For debugging, print raw values from the pH sensor to confirm it’s receiving input.
  float get_soil_ph()
  {
    int rawValue = analogRead(PH_PIN);
    Serial.print("Raw pH value: ");
    Serial.println(rawValue);
    return ((float)rawValue / 4095.0) * 14;
  }
}

void connect_mqtt()
{
  if (client.connect(CLIENT_ID))
  {
    client.subscribe(TOPIC);
    client.subscribe("thresholds"); // TODO check if it works
    client.publish(TOPIC, "Connection succesfull");
    digitalWrite(CONNECTION_SUCCESS_LED, HIGH);
    digitalWrite(CONNECTION_FAILURE_LED, LOW);
  }
  else
  {
    digitalWrite(CONNECTION_FAILURE_LED, HIGH);
    reconnect();
  }
}

// virtual infinite loop to reconnect to MQTT
void reconnect()
{
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(CLIENT_ID))
    {
      Serial.println("connected");
      client.publish(TOPIC, "Nodemcu connected to MQTT");
      client.subscribe(TOPIC);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// if there is too much sunlight, turn on the white LED
void check_sunlight()
{
  if (get_lux() < 20)
    Serial.println("Too dark");
  if (get_lux() > 50)
    digitalWrite(LED_PIN, LOW);
  else
    digitalWrite(LED_PIN, HIGH);
}

// if there is too much sunlight, close the roof
void manage_roof()
{
  if (get_lux() > 80000)
  {
    servo1.write(0);
    servo2.write(0);
  }
  else
  {
    servo1.write(90);
    servo2.write(90);
  }
}

void manage_watering()
{
  // digitalWrite(RELAY_PIN, (get_water_tank_level() > 70 && get_moisture() < 80) ? HIGH : LOW);

  // se l'acqua dista più di 70 cm e l'umidità è minore dell'80% ...
  if (get_water_tank_level() < 70 && get_moisture() < 80)
    digitalWrite(RELAY_PIN, HIGH); // Attiva l'irrigazione
  else
    digitalWrite(RELAY_PIN, LOW); // Disattiva l'irrigazione
}

void mqtt()
{
  // Gather sensor data
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float tankDistance = hc.dist();

  // Create JSON document
  StaticJsonDocument<256> doc;
  doc["lux"] = get_lux();
  doc["humidity"] = get_external_humidity();
  doc["temperature"] = get_external_temperature();
  doc["moisture"] = get_moisture();
  doc["soilTemperature"] = get_soil_temp();
  doc["pH"] = get_soil_ph();
  doc["tankDistance"] = tankDistance;

  // Serialize JSON document to a string
  char buffer[256];
  size_t n = serializeJson(doc, buffer);

  // Publish the JSON string to the MQTT topic
  client.publish(TOPIC, buffer, n);

  // Serial.println("Sended ;)");
}

// TODO: to be implemented
// Initialize default thresholds
Thresholds thresholds = { 1000.0, 50.0, 70.0 }; // Example default values

void mqtt_for_thresholds(const String& payload) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Failed to parse thresholds: ");
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("lux")) {
    thresholds.lux = doc["lux"];
  }
  if (doc.containsKey("moisture")) {
    thresholds.moisture = doc["moisture"];
  }
  if (doc.containsKey("tankDistance")) {
    thresholds.tankDistance = doc["tankDistance"];
  }

  Serial.print("Updated thresholds: Lux=");
  Serial.print(thresholds.lux);
  Serial.print(", Moisture=");
  Serial.print(thresholds.moisture);
  Serial.print(", Tank Distance=");
  Serial.println(thresholds.tankDistance);
}

// TODO check if it works, implement receiving of various threshold from mqtt server and use them as variables
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "thresholds") {
    mqtt_for_thresholds(message);
  } else if (String(topic) == TOPIC) {
    // Handle other topics if needed
  }
}

void loop()
{
  check_sunlight();

  manage_roof();

  manage_watering();

  Serial.print("\n\nph: ");
  Serial.print(get_soil_ph());
  
  Serial.print("\nmoisture: ");
  Serial.print(get_moisture());

  Serial.print("\nsoil temperature: ");
  Serial.print(get_soil_temp());

  mqtt();

  delay(1000);
}