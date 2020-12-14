#include <WiFi.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <PubSubClient.h>

#include "config.h"  // Dades de la xarxa wifi
#include "ESP32_Utils.hpp"

VL53L0X sensor;
VL53L0X sensor2;
int persones_dins=0;
int aforament_max = 10;
byte entrant = 0;
byte sortint = 0;
int entrant_delay = 0;
int sortint_delay = 0;

//Posició de la placa on es connecten els sensors
int pos_sensor1 = 25;
int pos_sensor2 = 26;

int pos1;
int pos2;
int pos3;
int numero = 10;
String TOPIC;

//MQTT
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
  ConnectWiFi_STA();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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

//Callback MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
 
 // Converteixo a número el missatge rebut
    for (int i = 0; i < length; i++) {
      if(i == 0){
        pos1 = payload[i] - '0';
        numero = pos1;
      }
      if(i == 1){
        pos2 = payload[i] - '0';
        numero = pos1*10+pos2;
      }
      if(i == 2){
        pos3 = payload[i] - '0';
        numero = numero * 10 + pos3;
      }
   //Si el topic es CAPACITAT l'actulaitzo
   if(String(topic) == "hmataro/salaespera1/capacitat"){
      if(aforament_max != numero){
        Serial.print("********* S'HA CANVIAT LA CPACITAT DE LA SALA A: ");
        Serial.println(numero);
        aforament_max = numero;
      }
    //Si el topic es PERSONES_DINS l'actualitzo
    }else if(String(topic) == "hmataro/salaespera1/persones_dins"){
        if(persones_dins != numero){
          Serial.print("********* RECUPERO PERSONES DINS DE LA BBDD: ");
          Serial.println(numero);
          persones_dins = numero;
        }
    }
  }
}

//Reconnect MQQT
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
      // resubscribe
      client.subscribe("hmataro/salaespera1/persones_dins");
      client.subscribe("hmataro/salaespera1/capacitat");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  byte count = 0;
 
  //CHECK DISTANCES
  long DISTANCE_SENSOR = (sensor.readRangeSingleMillimeters());
  long DISTANCE_SENSOR2 = (sensor2.readRangeSingleMillimeters());

 //Conta les persones que entren
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
      //Poasso al broker les persones que hi ha dins
      snprintf (msg, MSG_BUFFER_SIZE, "%ld", persones_dins);
      client.publish("hmataro/salaespera1/persones", msg);
      //Passo al broker les persones en %
      snprintf (msg, MSG_BUFFER_SIZE, "%ld", (int)((float)persones_dins/(float)aforament_max*100));
      client.publish("/persones_percentatge", msg);
      Serial.print("Persones dins: ");
      Serial.println(persones_dins);
      //La persona ha entrat inicialitzo la variable entrant
      entrant = 0;
      delay(500);
    }
    if(DISTANCE_SENSOR/1000.000 < 8.19 && sortint==1){
      persones_dins--;
      //Poasso al broker les persones que hi ha dins
      snprintf (msg, MSG_BUFFER_SIZE, "%ld", persones_dins);
      client.publish("hmataro/salaespera1/persones", msg);
      //Passo al broker les persones en %
      snprintf (msg, MSG_BUFFER_SIZE, "%ld", (int)((float)persones_dins/(float)aforament_max*100));
      client.publish("hmataro/salaespera1/persones_percentatge", msg);
      Serial.print("Persones dins: ");
      Serial.println(persones_dins);
      //La persona ha sortit inicialitzo la variable sortint
      sortint = 0;
      delay(500);
    }
    
    if(entrant==1){
      entrant_delay++;
    }
    
    if(entrant_delay > 100 && entrant==1){
      //Massa temps sense acabar d'entrar incialitzem les variables
      entrant = 0;
      entrant_delay = 0;
      Serial.println("...Massa temps sense acabar d'entrar...");
      delay(500);
    }
    
    if(sortint==1){
      sortint_delay++;
    }
    
    if(sortint_delay > 100 && sortint==1){
      //Massa temps sense acabar de sortir incialitzem les variables
      sortint = 0;
      sortint_delay = 0;
      Serial.println("...Massa temps sense acabar de sortir...");
      delay(500);
    }

  //MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
