#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// ================== CONFIGURACIÓN WIFI ==================
const char* ssid = "TU_WIFI_SSID";           // ← CAMBIA ESTO
const char* password = "TU_WIFI_PASSWORD";   // ← CAMBIA ESTO

// ================== URL DE RENDER ==================
const char* serverURL = "https://sistema-monitoreo-dhtil.onrender.com/api/data"; // ← USA TU URL

// ================== COMUNICACIÓN CON ARDUINO ==================
HardwareSerial SerialArduino(1);
#define RX_PIN 16   // ESP32 RX2 ← ARDUINO TX
#define TX_PIN 17   // ESP32 TX2 → ARDUINO RX (no necesario)

// ================== VARIABLES ==================
unsigned long lastSend = 0;
const long sendInterval = 5000; // Enviar cada 5 segundos
bool wifiConnected = false;

void setup() {
  Serial.begin(115200);
  
  // Inicializar comunicación con Arduino
  SerialArduino.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println();
  Serial.println("🚀 ESP32 Iniciado");
  Serial.println("📡 Conectando a WiFi...");
  
  conectarWiFi();
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    Serial.println("❌ WiFi desconectado - Reconectando...");
    conectarWiFi();
  }
  
  // Leer datos del Arduino
  if (SerialArduino.available()) {
    String data = SerialArduino.readStringUntil('\n');
    data.trim();
    
    if (data.startsWith("DATA:")) {
      Serial.print("📨 Dato de Arduino: ");
      Serial.println(data);
      
      // Procesar y enviar datos al servidor
      if (wifiConnected && millis() - lastSend >= sendInterval) {
        procesarYEnviarDatos(data);
        lastSend = millis();
      }
    }
  }
  
  delay(100);
}

void conectarWiFi() {
  WiFi.begin(ssid, password);
  int intentos = 0;
  
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(1000);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✅ Conectado a WiFi!");
    Serial.print("📡 IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("🌐 Servidor: ");
    Serial.println(serverURL);
  } else {
    Serial.println("\n❌ Falló conexión WiFi");
  }
}

void procesarYEnviarDatos(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000); // 10 segundos timeout
    
    // Parsear datos: "DATA:temp,hum,vent,foco"
    // Ejemplo: "DATA:23.5,65.0,1,0"
    data = data.substring(5); // Quitar "DATA:"
    
    int separators[3];
    separators[0] = data.indexOf(',');
    separators[1] = data.indexOf(',', separators[0] + 1);
    separators[2] = data.indexOf(',', separators[1] + 1);
    
    if (separators[0] != -1 && separators[1] != -1 && separators[2] != -1) {
      float temp = data.substring(0, separators[0]).toFloat();
      float hum = data.substring(separators[0] + 1, separators[1]).toFloat();
      bool vent = data.substring(separators[1] + 1, separators[2]).toInt();
      bool foco = data.substring(separators[2] + 1).toInt();
      
      // Crear JSON para enviar
      DynamicJsonDocument doc(200);
      doc["temperatura"] = temp;
      doc["humedad"] = hum;
      doc["ventilador"] = vent;
      doc["foco"] = foco;
      
      String jsonString;
      serializeJson(doc, jsonString);
      
      Serial.println("📤 Enviando al servidor...");
      Serial.print("JSON: ");
      Serial.println(jsonString);
      
      int httpResponseCode = http.POST(jsonString);
      
      if (httpResponseCode == 200) {
        Serial.println("✅ Datos enviados exitosamente!");
      } else {
        Serial.print("❌ Error HTTP: ");
        Serial.println(httpResponseCode);
        
        String response = http.getString();
        Serial.print("Respuesta: ");
        Serial.println(response);
      }
      
      http.end();
    } else {
      Serial.println("❌ Formato de datos incorrecto del Arduino");
    }
  }
}