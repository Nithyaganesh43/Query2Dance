#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ================== Wi-Fi ==================
const char* WIFI_SSID = "Sorry";   // replace if needed
const char* WIFI_PASS = "aaaaaaaa"; // replace if needed

// ================== HTTP Server ==================
WebServer server(80);

// ================== Servos (existing) ==================
int servoPins[10] = {16,17,18,19,21,22,23,25,26,27};
const int SERVO_MIN_US = 520;
const int SERVO_MAX_US = 2380;
const int SERVO_FREQ   = 50;
Servo servos[10];
volatile bool active[10] = {false};
unsigned long startMs[10] = {0};

const int POS_CENTER = 90;
const int POS_RIGHT  = 170;
const int POS_LEFT   = 10;

// Motion timing
const unsigned long RUN_DURATION_MS = 30000;     // total run per channel (kept as in your code)
const unsigned long LEG_DURATION_MS = 2000;      // 2s per leg (Right->Left or Left->Right), no endpoint wait

// Smooth sweep state (per-servo)
unsigned long legStartMs[10] = {0};
int startAngle[10] = {0};
int targetAngle[10] = {0};

// Modes
volatile int currentMode = 0; // 0 = normal setcmd, 1 = auto sequence
unsigned long lastSequenceStep = 0;
int seqStep = 0;

// ================== Relays (new) ==================
// Pick safe output GPIOs for relays (active-low typical boards)
const int RELAY1_PIN = 32; // /light1
const int RELAY2_PIN = 33; // /light2
bool relay1On = false;     // logical state (true = ON)
bool relay2On = false;     // logical state (true = ON)

// Map logical ON/OFF to pin level for active-low relay modules:
// ON -> LOW (energized), OFF -> HIGH (de-energized)
inline void applyRelayState() {
  digitalWrite(RELAY1_PIN, relay1On ? LOW : HIGH);
  digitalWrite(RELAY2_PIN, relay2On ? LOW : HIGH);
}

void toggleRelay1() {
  relay1On = !relay1On;
  digitalWrite(RELAY1_PIN, relay1On ? LOW : HIGH);
}

void toggleRelay2() {
  relay2On = !relay2On;
  digitalWrite(RELAY2_PIN, relay2On ? LOW : HIGH);
}

// ================== Smooth sweep helpers ==================
inline int lerpAngle(int a, int b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  return a + (int)((b - a) * t);
}

// Start a new leg for channel i: from 'fromA' to 'toA'
void beginLeg(int i, int fromA, int toA) {
  startAngle[i] = fromA;
  targetAngle[i] = toA;
  legStartMs[i] = millis();
  servos[i].write(fromA);  // set starting point immediately
}

// ================== Servos logic (updated for smooth motion) ==================
void startMotion(int ch) {
  active[ch] = true;
  startMs[ch] = millis();
  // Begin at current center to right first; then ping-pong R<->L continuously with no endpoint wait
  beginLeg(ch, POS_CENTER, POS_RIGHT);
}

void stopMotion(int ch) {
  active[ch] = false;
  servos[ch].write(POS_CENTER);
}

void stopAll() {
  for (int i = 0; i < 10; i++) stopMotion(i);
}

// Smooth, non-blocking sweep: always in motion, 2s per leg, immediate reversal at endpoints
void updateMotions() {
  unsigned long now = millis();
  for (int i = 0; i < 10; i++) {
    if (!active[i]) continue;

    // Optional overall run captoggle
    if (now - startMs[i] >= RUN_DURATION_MS) { stopMotion(i); continue; }

    unsigned long elapsed = now - legStartMs[i];
    float t = (float)elapsed / (float)LEG_DURATION_MS;

    if (t >= 1.0f) {
      // Finish current leg exactly at target angle
      servos[i].write(targetAngle[i]);
      // Immediately start reverse leg with no wait
      int nextFrom = targetAngle[i];
      int nextTo = (targetAngle[i] == POS_RIGHT) ? POS_LEFT : POS_RIGHT;
      beginLeg(i, nextFrom, nextTo);
    } else {
      // Interpolate smoothly
      int angle = lerpAngle(startAngle[i], targetAngle[i], t);
      servos[i].write(angle);
    }
  }
}

