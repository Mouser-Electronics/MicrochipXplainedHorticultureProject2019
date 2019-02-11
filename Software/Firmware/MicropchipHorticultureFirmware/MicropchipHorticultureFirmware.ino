/***************************************************************************
  Microchip Xplained Board 8-bit microcontroller Horticulture Project
  - BME280 temperature and humidity sensor
  - STEMMA soil moisture sensor feat. Microchip ATSAMD10
  - DFRobot pH Sensor

  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

//Include needed support libriaries
#include <Wire.h>
#include <WiFiEsp.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_seesaw.h>
#include "secretstuff.h"

//Decalre instances of sensors and wifi objects
Adafruit_BME280 bme;
Adafruit_seesaw soilSensor;
WiFiEspClient wifiClient;

//Control variables for MQTT connection to MediumOne servers
static int heartbeat_timer = 0;
static int sensor_timer = 0;
static int heartbeat_period = 60000;
long lastReconnectAttempt = 0;

//Local control variables
static int sensor_period = 5000;
const float vref = 5.0;
const float phOffset = 0.0;

//Declare variables for pins
const int phSensorPin = A0;


/*****************************************************
 * Function: setup
 * Inputs: None
 * Return: None
 * Description:  Initializes serial communications,
 * sensor I2C comms, and I/O mode of pins
 *****************************************************/
void setup() {

  Serial.begin(9600);
  while (!Serial) {}

  Serial.println(F("Initializing hardware settings..."));

  Serial.println(F("Attempting to connect to WiFi network..."));
  WiFi.begin(myssid, myssidpw);
  delay(5000);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Failed to connect, resetting"));
    WiFi.begin(myssid, myssidpw);
    delay(1000);
  }
  Serial.println(F("Connected to Wifi!"));
  connectMQTT();

  Serial.println(F("Connecting to BME280 sensor..."));
  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1);
  }
  Serial.println(F("Successfully connected to BME280 sensor!"));

  Serial.println(F("Connecting to soil sensor..."));
  if (!soilSensor.begin(0x36)) {
    Serial.println(F("ERROR! Soil sensor not found"));
    while(1);
  } else {
    Serial.print("Soil sensor started! version: ");
    Serial.println(soilSensor.getVersion(), HEX);
  }

  pinMode(phSensorPin, INPUT);
  
  Serial.println("Setup is complete!");
}


/*****************************************************
 * Function: callback
 * Inputs: 
 *   char* topic:  char array with topic of MQTT packet
 *   byte* payload: char array with payload of MQTT packet
 *   unsigned int length: length of the payload
 * Return: None
 * Description:
 *****************************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  int i = 0;
  char message_buff[length + 1];
  for (i = 0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';

  Serial.print(F("Received some data: "));
  Serial.println(String(message_buff));
}


/*****************************************************
 * Declares an instance of a PubSubClient
 *****************************************************/
PubSubClient client(server, port, callback, wifiClient);


/*****************************************************
 * Function: connectMQTT
 * Inputs: None
 * Return: boolean
 * Description:
 *****************************************************/
boolean connectMQTT()
{
  if (client.connect((char*) mqtt_username, (char*) mqtt_username, (char*) mqtt_password)) {
    Serial.println(F("Connected to MQTT broker"));

    if (client.publish((char*) pub_topic, "{\"event_data\":{\"mqtt_connected\":true}}")) {
      Serial.println("Publish connected message ok");
    } else {
      Serial.print(F("Publish connected message failed: "));
      Serial.println(String(client.state()));
    }

    if (client.subscribe((char *)sub_topic, 1)) {
      Serial.println(F("Successfully subscribed"));
    } else {
      Serial.print(F("Subscribed failed: "));
      Serial.println(String(client.state()));
    }
  } else {
    Serial.println(F("MQTT connect failed"));
    Serial.println(F("Will reset and try again..."));
    abort();
  }
  return client.connected();
}


/*****************************************************
 * Function: loop
 * Inputs: None
 * Return: None
 * Description:  
 *****************************************************/
void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 1000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (connectMQTT()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
  heartbeat_loop();
  sensor_loop();
}


/*****************************************************
 * Function: sensor_loop
 * Inputs: None
 * Return: None
 * Description:  
 *****************************************************/
void sensor_loop() {
  float humidity = 0.0;
  float tempC = 0.0;
  float moistureLevel = 0.0;
  float phAnalogReading = 0.0;
  float phLevel = 0.0;
  
  
  if ((millis() - sensor_timer) > sensor_period) {
    sensor_timer = millis();
    tempC = bme.readTemperature();
    humidity = bme.readHumidity();
    moistureLevel = soilSensor.touchRead(0);
    phAnalogReading = analogRead(phSensorPin) * vref / 1024.0;
    phLevel = 3.5 * phAnalogReading + phOffset;

    String payload = "{\"event_data\":{\"temperature\":";
    payload += tempC;
    payload += ",\"humidity\":";
    payload += humidity;
    payload += ",\"moisture\":";
    payload += moistureLevel;
    payload += ",\"ph\":";
    payload += phLevel;
    payload += "}}";

    if (client.loop()) {
      Serial.print(F("Sending sensor: "));
      Serial.println(payload);

      if (client.publish((char *) pub_topic, (char*) payload.c_str()) ) {
        Serial.println("Publish ok");
      } else {
        Serial.print(F("Failed to publish sensor data: "));
        Serial.println(String(client.state()));
      }
    }

    Serial.println();
    delay(2000);
  }
}


/*****************************************************
 * Function: heartbeat_loop
 * Inputs: None
 * Return: None
 * Description:  Initializes serial communications,
 * sensor I2C comms, and I/O mode of pins
 *****************************************************/
void heartbeat_loop() {
  if ((millis() - heartbeat_timer) > heartbeat_period) {
    heartbeat_timer = millis();
    String payload = "{\"event_data\":{\"millis\":";
    payload += millis();
    payload += ",\"heartbeat\":true}}";

    if (client.loop()) {
      Serial.print(F("Sending heartbeat: "));
      Serial.println(payload);

      if (client.publish((char *) pub_topic, (char*) payload.c_str()) ) {
        Serial.println(F("Publish ok"));
      } else {
        Serial.print(F("Failed to publish heartbeat: "));
        Serial.println(String(client.state()));
      }
    }
  }
}
