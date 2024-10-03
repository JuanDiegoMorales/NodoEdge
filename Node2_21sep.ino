
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#include <WiFi.h>

#include <WiFiMulti.h>
#include "time.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DEVICE "ESP32"


//#define WIFI_SSID "iPhone de Juan Diego"
// WiFi password
//#define WIFI_PASSWORD "123.123."
#define WIFI_SSID "TP-LINK_DE16"
// WiFi password
#define WIFI_PASSWORD "67743667"
// Contraseña TpLink 67743667
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "oJC3TJ8BEnShNJW6lJNMUkqf8y8vzD9PVL5mmSK_gFzjxUKk1sLENlKHjh9GfA0ZIaqW9j3GGEG_7GIexS33Ig=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "7bbce2df7904a4d8"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "Avocados"

#define TZ_INFO "<-05>5"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3600*6;
const int   daylightOffset_sec = 3600;
// MESH Details
#define   MESH_PREFIX     "avocados" //name for your MESH
#define   MESH_PASSWORD   "avocados123" //password for your MESH
#define   MESH_PORT       5555 //default port

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// InfluxDB client instance without preconfigured InfluxCloud certificate for insecure connection 
//InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Data point


int redPin =  13 ;
int greenPin =  12 ;
int bluePin =  14 ;
//Number for this node
int nodeNumber = 2;
int node = 0;
double temp = 0;
double hum = 0;
String Color;
bool state=0;

bool yetactivated=0;
//String to send to other nodes with sensor readings
String readings;
WiFiMulti wifiMulti;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
char hour[3];
char minut[3];
char sec[3];

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings
Point sensor("Temmperature");
Point sens2("humidity");
Point sens3("led_state");
Point sens4("time");
//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

String getReadings () {
  JSONVar jsonReadings;
  printLocalTime();
  
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minut);
  Serial.print(":");
  Serial.println(sec);  

  jsonReadings["node"] = nodeNumber;
  jsonReadings["color"] = Color;
 
 
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
  
  int hourR = myObject["hour"];   
  int minR = myObject["min"];
  int secR = myObject["sec"];

  node = myObject["node"];

  temp = myObject["temp"];
  
  hum = myObject["hum"];

  Serial.print(hourR);
  Serial.print(":");
  Serial.print(minR);
  Serial.print(":");
  Serial.println(secR);
  
  Serial.print("Nodep: ");
  Serial.println(node);
  Serial.print("Temperaturep: ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.print("Humidityp: ");
  Serial.print(hum);
  Serial.println(" %");

  condition(temp, hum);
  char aStr[12]; 
  sprintf(aStr,"%d : %d : %d", hourR, minR, secR);
  // Stops the mesh in this node
  mesh.stop();

  //connect to wifi
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");  
  }  
  Serial.println("");
  Serial.println("WiFi connected.");  
  // Init and get the time
  
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  //Init the inserting of data in the basedata
    sensor.clearFields();
    sens2.clearFields();
    sens3.clearFields();
    sens4.clearFields();
    // Report RSSI of currently connected network
    sensor.addField("Temperature", temp);
    sens2.addField("humidity", hum);
    sens3.addField("led_state", state);  // Añadir el estado del LED
    sens4.addField("timestamp", String(aStr)); // Añadir timestamp
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(sensor));
    
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    
    // Write point
    if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write temperature failed: ");
    Serial.println(client.getLastErrorMessage());
    }
    else{
      Serial.println("InfluxDB write temperature sucesfull");
    }
    if (!client.writePoint(sens2)) {
    Serial.print("InfluxDB write humedity failed: ");
    Serial.println(client.getLastErrorMessage());
    }
    else{
      Serial.println("InfluxDB write humedity sucesfull");
    }
    if (!client.writePoint(sens3)) {
    Serial.print("InfluxDB write led state failed: ");
    Serial.println(client.getLastErrorMessage());
    }
    else{
      Serial.println("InfluxDB write led state sucesfull");
    }
    if (!client.writePoint(sens4)) {
    Serial.print("InfluxDB write time failed: ");
    Serial.println(client.getLastErrorMessage());
    }
    else{
      Serial.println("InfluxDB write time sucesfull");
    }
    
    


  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }  

  // Disconnect the wifi
  WiFi.disconnect();
  Serial.print("Wifi Disconnected");
  //connect to the mesh
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
    
  mesh.update();

}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}
void condition( float temp, int hum){
  if  (temp > 29){
    setColor(255, 0, 0);
    Color = "Red";
    state = 1;
    }
  else {
    setColor(0,  255, 0);
    Color = "Green";
    state= 0;
  }
  Serial.print("Led state changed to = ");
  Serial.println(Color);
}
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}
void setColor(int redValue, int greenValue,  int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin,  greenValue);
  analogWrite(bluePin, blueValue);
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
  
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  
  pinMode ( redPin ,   OUTPUT ) ;              
  pinMode ( greenPin ,  OUTPUT ) ;
  pinMode ( bluePin ,  OUTPUT ) ;
  
  
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  // Initialize WiFi (for later use)
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");  
  }  
  Serial.println("");
  Serial.println("WiFi connected.");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  Serial.print(hour);
  Serial.print(":");
  Serial.print(minut);
  Serial.print(":");
  Serial.println(sec);

  sensor.addTag("Temp", "Temperature");
  sens2.addTag("Hum", "Humedity");
  sens3.addTag("Sta", "Led_state");
  sens4.addTag("Time", "datetime");
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  
  WiFi.disconnect();
  Serial.print("Wifi Disconnected");
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  userScheduler.addTask(taskSendMessage);
  
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();

}