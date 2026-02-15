#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* WIFI_SSID="Sorry";
const char* WIFI_PASS="aaaaaaaa";

WebServer server(80);
int servoPins[10]={16,17,18,19,21,22,23,25,26,27};
const int SERVO_MIN_US=520;
const int SERVO_MAX_US=2380;
const int SERVO_FREQ=50;
Servo servos[10];
volatile bool active[10]={false};
unsigned long startMs[10]={0};
unsigned long lastToggleMs[10]={0};
bool toRight[10]={true};
const int POS_CENTER=90;
const int POS_RIGHT=170;
const int POS_LEFT=10;
const unsigned long RUN_DURATION_MS=30000;
const unsigned long HALF_PERIOD_MS=2000;

void startMotion(int ch){
  active[ch]=true;
  startMs[ch]=millis();
  lastToggleMs[ch]=millis();
  toRight[ch]=true;
  servos[ch].write(POS_RIGHT);
}
void stopMotion(int ch){
  active[ch]=false;
  servos[ch].write(POS_CENTER);
}
void updateMotions(){
  unsigned long now=millis();
  for(int i=0;i<10;i++){
    if(!active[i])continue;
    if(now-startMs[i]>=RUN_DURATION_MS){stopMotion(i);continue;}
    if(now-lastToggleMs[i]>=HALF_PERIOD_MS){
      lastToggleMs[i]=now;
      toRight[i]=!toRight[i];
      servos[i].write(toRight[i]?POS_RIGHT:POS_LEFT);
    }
  }
}

void handleSetCmd(){
  String path=server.uri();
  if(!path.startsWith("/setcmd/")){server.send(400,"text/plain","Bad request");return;}
  String bits=path.substring(8); // skip "/setcmd/"
  if(bits.length()!=10){server.send(400,"text/plain","Bitstring must be length 10");return;}
  for(int i=0;i<10;i++){if(bits[i]!='0'&&bits[i]!='1'){server.send(400,"text/plain","Bitstring must contain only 0/1");return;}}
  for(int i=0;i<10;i++){if(bits[i]=='1')startMotion(i);else stopMotion(i);}
  server.send(200,"application/json","{\"status\":\"ok\",\"cmd\":\""+bits+"\"}");
}

void handleEsp(){
  server.send(200,"text/plain","yes i am");
}

void setup(){
  Serial.begin(115200);
  delay(100);
  for(int i=0;i<10;i++){
    servos[i].setPeriodHertz(SERVO_FREQ);
    servos[i].attach(servoPins[i],SERVO_MIN_US,SERVO_MAX_US);
    servos[i].write(POS_CENTER);
    delay(20);
  }
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  Serial.print("Connecting to WiFi");
  unsigned long t0=millis();
  while(WiFi.status()!=WL_CONNECTED&&millis()-t0<15000){Serial.print(".");delay(300);}
  Serial.println();
  if(WiFi.status()==WL_CONNECTED){Serial.print("IP: ");Serial.println(WiFi.localIP());}
  else Serial.println("WiFi connect failed, continuing AP-less");

  server.onNotFound([](){
    String path=server.uri();
    if(path.startsWith("/setcmd/")) handleSetCmd();
    else server.send(404,"text/plain","Not found");
  });

  server.on("/esp",HTTP_GET,handleEsp);
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop(){
  server.handleClient();
  updateMotions();
}
