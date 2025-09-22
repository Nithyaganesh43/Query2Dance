#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const char* ssid="YOUR_WIFI_SSID";
const char* password="YOUR_WIFI_PASSWORD";

ESP8266WebServer server(80);
Adafruit_PWMServoDriver pwm=Adafruit_PWMServoDriver(0x40);

#define SERVO_MIN 150
#define SERVO_MAX 600

const int DOLL_COUNT=10;

void setup(){
  Serial.begin(9600);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){delay(500);}
  pwm.begin();
  pwm.setPWMFreq(60);
  server.on("/setcmd",handleSetCmd);
  server.begin();
}

void loop(){
  server.handleClient();
}

void handleSetCmd(){
  if(!server.hasArg("plain")){server.send(400,"text/plain","Missing bitstring");return;}
  String bitstring=server.arg("plain");
  bitstring.trim();
  if(bitstring.length()!=DOLL_COUNT){server.send(400,"text/plain","Invalid bitstring length");return;}
  for(int i=0;i<DOLL_COUNT;i++){
    if(bitstring[i]=='1'){makeDollDance(i);}
  }
  server.send(200,"text/plain","OK");
}

void makeDollDance(int index){
  int channel=index;
  pwm.setPWM(channel,0,SERVO_MIN);
  delay(300);
  pwm.setPWM(channel,0,SERVO_MAX);
  delay(300);
  pwm.setPWM(channel,0,(SERVO_MIN+SERVO_MAX)/2);
}
