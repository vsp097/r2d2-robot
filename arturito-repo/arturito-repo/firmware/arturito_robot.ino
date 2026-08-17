// ============================================================
// ARTURITO - R2-D2 Inspired Mobile Robot (ESP32)
// Main firmware: WiFi server, motor control, sensor, laser, modes
// ============================================================

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include "index_html.h"

// ---------- Home WiFi credentials ----------
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
AsyncWebServer server(80);

// Optional: direct connection by MAC + channel (faster reconnect)
uint8_t bssid[] = {0x32, 0x16, 0x9D, 0x79, 0x6C, 0x71};
int wifiChannel = 6;

// ---------- Dome servo ----------
Servo domeServo;
const int servoPin = 13;
int centerPosition = 90;

// ---------- Motors ----------
// Driver 1: rear motors (tricycle-style traction)
const int IN1 = 26; const int IN2 = 27;
const int IN3 = 32; const int IN4 = 33;
// Driver 2: front motor
const int IN5 = 25; const int IN6 = 14;

// ---------- HC-SR04 ultrasonic sensor ----------
const int trigPin = 5;
const int echoPin = 18;

// ---------- Laser emitter ----------
const int laserPin = 23;

// ============================================================
// Motor control
// ============================================================
void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  digitalWrite(IN5, LOW); digitalWrite(IN6, LOW);
}

// ============================================================
// Reads distance to the nearest obstacle (cm) using pulse timing
// ============================================================
float readDistance() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1; // out-of-range / timeout
  return duration * 0.0343 / 2; // round-trip -> one-way distance
}

// ============================================================
// Emotion animations
// ============================================================
void animationHappy() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
    digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
    delay(200);
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
    delay(200);
  }
  stopMotors();
}

void animationAngry() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
  delay(400);
  stopMotors();
  delay(200);
  domeServo.write(60); delay(150);
  domeServo.write(120); delay(150);
  domeServo.write(centerPosition);
}

void animationScared() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
  delay(600);
  stopMotors();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(IN5, OUTPUT); pinMode(IN6, OUTPUT);
  stopMotors();

  pinMode(trigPin, OUTPUT); pinMode(echoPin, INPUT);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, HIGH); // off by default

  ESP32PWM::allocateTimer(0);
  domeServo.setPeriodHertz(50);
  domeServo.attach(servoPin, 500, 2400);
  domeServo.write(centerPosition);

  Serial.println("=========================================");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  WiFi.begin(ssid, password, wifiChannel, bssid);
  Serial.print("Connecting to WiFi... ");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("Robot IP: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=========================================");
  } else {
    Serial.println("\n[ERROR] Could not connect to WiFi.");
    Serial.println("=========================================");
  }

  // --- WEB SERVER ROUTES ---

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGINA_HTML);
  });

  // Telemetry endpoint: returns live distance as JSON
  server.on("/api/estado", HTTP_ANY, [](AsyncWebServerRequest *request){
    float d = readDistance();
    String json = "{\"distancia\":" + String(d) + "}";
    request->send(200, "application/json", json);
  });

  // Command endpoint: receives and executes robot actions
  server.on("/api/cmd", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (request->hasArg("c")) {
      String cmd = request->arg("c");

      if (cmd == "adelante") {
        digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
        digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
      }
      else if (cmd == "atras") {
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
        digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
      }
      else if (cmd == "izquierda") {
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
        digitalWrite(IN5, LOW);  digitalWrite(IN6, LOW);
      }
      else if (cmd == "derecha") {
        digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
        digitalWrite(IN5, LOW);  digitalWrite(IN6, LOW);
      }
      else if (cmd == "parar" || cmd == "para_todo") {
        stopMotors();
      }
      else if (cmd == "feliz") { animationHappy(); }
      else if (cmd == "enojado") { animationAngry(); }
      else if (cmd == "asustado") { animationScared(); }
      else if (cmd == "laser_on") { digitalWrite(laserPin, LOW); }
      else if (cmd == "laser_off") { digitalWrite(laserPin, HIGH); }
      else if (cmd == "mira_izq") {
        domeServo.write(135); delay(500); domeServo.write(centerPosition);
      }
      else if (cmd == "mira_der") {
        domeServo.write(45); delay(500); domeServo.write(centerPosition);
      }
      else if (cmd.startsWith("audio:")) {
        // Audio playback is handled client-side (see /web). No-op here.
      }

      // ============================================================
      // PERSONALITY MODES (timed motor + light sequences)
      // ============================================================

      else if (cmd == "modo_feliz") {
        for (int i = 0; i < 2; i++) {
          digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
          digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
          digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
          delay(400);
          digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
          digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
          delay(400);
        }
        stopMotors();
      }

      else if (cmd == "modo_fiesta") {
        for (int i = 0; i < 4; i++) {
          digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
          digitalWrite(IN5, LOW);  digitalWrite(IN6, LOW);
          delay(500);
          digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
          digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
          digitalWrite(IN5, LOW);  digitalWrite(IN6, LOW);
          delay(500);
          digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
          digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
          digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
          delay(400);
          digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
          digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
          delay(400);
        }
        stopMotors();
      }

      else if (cmd == "modo_molesto") {
        for (int i = 0; i < 6; i++) {
          digitalWrite(laserPin, LOW);
          delay(150);
          digitalWrite(laserPin, HIGH);
          delay(150);
        }
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
        digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
        delay(2000);
        stopMotors();
      }

      // Autonomous obstacle-avoidance mode (max 10s), perception-action loop
      else if (cmd == "modo_messi") {
        unsigned long startTime = millis();
        unsigned long lastSample = 0;

        digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
        digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);

        while (millis() - startTime < 10000) {
          unsigned long now = millis();

          if (now - lastSample >= 100) {
            lastSample = now;
            float distance = readDistance();

            if (distance > 0 && distance < 20) {
              stopMotors();
              delay(100);

              digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
              digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
              digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
              delay(400);

              digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
              digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
              digitalWrite(IN5, LOW);  digitalWrite(IN6, LOW);
              delay(500);

              digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
              digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
              digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
            }
          }
          yield(); // keep WiFi/async server responsive during the loop
        }
        stopMotors();
      }

      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing command");
    }
  });

  server.begin();
}

void loop() {
  delay(50);
}
