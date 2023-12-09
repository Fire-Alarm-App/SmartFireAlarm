#include <WiFi.h>

const char* ssid = "Samsung Galaxy S7 0900";
const char* password = "Crazycan";
const char* host = "192.168.241.200"; // IP address of your Python server, may need to change when using local computer
int port = 8080; // Port to connect to

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

  Serial.println("Starting...");
}

void MQTTCom(){

  // Collect the data you want to send
  String dataToSend = "STATUS: ALARM\n<Fire Detected>\n<Send Notification to MQTT Broker>\n\n";

  // Establish a connection to the local Python server
  Serial.println(ssid);

  WiFiClient client;
  if (client.connect(host, port)) {
    Serial.println("Connected to server");
    client.println(dataToSend);
    client.stop();
  } else {
    Serial.println("Connection failed");
  }

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
  checkFire();
  
  

  delay(1000);
}
