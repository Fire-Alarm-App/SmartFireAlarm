/*
NOTES:


*/


#include <WiFi.h>
#include <PubSubClient.h> // Include the PubSubClient library

#include "esp_wpa2.h"

#define MQ2pin A0

float sensorValue;  //variable to store sensor value

String macAddress;
const char* ssid = "MSetup"; // your ssid

#define EAP_ID "Your Uniquename here"
#define EAP_USERNAME "Your Uniquename Here"
#define EAP_PASSWORD "Your Password Here"
String sendTopic = "/ER/"; //Base send topic
String receiveTopic = "/response/"; //Base recieve topic
String setupTopic = "/setup"; //Setup Topic




const char* mqtt_server = "141.215.80.233"; // IP address or hostname of your MQTT broker
const int mqtt_port = 1883; // MQTT broker port (usually 1883)

String mqtt_username = "default"; //bcsotty
String mqtt_password = "blaze"; //correct

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

  String resp = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    resp += (char)payload[i];
  }
  Serial.println();

  // Convert the payload to an integer and store it
  response = atoi(resp.c_str());


  // Add your custom logic to handle the received integer value here
  Serial.print("Response from server: ");
  Serial.println(response);

  // Add your custom logic to handle the received message here
  receivedResponse = true; // Set the flag to indicate that a response has been received
}

/*
Author: Andrew Pearson
Description: Setup initial MQTT communication 
            and give a unique username and 
            password using the mac address
*/
void setupMQTT(){

  Serial.print("Setting up arduino MQTT connections " + macAddress);

  //Checks if not connected to client
  while(!client.connected()){
    Serial.println("Trying to connect for setup, Please wait");
    if(client.connect("arduinoClient", mqtt_username.c_str(), mqtt_password.c_str())){
      Serial.println("Connected");
    }
    delay(5000);
  }

  //Username cannot contain ':' so replace with '.'
  macAddress.replace(":", ".");


  //Generate unique username and password using the mac address
  mqtt_username = mqtt_username + macAddress;
  mqtt_password = mqtt_password + macAddress;

  String dataToSend = macAddress;
  Serial.println(mqtt_username);
  Serial.println(mqtt_password);

  //Publish mac address to broker to be stored in database for future use
  client.publish(setupTopic.c_str(), dataToSend.c_str()); // Publish data to the MQTT topic

  //Disconnect to free up default username and password
  //Will reconnect upon entering main loop
  client.disconnect();

  delay(5000);

  //generate topics to listen on
  sendTopic = sendTopic + macAddress;
  receiveTopic = receiveTopic + macAddress;

  return;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  pinMode(out, OUTPUT);
  pinMode(in, INPUT);
  digitalWrite(out, HIGH);
  pinMode(alarmOut, OUTPUT);

  

  //This allows us to connect to the UMICH server

  // WPA2 enterprise magic starts here
  WiFi.disconnect(true);      
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  // WPA2 enterprise magic ends here

  WiFi.begin(ssid);


  //Connect to wifi

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  client.setServer(mqtt_server, mqtt_port);

  // Subscribe to the desired MQTT topic and set the callback function
  client.setCallback(callback);

  Serial.println("Starting...");
  
  // Obtain and print the MAC address
  // Mac address used for unique id

  macAddress = WiFi.macAddress();
  setupMQTT();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);
  Serial.println("MQ2 warming up!");

	//delay(20000); // allow the MQ2 to warm up
}


/*
Author: Andrew Pearson
Description: Reconnects if connection is lost to the MQTT broker
*/
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    Serial.print(receiveTopic);
    if (client.connect("arduinoClient", mqtt_username.c_str(), mqtt_password.c_str())) {
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


/*
Author: Andrew Pearson
Description: Sleeps if a false alarm or if fire is no longer detected, sleep fire alarm
*/
void sleepAlarm(){
  receivedResponse = false;
  digitalWrite(alarmOut, LOW); // turns off alarm
  delay(2000); // set back to 60000
  checkFire();
}


/*
Author: Andrew Pearson
Description: Turns on fire alarm if fire is detected
*/
void procFireAlarm(){
  receivedResponse = false;
  digitalWrite(alarmOut, HIGH); // turns on alarm
  int timer = 0;
  while((digitalRead(in) == 1) && (sensorValue > 1300)){ //Continuously check if there is a fire
    sensorValue = analogRead(MQ2pin); // read analog input pin 0
    delay(30 * 1000); // Delay 30 minute
  }
  checkFire(); //Perform check to see if there is a fire
}

/*
Author: Andrew Pearson
Description: If an alarm goes off, wait for a user response or the time out variable
*/
void waitForResponse(){
  //decide to turn off or trigger
  //if there is a fire alarm trigger event
  //else sleep for 1-5 minute
  Serial.println("Waiting for a response");

  while(!receivedResponse){
    //Serial.print(".");
    client.loop();
  }
  Serial.println(response);
  Serial.println("Made It Here");
  
  if(response == 0){ // if resonse is 0, there is no fire
    sleepAlarm();
  }
  else{
    procFireAlarm();
  }

  //delay(5000);
}

/*
Author: Andrew Pearson
Description: This function communicates the alert to the MQTT broker
*/
void MQTTCom(){

  // Collect the data you want to send
  String dataToSend = "1";

  global_stamp += 1;
  // Establish a connection to the local Python server
  Serial.println(ssid);

  Serial.println(sendTopic);

  if (!client.connected()) {
    reconnect();
  }

  client.publish(sendTopic.c_str(), dataToSend.c_str()); // Publish data to the MQTT topic
  waitForResponse();

}

/*
Author: Andrew Pearson
Description: checks if there is a fire
*/
void checkFire(){
  if(digitalRead(in) == 1){ // this means that the IR Relay flame sensor procs
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

  client.loop();
  //client.loop();

  if(receivedResponse){
    waitForResponse(); // will move past loop because a response was received
  }

  checkFire();
  delay(1000);

}
