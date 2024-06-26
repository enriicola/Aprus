#include <ESP32Servo.h>
#include <MKL_HCSR04.h>

int servo_pin1 = 33;   // Declare the Servo pin 
int servo_pin2 = 32;
Servo Servo1;        // Create a servo object 
Servo Servo2;

int ldr_pin = 34;//assigning ldr pin no.
int led_pin = 22;//assigning bulb pin no.

int moisture_pin = 25;

int hc_trigger = 25;
int hc_echo = 27;
MKL_HCSR04 hc(hc_trigger, hc_echo);

void setup()
{
  // Serial
  Serial.begin(9600);
    

  // LightSensor
  pinMode(ldr_pin, INPUT);
  	// Light Bulb
  	pinMode(led_pin, OUTPUT);
	  // Servo
    Servo1.attach(servo_pin1);
    Servo2.attach(servo_pin2);
}

void led() {
  
  if (analogRead(ldr_pin) > 500) {
      digitalWrite(led_pin, 0);
  } else {
      digitalWrite(led_pin, 1);
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
  Serial.println(analogRead(moisture_pin));
}

void watertank() {
  Serial.print("Tank: ");
  Serial.println(hc.dist());
}

void mqtt() {
  int lightsensor_data = analogRead(ldr_pin);
  
  String message = "{lightsensor: " + String(lightsensor_data) + "}";
  Serial.println(message);
}

void loop()
{ 
  led();
  servo();
  moisture();
  watertank();
  mqtt();
  delay(1000);
}
