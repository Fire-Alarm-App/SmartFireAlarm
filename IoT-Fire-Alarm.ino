#include <WiFi.h>
#include <PubSubClient.h> // Include the PubSubClient library

#include "esp_wpa2.h"

#define MQ2pin A0

float sensorValue;  //variable to store sensor value

String macAddress;
const char* ssid = "MSetup"; // your ssid
#define EAP_ID "crazycan"
#define EAP_USERNAME "crazycan"
#define EAP_PASSWORD "green Potato05!"
/*const char* host = "192.168.241.200"; // IP address of your Python server, may need to change when using local computer
int port = 8080; // Port to connect to*/
String sendTopic = "/ER/";
String receiveTopic = "/response/";
String setupTopic = "/setup";


const char* mqtt_server = "141.215.80.233"; // IP address or hostname of your MQTT broker
const int mqtt_port = 1883; // MQTT broker port (usually 1883)
const char* mqtt_username = "bcsotty";
const char* mqtt_password = "correct";
const char* mqtt_location = "None";

bool receivedResponse = false; // Flag to track whether a response has been received
int response = 0;

int global_stamp = 0;

WiFiClient espClient;
PubSubClient client(espClient);

int out = 5, in = 2;
int alarmOut = 4;

void callback(char* topic, byte* payload, unsigned int length) {
  // This function will be called when a message is received on the subscribed topic
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Message payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Convert the payload to an integer and store it
  response = atoi((char*)payload);

  // Add your custom logic to handle the received integer value here
  Serial.print("Response from server: ");
  Serial.println(response);

  // Add your custom logic to handle the received message here
  receivedResponse = true; // Set the flag to indicate that a response has been received
}

void setupMQTT(){
  while(!client.connected()){
    Serial.println("Trying to connect for setup, Please wait");
    if(client.connect("arduinoClient", mqtt_username, mqtt_password)){
      Serial.println("Connected");
    }
    delay(1000);
  }


  Serial.print("Setting up arduino MQTT connections " + macAddress);

  String dataToSend = macAddress;
  

  client.publish(setupTopic.c_str(), dataToSend.c_str()); // Publish data to the MQTT topic

  sendTopic = sendTopic + macAddress;
  receiveTopic = receiveTopic + macAddress;

  /*Serial.println(sendTopic);
  Serial.println(receiveTopic);*/

  return;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  pinMode(out, OUTPUT);
  pinMode(in, INPUT);
  digitalWrite(out, HIGH);
  pinMode(alarmOut, OUTPUT);

  

  // WPA2 enterprise magic starts here
  WiFi.disconnect(true);      
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  // WPA2 enterprise magic ends here

  WiFi.begin(ssid);

  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  client.setServer(mqtt_server, mqtt_port);

  // Subscribe to the desired MQTT topic and set the callback function
  client.setCallback(callback);

  Serial.println("Starting...");
    // Obtain and print the MAC address
  macAddress = WiFi.macAddress();
  setupMQTT();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);
  Serial.println("MQ2 warming up!");

	//delay(20000); // allow the MQ2 to warm up
}



void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    Serial.print(receiveTopic);
    //client.connect("arduinoClient", mqtt_username, mqtt_password);
    if (client.connect("arduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(receiveTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void sleepAlarm(){
  digitalWrite(alarmOut, LOW); // turns off alarm
  delay(2000); // set back to 60000
  checkFire();
}

void procFireAlarm(){
  digitalWrite(alarmOut, HIGH); // turns on alarm
  delay(2000); //set back to 180000
  checkFire();
}

void waitForResponse(){
  //decide to turn off or trigger
  //if there is a fire alarm trigger event
  //else sleep for 1-5 minute
  Serial.println("Waiting for a response");

  while(!receivedResponse){
    //Serial.print(".");
    client.loop();
  }

  Serial.println("Made It Here");

  receivedResponse = false;
  
  if(response == 0){
    sleepAlarm();
  }
  else{
    procFireAlarm();
  }

  //delay(5000);
}

void MQTTCom(){

  // Collect the data you want to send
  String dataToSend = "STATUS: ALARM\n<Fire Detected>\n<Send Notification to MQTT Broker>\n\n" + String(global_stamp);

  global_stamp += 1;
  // Establish a connection to the local Python server
  Serial.println(ssid);

  if (!client.connected()) {
    reconnect();
  }

  client.publish(sendTopic.c_str(), dataToSend.c_str()); // Publish data to the MQTT topic
  waitForResponse();

}

void checkFire(){
  if(digitalRead(in) == 1){
    sensorValue = analogRead(MQ2pin); // read analog input pin 0
    if(sensorValue > 1300){
      digitalWrite(alarmOut, HIGH); // turns on alarm
      MQTTCom();
    }
  }
  else{
    Serial.println("STATUS: READY\n<No Fire Detected>\n\n");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(client.connected());
  
  if (!client.connected()) {
    //client.disconnect(); // Disconnect if already connected
    reconnect();
  }

  //client.loop();

  if(receivedResponse){
    waitForResponse();
  }
  checkFire();
  delay(1000);
}
