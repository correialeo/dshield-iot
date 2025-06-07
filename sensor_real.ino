#include "painlessMesh.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>

#define TRIGGER_PIN 18    // Pino Trig do HC-SR04
#define ECHO_PIN 19       // Pino Echo do HC-SR04
#define MQ2_PIN 39        // Pino analógico do MQ2

#define MESH_PREFIX "EmergencyMesh"
#define MESH_PASSWORD "mesh123456"
#define MESH_PORT 5555

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

#define API_URL "https://cd18-177-139-13-210.ngrok-free.app/api/data"   

#define WATER_LED_PIN 2
#define SMOKE_LED_PIN 4
#define WIFI_LED_PIN 5

LiquidCrystal lcd(12, 13, 14, 27, 26, 25);

painlessMesh mesh;
WiFiClient wifiClient;

bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 30000;

unsigned long lastWaterReading = 0;
unsigned long lastSmokeReading = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long waterReadingInterval = 3000;
const unsigned long smokeReadingInterval = 2000;
const unsigned long displayUpdateInterval = 1000;

float waterLevel = 20.0;
float smokeLevel = 10.0;
bool waterAlert = false;
bool smokeAlert = false;

const uint32_t GATEWAY_NODE_ID = 1000;
const uint32_t WATER_SENSOR_NODE_ID = 2000;
const uint32_t SMOKE_SENSOR_NODE_ID = 3000;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("[DEBUG] Serial iniciado!");
  
  pinMode(WATER_LED_PIN, OUTPUT);
  pinMode(SMOKE_LED_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Emergency Mesh");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  digitalWrite(WATER_LED_PIN, LOW);
  digitalWrite(SMOKE_LED_PIN, LOW);
  digitalWrite(WIFI_LED_PIN, LOW);
  
  Serial.println();
  Serial.println("=================================");
  Serial.println("=== EMERGENCY MESH SYSTEM ===");
  Serial.println("=================================");
  Serial.println("Simulando 3 nós em 1 ESP32:");
  Serial.println("- Gateway (ID: 1000)");
  Serial.println("- Water Sensor (ID: 2000, deviceId: 1)");
  Serial.println("- Smoke Sensor (ID: 3000, deviceId: 2)");
  Serial.println("=================================");
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  connectToWiFi();
  
  Serial.println("✅ Sistema pronto!");
  Serial.println("🔧 Sensores reais configurados");
  Serial.println("=====================================");
  
  delay(2000);
}

void loop() {
  mesh.update();
  
  simulateWaterSensor();
  simulateSmokeSensor();
  
  updateDisplay();
  
  if (millis() - lastWifiCheck > wifiCheckInterval) {
    checkWiFiConnection();
    lastWifiCheck = millis();
  }
  
  delay(50);
}

void simulateWaterSensor() {
  if (millis() - lastWaterReading > waterReadingInterval) {
    // Medição com HC-SR04
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    waterLevel = duration * 0.034 / 2;
    
    float noise = random(-10, 11) / 10.0;
    waterLevel += noise;
    
    if (waterLevel < 0) waterLevel = 0;
    if (waterLevel > 200) waterLevel = 200;
    
    Serial.printf("🌊 [WATER SENSOR] Nível: %.1f cm (duração: %ldµs)\n", waterLevel, duration);
    
    if (waterLevel > 100) {
      waterAlert = true;
      digitalWrite(WATER_LED_PIN, HIGH);
      Serial.println("⚠️ [WATER SENSOR] ALERTA CRÍTICO: Enchente!");
    } else if (waterLevel > 50) {
      waterAlert = true;
      digitalWrite(WATER_LED_PIN, HIGH);
      Serial.println("⚠️ [WATER SENSOR] Atenção: Nível alto");
    } else {
      waterAlert = false;
      digitalWrite(WATER_LED_PIN, LOW);
    }
    
    simulateMeshMessage(WATER_SENSOR_NODE_ID, 1, waterLevel, "water_level");
    lastWaterReading = millis();
  }
}

void simulateSmokeSensor() {
  if (millis() - lastSmokeReading > smokeReadingInterval) {
    int adcValue = analogRead(MQ2_PIN);
    smokeLevel = map(adcValue, 0, 4095, 5, 1000); 
    
    float noise = random(-20, 21) / 10.0;
    smokeLevel += noise;
    
    if (smokeLevel < 5) smokeLevel = 5;
    if (smokeLevel > 1000) smokeLevel = 1000;
    
    Serial.printf("🔥 [SMOKE SENSOR] Fumaça: %.1f ppm (ADC: %d)\n", smokeLevel, adcValue);
    
    if (smokeLevel > 400) {
      smokeAlert = true;
      digitalWrite(SMOKE_LED_PIN, HIGH);
      Serial.println("⚠️ [SMOKE SENSOR] ALERTA CRÍTICO: Incêndio!");
    } else if (smokeLevel > 200) {
      smokeAlert = true;
      digitalWrite(SMOKE_LED_PIN, HIGH);
      Serial.println("⚠️ [SMOKE SENSOR] ALERTA: Fumaça alta!");
    } else if (smokeLevel > 100) {
      smokeAlert = true;
      digitalWrite(SMOKE_LED_PIN, HIGH);
      Serial.println("⚠️ [SMOKE SENSOR] Atenção: Fumaça detectada");
    } else {
      smokeAlert = false;
      digitalWrite(SMOKE_LED_PIN, LOW);
    }
    
    simulateMeshMessage(SMOKE_SENSOR_NODE_ID, 2, smokeLevel, "smoke");
    lastSmokeReading = millis();
  }
}

