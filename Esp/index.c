#include <WiFi.h>
#include <WebServer.h>

// WiFi
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// HTTP server
WebServer server(80);

// Servos: 10 direct pins
const int DOLL_COUNT = 10;
int servoPins[DOLL_COUNT] = {13, 14, 15, 16, 17, 18, 19, 21, 22, 23};

// LEDC PWM config
const int SERVO_FREQ = 50;
const int SERVO_TIMER_BITS = 16;

// Pulse width limits and initial angle
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;
const int INIT_ANGLE   = 90;

// Motion timing
const uint32_t HALF_CYCLE_MS = 2000;   // 2 s per half move (right, then left)
const uint32_t TOTAL_MS      = 30000;  // 30 s total

// Helpers to convert microseconds to LEDC duty counts
inline uint32_t usToCounts(int us) {
  return (uint32_t)((uint64_t)us * 65535ULL / 20000ULL); // 20 ms period at 50 Hz
}

inline uint32_t angleToCounts(int angle) {
  int us = SERVO_MIN_US + (SERVO_MAX_US - SERVO_MIN_US) * angle / 180;
  return usToCounts(us);
}

void setAngle(int ch, int angle) {
  uint32_t duty = angleToCounts(angle);
  ledcWrite(ch, duty);
}

void moveToInitial(int ch) { setAngle(ch, INIT_ANGLE); }

void sweepTo(int ch, int fromAngle, int toAngle, uint32_t durationMs) {
  const int steps = 100;
  for (int i = 0; i <= steps; i++) {
    int a = fromAngle + (toAngle - fromAngle) * i / steps;
    setAngle(ch, a);
    delay(durationMs / steps);
  }
}

void runMotion(int ch) {
  const int CENTER = 90, RIGHT = 0, LEFT = 180;
  setAngle(ch, CENTER);
  uint32_t start = millis();
  while (millis() - start < TOTAL_MS) {
    sweepTo(ch, CENTER, RIGHT, HALF_CYCLE_MS);
    setAngle(ch, CENTER);
    sweepTo(ch, CENTER, LEFT, HALF_CYCLE_MS);
    setAngle(ch, CENTER);
  }
  setAngle(ch, CENTER);
}

// Validate that a string has exactly 10 chars and only '0'/'1'
bool isValidBitstring(const String& s) {
  if (s.length() != DOLL_COUNT) return false;
  for (int i = 0; i < DOLL_COUNT; i++) {
    char c = s[i];
    if (c != '0' && c != '1') return false;
  }
  return true;
}

// Handler for /setcmd/<bitstring>
void handleSetCmdPath() {
  // Example URIs:
  //  - /setcmd/1011000001
  //  - /setcmd/0000000000
  String uri = server.uri(); // full path
  // Extract the part after "/setcmd/"
  const String prefix = "/setcmd/";
  if (!uri.startsWith(prefix)) {
    server.send(404, "application/json", "{\"error\":\"Not found\"}");
    return;
  }
  String bitstring = uri.substring(prefix.length());

  if (!isValidBitstring(bitstring)) {
    server.send(400, "application/json", "{\"error\":\"Invalid bitstring (expect 10 chars of 0/1)\"}");
    return;
  }

  // Execute actions
  for (int i = 0; i < DOLL_COUNT; i++) {
    if (bitstring[i] == '1') runMotion(i);
    else moveToInitial(i);
  }

  // Respond OK
  String ok = String("{\"ok\":true,\"bitstring\":\"") + bitstring + "\"}";
  server.send(200, "application/json", ok);
}

void setup() {
  Serial.begin(115200);

  // Setup LEDC channels and attach pins
  for (int ch = 0; ch < DOLL_COUNT; ch++) {
    ledcSetup(ch, SERVO_FREQ, SERVO_TIMER_BITS);
    ledcAttachPin(servoPins[ch], ch);
    moveToInitial(ch);
  }

  // Wi-Fi connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Route for /setcmd/<bitstring>
  // Use onNotFound to catch dynamic path after prefix, or add two routes:
  server.on("/setcmd", HTTP_ANY, []() {
    server.send(400, "application/json", "{\"error\":\"Path must be /setcmd/<10-bit bitstring>\"}");
  });

  // Use onNotFound to handle /setcmd/<bitstring>
  server.onNotFound([]() {
    String uri = server.uri();
    if (uri.startsWith("/setcmd/")) {
      handleSetCmdPath();
    } else {
      server.send(404, "application/json", "{\"error\":\"Not found\"}");
    }
  });

  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
}
