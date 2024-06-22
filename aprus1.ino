#include <Servo.h>  // Include the Servo library 

int servoPin1 = 6;   // Declare the Servo pin 
int servoPin2 = 5;
Servo Servo1;        // Create a servo object 
Servo Servo2;

int ldr = A3;//assigning ldr pin no.
int bulb = 7;//assigning bulb pin no.

void setup()
{
  // Serial
  Serial.begin(9600);
    

  // LightSensor
  pinMode(ldr, INPUT);
  	// Light Bulb
  	pinMode(bulb, OUTPUT);
	// Servo
    Servo1.attach(servoPin1);
    Servo2.attach(servoPin2);
}

void lightbulb() {
  
  if (analogRead(ldr) > 500) {
      digitalWrite(bulb, 0);
  } else {
      digitalWrite(bulb, 1);
  }

}

void servo() {
  
  if (analogRead(ldr) > 500) {
      Servo1.write(90);
      Servo2.write(90);
  } else {
      Servo1.write(0);
      Servo2.write(0);
  }
  
}

void mqtt() {
  int lightsensor_data = analogRead(ldr);
  
  String message = "{lightsensor: " + String(lightsensor_data) + "}";
  Serial.println(message);
}

void loop()
{ 
  lightbulb();
  servo();
  mqtt();
  delay(1000);
}