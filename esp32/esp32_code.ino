#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// ==================== CONFIGURACIÓN WIFI ====================
const char* ssid = "TU_RED_WIFI";          // ← CAMBIA ESTO
const char* password = "TU_PASSWORD_WIFI"; // ← CAMBIA ESTO

// ==================== URL DEL SERVIDOR ====================
const char* serverURL = "https://tu-app.onrender.com/api/data"; // ← CAMBIA ESTO

// ==================== COMUNICACIÓN CON ARDUINO ====================
HardwareSerial SerialArduino(1);  // Usar Serial1 para comunicación con Arduino
#define RX_PIN 16                 // ESP32 RX ← Arduino TX
#define TX_PIN 17                 // ESP32 TX → Arduino RX (no necesario)

// ==================== VARIABLES DE CONTROL ====================
unsigned long lastSend = 0;
const long sendInterval = 5000;   // Enviar datos cada 5 segundos

void setup() {
  // Iniciar comunicación con PC (Monitor Serial)
  Serial.begin(115200);
  
  // Iniciar comunicación con Arduino
  SerialArduino.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println();
  Serial.println("🚀 Iniciando ESP32 - Puente Arduino→Internet");
  
  // Conectar a WiFi
  conectarWiFi();
  
  Serial.println("✅ ESP32 listo para recibir datos del Arduino");
  Serial.println("📡 Esperando datos en formato: DATA:temperatura,humedad,ventilador,foco");
}

void loop() {
  // Verificar y mantener conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi desconectado - Reconectando...");
    conectarWiFi();
  }
  
  // Leer datos del Arduino
  if (SerialArduino.available()) {
    String data = SerialArduino.readStringUntil('\n');
    data.trim();
    
    if (data.startsWith("DATA:")) {
      Serial.print("📨 Dato recibido de Arduino: ");
      Serial.println(data);
      
      // Procesar y enviar datos al servidor web
      procesarYEnviarDatos(data);
    }
  }
  
  // Enviar heartbeat cada 30 segundos
  if (millis() - lastSend > 30000) {
    enviarHeartbeat();
    lastSend = millis();
  }
  
  delay(100);
}

void conectarWiFi() {
  Serial.println();
  Serial.print("📡 Conectando a WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi conectado!");
    Serial.print("📶 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("❌ Error conectando a WiFi");
  }
}

void procesarYEnviarDatos(String data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi no conectado - No se pueden enviar datos");
    return;
  }
  
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");
  
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
    
    Serial.print("📤 Enviando datos al servidor... ");
    Serial.println(jsonString);
    
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode == 200) {
      Serial.println("✅ Datos enviados exitosamente al servidor!");
    } else {
      Serial.print("❌ Error enviando datos. Código: ");
      Serial.println(httpResponseCode);
      
      // Intentar obtener respuesta de error
      String response = http.getString();
      Serial.println("Respuesta del servidor: " + response);
    }
    
    http.end();
  } else {
    Serial.println("❌ Formato de datos incorrecto del Arduino");
    Serial.println("Formato esperado: DATA:temperatura,humedad,ventilador,foco");
  }
}

void enviarHeartbeat() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(serverURL));
    http.addHeader("Content-Type", "application/json");
    
    DynamicJsonDocument doc(100);
    doc["device"] = "ESP32";
    doc["status"] = "online";
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int responseCode = http.POST(jsonString);
    
    if (responseCode == 200) {
      Serial.println("💓 Heartbeat enviado - ESP32 online");
    }
    
    http.end();
  }
}