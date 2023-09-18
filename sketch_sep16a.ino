#include <WiFi.h>
#include <WebSocketClient.h>

#define DHTPIN 12         
#define DHTTYPE DHT11 
#include <DHT.h>
#include "time.h"

DHT dht(DHTPIN, DHTTYPE); 

const char* ssid = "xxx";
const char* password = "xxx";
char path[] = "/";
char host[] = "192.168.0.123"; 
int port = 5000;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 36000;       
const int daylightOffset_sec = 3600;

float setTemp = 25;
String state = "none";
String forceState = "auto";

String schedulState = "auto";
tm startSchedule;
tm endSchedule;

WebSocketClient webSocketClient;

WiFiClient client;

tm parseTime(const String timeStr) {
  tm temptimeinfo;

  int hours, minutes;
  sscanf(timeStr.c_str(), "%d:%d", &hours, &minutes);

  getLocalTime(&temptimeinfo);

  temptimeinfo.tm_hour = hours;
  temptimeinfo.tm_min = minutes;
  temptimeinfo.tm_sec = 0; 

  return temptimeinfo;
}

void splitScehduleString(String input, char delimiter) {
  int partCount = 0;
  int startIndex = 0;
  int endIndex;
  String parts[3]; 
 
  while (true) {
    endIndex = input.indexOf(delimiter, startIndex);

    if (endIndex == -1) {
      parts[partCount++] = input.substring(startIndex);
      break;
    }

    parts[partCount++] = input.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
  }
  schedulState = parts[0];
  startSchedule = parseTime(parts[1]);
  endSchedule = parseTime(parts[2]);
}

void setup() {
  Serial.begin(115200);
  dht.begin();  

  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(5000);

  if (client.connect(host, port)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    while(1) {
    }
  }

  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    startSchedule = parseTime("0:00");
    endSchedule = parseTime("0:00");

    webSocketClient.sendData("temperature: initiate");

  } else {
    Serial.println("Handshake failed.");
    while(1) {
    }  
  }
}



void loop() {
  String data;
  float temperature = dht.readTemperature();
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  if (client.connected()) {
    
    webSocketClient.getData(data);
    if (data.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(data);

      if (data.startsWith("setTemp: ")) {
        data = data.substring(8);
        Serial.println(data);
        setTemp = data.toFloat();

      }
      if (data.startsWith("setState: ")) {
        data = data.substring(10);
        Serial.println(data);
        forceState = data;

      }
      if (data.startsWith("setSchedule: ")) {
        data = data.substring(13);
        Serial.println(data);
        splitScehduleString(data,',');

      }
    }
   
   if (temperature > setTemp+0.5){
     state = "cool";

   }else if (temperature < setTemp-0.5){
     state = "heat";

   }else{
     state = "none";
   }

   if(forceState != "auto"){
     state=forceState;
   }

   if (timeinfo.tm_hour > startSchedule.tm_hour || (timeinfo.tm_hour == startSchedule.tm_hour && timeinfo.tm_min > startSchedule.tm_min)) {
     if (timeinfo.tm_hour > endSchedule.tm_hour || (timeinfo.tm_hour == endSchedule.tm_hour && timeinfo.tm_min > endSchedule.tm_min)) {
      } else {
        if(schedulState != "auto"){
          state=schedulState;
        }

      }
   } else {
   }

   String tempData = "temperature: " + String(temperature)+ ',' + state + ',' + String(setTemp);

   webSocketClient.sendData(tempData);
    
  } else {
    Serial.println("Client disconnected.");
    while (1) {
    }
  }
  
  delay(2000);
}

