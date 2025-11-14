#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ================== SENSOR DHT11 ==================
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================== LCD ==================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== RELÉS ==================
#define RELAY_VENTILADOR 8
#define RELAY_FOCO 13

// ================== VARIABLES ==================
bool ventiladorEncendido = false;
bool focoEncendido = false;
unsigned long lastSend = 0;
const long sendInterval = 3000; // Enviar cada 3 segundos

void setup() {
  Serial.begin(115200);  // Comunicación con ESP32
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(RELAY_VENTILADOR, OUTPUT);
  pinMode(RELAY_FOCO, OUTPUT);

  digitalWrite(RELAY_VENTILADOR, HIGH);
  digitalWrite(RELAY_FOCO, HIGH);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Iniciado");
  lcd.setCursor(0, 1);
  lcd.print("Esperando datos...");
  
  delay(2000);
  Serial.println("🚀 Arduino iniciado - Listo para enviar datos al ESP32");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    mostrarErrorSensor();
    return;
  }

  // Lógica del ventilador
  if (temp >= 24.0 && !ventiladorEncendido) {
    ventiladorEncendido = true;
  } else if (temp < 23.0 && ventiladorEncendido) {
    ventiladorEncendido = false;
  }

  // Lógica del foco
  if (temp >= 21.0 && !focoEncendido && !ventiladorEncendido) {
    focoEncendido = true;
  } else if ((temp < 20.0 || ventiladorEncendido) && focoEncendido) {
    focoEncendido = false;
  }

  // Aplicar estados a los relés
  digitalWrite(RELAY_VENTILADOR, ventiladorEncendido ? LOW : HIGH);
  digitalWrite(RELAY_FOCO, focoEncendido ? LOW : HIGH);

  // Mostrar en LCD
  mostrarEnLCD(temp, hum);

  // Enviar datos al ESP32 cada 3 segundos
  if (millis() - lastSend >= sendInterval) {
    enviarDatos(temp, hum, ventiladorEncendido, focoEncendido);
    lastSend = millis();
  }

  delay(1000);
}

void mostrarErrorSensor() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Error sensor DHT11");
  lcd.setCursor(0, 1);
  lcd.print("Verificar conexion");
  delay(3000);
}

void mostrarEnLCD(float temp, float hum) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print("C H:");
  lcd.print(hum, 1);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("V:");
  lcd.print(ventiladorEncendido ? "ON " : "OFF");
  lcd.print(" F:");
  lcd.print(focoEncendido ? "ON" : "OFF");
}

void enviarDatos(float temp, float hum, bool vent, bool foco) {
  // Formato: "DATA:temperatura,humedad,ventilador,foco"
  Serial.print("DATA:");
  Serial.print(temp, 1);  // 1 decimal
  Serial.print(",");
  Serial.print(hum, 1);   // 1 decimal
  Serial.print(",");
  Serial.print(vent ? "1" : "0");
  Serial.print(",");
  Serial.println(foco ? "1" : "0");
  
  // Debug en monitor serial
  Serial.print("📤 Enviado: ");
  Serial.print(temp, 1);
  Serial.print("C, ");
  Serial.print(hum, 1);
  Serial.print("%, Vent:");
  Serial.print(vent ? "ON" : "OFF");
  Serial.print(", Foco:");
  Serial.println(foco ? "ON" : "OFF");
}