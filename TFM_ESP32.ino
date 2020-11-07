#include "WiFi.h"
#include <Wire.h>
#include <VL53L0X.h>
#include <PubSubClient.h>

#include "config.h"  // Sustituir con datos de vuestra red
#include "ESP32_Utils.hpp"

VL53L0X sensor;
VL53L0X sensor2;
int persones_dins=0;
int aforament_max = 10;
byte entrant = 0;
byte sortint = 0;

int pos_sensor1 = 25;
int pos_sensor2 = 26;

//MQTT
const char* mqtt_server = "172.16.5.6";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup() {

  pinMode(pos_sensor1, OUTPUT);
  pinMode(pos_sensor2, OUTPUT);

  digitalWrite(pos_sensor1, LOW);
  digitalWrite(pos_sensor2, LOW);

  delay(500);
  Wire.begin();


  Serial.begin (115200);

  //MQTT
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ConnectWiFi_STA();

  //SENSOR 1
  pinMode(pos_sensor1, INPUT);
  delay(100);
  Serial.println("00");
  sensor.init(true);
  Serial.println("01");
  delay(100);
  sensor.setAddress((uint8_t)22);
  Serial.println("02");

  //SENSOR 2
  pinMode(pos_sensor2, INPUT);
  delay(100);
  sensor2.init(true);
  Serial.println("03");
  delay(100);
  sensor2.setAddress((uint8_t)25);
  Serial.println("04");


  Serial.println("");
  Serial.println("addresses set");
  Serial.println("");
  Serial.println("");
  sensor.setTimeout(500);

}

//MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("hmataro/salaespera1/persones", "El meu primer missatge");
      // ... and resubscribe
      client.subscribe("hmataro/salaespera1/luz");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  byte count = 0;
 
  //CHECK DISTANCES
  long DISTANCE_SENSOR = (sensor.readRangeSingleMillimeters());
  long DISTANCE_SENSOR2 = (sensor2.readRangeSingleMillimeters());

  /*//FWD OR SENSOR
  if (sensor.timeoutOccurred())
  {
    Serial.println("_________________________________");
    Serial.print("Distance Sensor 1 (READING): ");
    Serial.println(" TIMEOUT");
    Serial.println("");
  }
  else
  {
    Serial.println("_________________________________");
    Serial.print("Distance Sensor 1 (meters): ");
    Serial.println(DISTANCE_SENSOR/1000.000);
    Serial.println("");
  }

  //FLT OR SENSOR2
  if (sensor2.timeoutOccurred())
  {
    Serial.print("Distance Sensor 2 (READING): ");
    Serial.println(" TIMEOUT");
    Serial.println("_________________________________");
    Serial.println("");
  }
  else
  {
    Serial.print("Distance Sensor 2 (meters): ");
    Serial.println(DISTANCE_SENSOR2/1000.000);
    Serial.println("_________________________________");
    Serial.println("");
  }*/

 //Conta les que entren
    if(DISTANCE_SENSOR/1000.000 < 8.19 && sortint==0){ 
      if(persones_dins>=aforament_max){
        Serial.println("...AFORAMENT MÀXIM!, ESPERI PER ENTRAR!...");
        delay(1000);
      }else{
        entrant = 1;
        Serial.println("...ENTRANT...");
        delay(1000);
      }
    }
    if(DISTANCE_SENSOR2/1000.000 < 8.19 && entrant==0){ 
      if(persones_dins==0){
        Serial.println("...CAP PERSONA A LA SALA, NINGU POT SORTIR!...");
        delay(1000);
      }else{
        sortint = 1;
        Serial.println("...SORTINT...");
        delay(500);
      }
    }
      
    if(DISTANCE_SENSOR2/1000.000 < 8.19 && entrant==1){
      persones_dins++;
      snprintf (msg, MSG_BUFFER_SIZE, "Persones dins #%ld", persones_dins);
      client.publish("hmataro/salaespera1/persones", msg);
      Serial.print("Persones dins: ");
      Serial.println(persones_dins);
      entrant = 0;
      delay(500);
    }
    if(DISTANCE_SENSOR/1000.000 < 8.19 && sortint==1){
      persones_dins--;
      snprintf (msg, MSG_BUFFER_SIZE, "Persones dins #%ld", persones_dins);
      client.publish("hmataro/salaespera1/persones", msg);
      Serial.print("Persones dins: ");
      Serial.println(persones_dins);
      sortint = 0;
      delay(500);
    }
    /*if(DISTANCE_SENSOR2/1000.000 == 8.19 && entrant==1){
      //Massa temps sense acabar d'entrar
      entrant = 0;
      Serial.println("...Massa temps sense acabar d'entrar...");
      delay(500);
    }*/
        
  //delay(1200);//can change to a lower time like 100

  //MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

 /* unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("hmataro/salaespera1/persones", msg);
  }*/
  
}