// Auto sequence uses startMotion to kick off channels;
// the continuous sweep is handled in updateMotions()
void runSequence() {
  unsigned long now = millis();
  if (now - lastSequenceStep < 2000) return; // sequencing cadence; motion itself is continuous
  lastSequenceStep = now;
  stopAll();

  switch (seqStep) {
    case 0: startMotion(0); startMotion(1); startMotion(2); startMotion(3); break;
    case 1: startMotion(4); startMotion(5); startMotion(6); startMotion(7); break;
    case 2: startMotion(8); startMotion(9); break;
    case 3: for (int i = 0; i < 10; i++) startMotion(i); break;
    case 4: startMotion(0); startMotion(1); startMotion(2); startMotion(3); break;
    case 5: for (int i = 0; i < 10; i++) startMotion(i); break;
    case 6: startMotion(4); startMotion(5); startMotion(6); startMotion(7); break;
    case 7: for (int i = 0; i < 10; i++) startMotion(i); break;
    case 8: startMotion(8); startMotion(9); break;
    case 9: for (int i = 0; i < 10; i++) startMotion(i); break;
  }
  seqStep = (seqStep + 1) % 10;
}

// ================== HTTP Handlers ==================
void handleSetCmd() {
  if (currentMode == 1) { server.send(409, "text/plain", "Mode=1, cannot accept setcmd"); return; }
  String path = server.uri();
  String bits = path.substring(String("/setcmd/").length());
  if (bits.length() != 10) { server.send(400, "text/plain", "Bitstring must be length 10"); return; }
  for (int i = 0; i < 10; i++) {
    if (bits[i] != '0' && bits[i] != '1') { server.send(400, "text/plain", "Bitstring must contain only 0/1"); return; }
  }
  for (int i = 0; i < 10; i++) { if (bits[i] == '1') startMotion(i); else stopMotion(i); }
  server.send(200, "application/json", "{\"status\":\"ok\",\"cmd\":\"" + bits + "\"}");
}

void handleToggleMode() {
  currentMode = (currentMode == 0) ? 1 : 0;
  stopAll();
  server.send(200, "application/json", "{\"mode\":" + String(currentMode) + "}");
}

void handleEsp() { server.send(200, "text/plain", "yes i am"); }

// New: /light1 toggle endpoint
void handleLight1() {
  toggleRelay1();
  String body = String("{\"light1\":\"") + (relay1On ? "on" : "off") + "\"}";
  server.send(200, "application/json", body);
}

// New: /light2 toggle endpoint
void handleLight2() {
  toggleRelay2();
  String body = String("{\"light2\":\"") + (relay2On ? "on" : "off") + "\"}";
  server.send(200, "application/json", body);
}

// ================== Setup / Loop ==================
void setup() {
  Serial.begin(115200);
  delay(100);

  // Servos init
  for (int i = 0; i < 10; i++) {
    servos[i].setPeriodHertz(SERVO_FREQ);
    servos[i].attach(servoPins[i], SERVO_MIN_US, SERVO_MAX_US);
    servos[i].write(POS_CENTER);
    delay(20);
  }
  delay(200);

  // Relays init (active-low default -> keep OFF at boot)
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH); // OFF
  digitalWrite(RELAY2_PIN, HIGH); // OFF

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { Serial.print("."); delay(300); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) { Serial.print("IP: "); Serial.println(WiFi.localIP()); }
  else Serial.println("WiFi connect failed, continuing AP-less");

  // Routes
  server.onNotFound([]() {
    String path = server.uri();
    if (path.startsWith("/setcmd/")) handleSetCmd();
    else server.send(404, "text/plain", "Not found");
  });

  server.on("/esp", HTTP_GET, handleEsp);
  server.on("/togglemode", HTTP_GET, handleToggleMode);

  // New relay toggle routes
  server.on("/light1", HTTP_GET, handleLight1);
  server.on("/light2", HTTP_GET, handleLight2);

  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
  if (currentMode == 1) runSequence();
  updateMotions(); // smooth, continuous sweeps with no endpoint wait
}
