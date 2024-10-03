#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include "DHT.h"

#include <WiFi.h>
#include "time.h"

//#define WIFI_SSID "iPhone de Juan Diego"
// WiFi password
//#define WIFI_PASSWORD "123.123."
#define WIFI_SSID "TP-LINK_DE16"
// WiFi password
#define WIFI_PASSWORD "67743667"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3600*6;
const int   daylightOffset_sec = 3600;


// MESH Details
#define   MESH_PREFIX     "avocados" //name for your MESH
#define   MESH_PASSWORD   "avocados123" //password for your MESH
#define   MESH_PORT       5555 //default port


#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321





DHT dht(DHTPIN, DHTTYPE);

//Number for this node
int nodeNumber = 1;

//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

char hour[3];
char minut[3];
char sec[3];
float temperature = 0;
float humidity = 0;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings


//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 30 , TASK_FOREVER , &sendMessage);

String getReadings () {
  JSONVar jsonReadings;
  
  printLocalTime();
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minut);
  Serial.print(":");
  Serial.println(sec);

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperature);
  Serial.println(F("°C "));


  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = temperature;
  jsonReadings["hum"] = humidity;
  jsonReadings["hour"] = hour;
  jsonReadings["min"] = minut;
  jsonReadings["sec"] = sec;
  

  readings = JSON.stringify(jsonReadings);
  return readings;
}

void sendMessage () {
  String msg = getReadings();
  Serial.println(msg);
  mesh.sendBroadcast(msg);
  

  
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());

  int hour = myObject["hour"];   
  int min = myObject["min"];
  int sec = myObject["sec"];

  int node = myObject["node"];

  String color = myObject["color"];

  Serial.print(hour);
  Serial.print(":");
  Serial.print(minut);
  Serial.print(":");
  Serial.println(sec);

  Serial.print("Node: ");
  Serial.println(node);

  Serial.print("Led color: ");
  Serial.println(color);
  //taskSendMessage.disable();
  
  //delay(1000);
  
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);

}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
 
  strftime(hour,3, "%H", &timeinfo);
  
  strftime(minut,3, "%M", &timeinfo);

  strftime(sec,3, "%S", &timeinfo);
}
void setup() {

  Serial.begin(115200);
  

  dht.begin();
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Add tags
  
    // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  
    
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minut);
  Serial.print(":");
  Serial.println(sec);
  

   //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
  
}