void updateDisplay() {
  if (millis() - lastDisplayUpdate > displayUpdateInterval) {
    lcd.clear();
    
    lcd.setCursor(0, 0);
    if (wifiConnected) {
      lcd.print("WiFi:OK ");
    } else {
      lcd.print("WiFi:-- ");
    }
    lcd.printf("W:%.0f S:%.0f", waterLevel, smokeLevel);
    
    lcd.setCursor(0, 1);
    if (waterAlert && smokeAlert) {
      lcd.print("ALERT: AGUA+FOGO");
    } else if (waterAlert) {
      lcd.print("ALERTA: ENCHENTE");
    } else if (smokeAlert) {
      lcd.print("ALERTA: INCENDIO");
    } else {
      lcd.print("Sistema Normal  ");
    }
    
    lastDisplayUpdate = millis();
  }
}

void simulateMeshMessage(uint32_t fromNodeId, int deviceId, float value, String sensorType) {
  DynamicJsonDocument doc(512);
  doc["deviceId"] = deviceId;
  doc["value"] = round(value * 10) / 10.0;
  doc["timestamp"] = millis();
  doc["sensorType"] = sensorType;
  
  String msg;
  serializeJson(doc, msg);
  
  Serial.printf("📡 [MESH] Nó %u → Gateway: %s\n", fromNodeId, msg.c_str());
  
  processReceivedData(fromNodeId, msg);
}

void processReceivedData(uint32_t from, String msg) {
  DynamicJsonDocument doc(512);
  deserializeJson(doc, msg);
  
  int deviceId = doc["deviceId"];
  float value = doc["value"];
  
  Serial.printf("🔄 [GATEWAY] Processando - DeviceId: %d, Value: %.1f\n", deviceId, value);
  
  if (wifiConnected) {
    sendToAPI(deviceId, value);
  } else {
    Serial.println("❌ [GATEWAY] WiFi desconectado - dados armazenados localmente");
  }
}

void connectToWiFi() {
  Serial.println("🔗 [GATEWAY] Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(WIFI_LED_PIN, HIGH);
    Serial.println();
    Serial.println("✅ [GATEWAY] WiFi conectado!");
    Serial.printf("📶 IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    wifiConnected = false;
    digitalWrite(WIFI_LED_PIN, LOW);
    Serial.println();
    Serial.println("❌ [GATEWAY] Falha na conexão WiFi - continuando offline");
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("📡 [GATEWAY] WiFi desconectado!");
    }
    wifiConnected = false;
    digitalWrite(WIFI_LED_PIN, LOW);
  } else if (!wifiConnected) {
    wifiConnected = true;
    digitalWrite(WIFI_LED_PIN, HIGH);
    Serial.println("📡 [GATEWAY] WiFi reconectado!");
  }
}

void sendToAPI(int deviceId, float value) {
  if (!wifiConnected) return;
  
  HTTPClient http;
  http.begin(wifiClient, API_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("ngrok-skip-browser-warning", "true");
  http.addHeader("Accept", "application/json");
  
  DynamicJsonDocument doc(256);
  doc["deviceId"] = deviceId;
  doc["value"] = value;
  
  String payload;
  serializeJson(doc, payload);
  
  Serial.printf("📤 [GATEWAY] Enviando para API: %s\n", payload.c_str());
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("✅ [API] Resposta (%d): %s\n", httpResponseCode, response.c_str());
  } else {
    Serial.printf("❌ [API] Erro na requisição: %d\n", httpResponseCode);
  }
  
  http.end();
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("📡 [MESH] Mensagem externa recebida do nó %u: %s\n", from, msg.c_str());
  processReceivedData(from, msg);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("🔗 [MESH] Nova conexão estabelecida: %u\n", nodeId);
  Serial.printf("📊 [MESH] Total de nós conectados: %d\n", mesh.getNodeList().size() + 1);
}

void changedConnectionCallback() {
  Serial.printf("🔄 [MESH] Topologia alterada. Nós ativos: %d\n", mesh.getNodeList().size() + 1);
  
  auto nodes = mesh.getNodeList();
  if (nodes.size() > 0) {
    Serial.print("📋 [MESH] Nós conectados: ");
    for (auto node : nodes) {
      Serial.printf("%u ", node);
    }
    Serial.println();
  }
}