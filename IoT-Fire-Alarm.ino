#include <WiFi.h>
#include <PubSubClient.h> // Include the PubSubClient library

const char* ssid = "Samsung Galaxy S7 0900";
const char* password = "Crazycan";
/*const char* host = "192.168.241.200"; // IP address of your Python server, may need to change when using local computer
int port = 8080; // Port to connect to*/
const char* topic = "/FireAlarm";
const char* mqtt_server = "mqtt.eclipseprojects.io"; // IP address or hostname of your MQTT broker
const int mqtt_port = 1883; // MQTT broker port (usually 1883)

int global_stamp = 0;

WiFiClient espClient;
PubSubClient client(espClient);

int out = 3, in = 2;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  pinMode(out, OUTPUT);
  pinMode(in, INPUT);
  digitalWrite(out, HIGH);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Starting...");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void waitForResponse(){
  //decide to turn off or trigger
  //if there is a fire alarm trigger event
  //else sleep for 1-5 minute
  delay(5000);
}

void MQTTCom(){

  // Collect the data you want to send
  String dataToSend = "STATUS: ALARM\n<Fire Detected>\n<Send Notification to MQTT Broker>\n\n" + String(global_stamp);

  global_stamp += 1;
  // Establish a connection to the local Python server
  Serial.println(ssid);

  if (!client.connected()) {
    /*Serial.println("Connected to server");
    client.println(dataToSend);
    client.stop();*/
    reconnect();
  }
  client.publish("/FireAlarm", dataToSend.c_str()); // Publish data to the MQTT topic
  waitForResponse();

  /*Serial.println("STATUS: ALARM\n<Fire Detected>\n<Send Notification to MQTT Broker>\n\n");
  delay(2000);
  Serial.println("STATUS: ALARM\n<Message sent to MQTT Broker>\n\n");
  //if else statement with a timeout variable
  delay(2000);
  Serial.print("STATUS: UPDATE\n<Message was recieved>\n<Determine validity of alarm>\n\n");*/
}

void checkFire(){
  if(digitalRead(in) == 1){
    MQTTCom();
  }
  else{
    Serial.println("STATUS: READY\n<No Fire Detected>\n\n");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  checkFire();
  delay(1000);
}
