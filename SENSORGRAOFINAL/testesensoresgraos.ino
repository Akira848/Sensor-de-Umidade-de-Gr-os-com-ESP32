#include <DHT.h>

// --- PINOS ---
#define DHTPIN 18          // Sensor de Ar (DHT22) no D18
#define DHTTYPE DHT22
#define SENSOR_GRAO 34     // Sensor de Grão (Capacitivo) no D34

// --- INICIALIZAÇÃO ---
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200); 
  Serial.println("======================================");
  Serial.println("    TESTE DE SENSORES (AR E GRAO)     ");
  Serial.println("======================================");
  
  dht.begin();
}

void loop() {
  // 1. Lendo Umidade e Temperatura do Ar (DHT22)
  float umidAr = dht.readHumidity();
  float tempAr = dht.readTemperature();

  // 2. Lendo Sensor Capacitivo do Grão (Valor Bruto de 0 a 4095)
  int leituraRaw = analogRead(SENSOR_GRAO);

  // --- EXIBIÇÃO NO MONITOR SERIAL ---
  Serial.println("\n--- LEITURA ATUAL ---");
  
  // Teste DHT22
  Serial.print("SENS. AR    - Umidade: ");
  if (isnan(umidAr)) {
    Serial.println("ERRO! (Verifique fiação do DHT22)");
  } else {
    Serial.print(umidAr); 
    Serial.print("% | Temp: "); 
    Serial.print(tempAr); 
    Serial.println(" C");
  }

  // Teste Capacitivo
  Serial.print("SENS. GRAO  - Umid. (Bruto): ");
  Serial.print(leituraRaw);
  
  // Dica de Calibração rápida
  if(leituraRaw > 3000) {
    Serial.println(" -> [SECO]");
  } else if(leituraRaw < 2000) {
    Serial.println(" -> [UMIDO]");
  } else {
    Serial.println(" -> [MEIO TERMO]");
  }

  Serial.println("--------------------------------------");
  
  delay(2000); // Aguarda 2 segundos
}