#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Sensor DHT11
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LCD con I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Relés
#define RELAY_VENTILADOR 8
#define RELAY_FOCO 13

// Estados
bool ventiladorEncendido = false;
bool focoEncendido = false;

// Control de envío
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
  
  Serial.println("🚀 Arduino iniciado - Listo para enviar datos");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error sensor DHT11");
    delay(3000);
    return;
  }

  // Lógica del ventilador
  if (!ventiladorEncendido && temp >= 24.0) {
    ventiladorEncendido = true;
  } else if (ventiladorEncendido && temp < 23.0) {
    ventiladorEncendido = false;
  }

  // Lógica del foco
  if (!focoEncendido && temp >= 21.0 && !ventiladorEncendido) {
    focoEncendido = true;
  } else if (focoEncendido && (temp < 20.0 || ventiladorEncendido)) {
    focoEncendido = false;
  }

  // Aplicar estados a los relés
  digitalWrite(RELAY_VENTILADOR, ventiladorEncendido ? LOW : HIGH);
  digitalWrite(RELAY_FOCO, focoEncendido ? LOW : HIGH);

  // Mostrar en LCD
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

  // Enviar datos al ESP32 cada 3 segundos
  if (millis() - lastSend >= sendInterval) {
    enviarDatos(temp, hum, ventiladorEncendido, focoEncendido);
    lastSend = millis();
  }

  delay(1000);
}

void enviarDatos(float temp, float hum, bool vent, bool foco) {
  Serial.print("DATA:");
  Serial.print(temp, 1);  // 1 decimal
  Serial.print(",");
  Serial.print(hum, 1);   // 1 decimal
  Serial.print(",");
  Serial.print(vent ? "1" : "0");
  Serial.print(",");
  Serial.println(foco ? "1" : "0");
  
  Serial.print("📤 Enviado: ");
  Serial.print(temp, 1);
  Serial.print("C, ");
  Serial.print(hum, 1);
  Serial.print("%, Vent:");
  Serial.print(vent ? "ON" : "OFF");
  Serial.print(", Foco:");
  Serial.println(foco ? "ON" : "OFF");
}