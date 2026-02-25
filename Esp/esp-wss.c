#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ================= WIFI =================
const char* WIFI_SSID="Sorry";
const char* WIFI_PASS="aaaaaaaa";

// ================= RENDER WSS =================
const char* WSS_HOST="query2dance.onrender.com";
const uint16_t WSS_PORT=443;
const char* WSS_PATH="/ws";

WebSocketsClient webSocket;

// ================= SERVOS =================
int servoPins[10]={16,17,18,19,21,22,23,25,26,27};
Servo servos[10];

const int POS_CENTER=90;
const int POS_RIGHT=170;
const int POS_LEFT=10;

const unsigned long RUN_DURATION_MS=30000;
const unsigned long LEG_DURATION_MS=2000;

volatile bool active[10]={false};
unsigned long startMs[10]={0};
unsigned long legStartMs[10]={0};
int startAngle[10]={0};
int targetAngle[10]={0};

// ================= RELAYS =================
#define RELAY1_PIN 32
#define RELAY2_PIN 33

bool relay1On=false;
bool relay2On=false;

void applyRelays(){
  digitalWrite(RELAY1_PIN,relay1On?LOW:HIGH);
  digitalWrite(RELAY2_PIN,relay2On?LOW:HIGH);
}

// ================= SERVO MOTION =================
int lerp(int a,int b,float t){
  if(t<=0)return a;
  if(t>=1)return b;
  return a+(b-a)*t;
}

void beginLeg(int i,int fromA,int toA){
  startAngle[i]=fromA;
  targetAngle[i]=toA;
  legStartMs[i]=millis();
  servos[i].write(fromA);
}

void startMotion(int ch){
  active[ch]=true;
  startMs[ch]=millis();
  beginLeg(ch,POS_CENTER,POS_RIGHT);
}

void stopMotion(int ch){
  active[ch]=false;
  servos[ch].write(POS_CENTER);
}

void stopAll(){
  for(int i=0;i<10;i++)stopMotion(i);
}

void updateMotions(){
  unsigned long now=millis();
  for(int i=0;i<10;i++){
    if(!active[i])continue;

    if(now-startMs[i]>=RUN_DURATION_MS){
      stopMotion(i);
      continue;
    }

    float t=(float)(now-legStartMs[i])/LEG_DURATION_MS;

    if(t>=1){
      servos[i].write(targetAngle[i]);
      int next=(targetAngle[i]==POS_RIGHT)?POS_LEFT:POS_RIGHT;
      beginLeg(i,targetAngle[i],next);
    }else{
      servos[i].write(lerp(startAngle[i],targetAngle[i],t));
    }
  }
}

// ================= COMMAND HANDLER =================
void processCommand(String payload){
  StaticJsonDocument<256> doc;
  DeserializationError err=deserializeJson(doc,payload);
  if(err)return;

  if(doc["type"]!="cmd")return;

  String bits=doc["bitstring"];
  relay1On=doc["light1"];
  relay2On=doc["light2"];
  applyRelays();

  if(bits.length()==10){
    for(int i=0;i<10;i++){
      if(bits[i]=='1')startMotion(i);
      else stopMotion(i);
    }
  }
}

// ================= WS EVENTS =================
void webSocketEvent(WStype_t type,uint8_t* payload,size_t length){
  switch(type){

    case WStype_CONNECTED:
      Serial.println("WS Connected");
      webSocket.sendTXT("{\"type\":\"hello\",\"device\":\"esp32\"}");
      break;

    case WStype_TEXT:
      Serial.println("Received:");
      Serial.println((char*)payload);
      processCommand((char*)payload);
      break;

    case WStype_DISCONNECTED:
      Serial.println("WS Disconnected");
      break;

    case WStype_ERROR:
      Serial.println("WS Error");
      break;
  }
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  // Servos init
  for(int i=0;i<10;i++){
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i]);
    servos[i].write(POS_CENTER);
    delay(20);
  }

  // Relays
  pinMode(RELAY1_PIN,OUTPUT);
  pinMode(RELAY2_PIN,OUTPUT);
  digitalWrite(RELAY1_PIN,HIGH);
  digitalWrite(RELAY2_PIN,HIGH);

  // WiFi
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  Serial.print("Connecting WiFi");
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  // WebSocket Secure Connection (Render)
  webSocket.beginSSL(WSS_HOST,WSS_PORT,WSS_PATH);
  webSocket.setInsecure();   // required for Render TLS
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

// ================= LOOP =================
void loop(){
  webSocket.loop();
  updateMotions();
}
