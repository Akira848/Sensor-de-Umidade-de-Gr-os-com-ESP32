#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// --- CONFIGURAÇÕES DE REDE E SERVIDOR ---
const char* ssid       = "";       // COLOQUE O NOME DO SEU WI-FI AQUI
const char* password   = "";      // COLOQUE A SENHA AQUI
const char* serverName = ""; // Se tiver uma rota (ex: /api/dados), coloque no final

// --- CONFIGURAÇÕES DE HARDWARE ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define DHTPIN 18
#define DHTTYPE DHT22
#define SENSOR_GRAO 34
DHT dht(DHTPIN, DHTTYPE);

// --- BOTÕES ---
const int BTN_AVANCAR    = 25;
const int BTN_SELECIONAR = 27;
const int BTN_HOME       = 26;

// --- VARIÁVEIS ---
int opcao = 0;
const int totalOpcoes = 4;
String itens[] = {"CAFE", "SOJA", "MILHO", "PERSONALIZADO"}; 

// --- FUNÇÃO PARA PEGAR DATA E HORA DA INTERNET ---
String pegarDataHoraAtual() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "Erro_Relogio";
  }
  char timeStringBuff[50];
  // Formata no padrão exigido: "YYYY-MM-DD HH:MM:SS"
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(32, 33);
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(BTN_AVANCAR, INPUT_PULLUP);
  pinMode(BTN_SELECIONAR, INPUT_PULLUP);
  pinMode(BTN_HOME, INPUT_PULLUP);

  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configura o fuso horário (UTC-3 para o Brasil)
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SISTEMA PRONTO");
  lcd.setCursor(0, 1);
  lcd.print("WIFI OK");
  delay(1500);
  lcd.clear();
}

void loop() {
  // --- MENU ---
  lcd.setCursor(0, 0);
  lcd.print("SELECIONAR GRAO: ");
  lcd.setCursor(0, 1);
  lcd.print("> " + itens[opcao] + "           ");

  if (digitalRead(BTN_AVANCAR) == LOW) {
    opcao = (opcao + 1) % totalOpcoes;
    delay(250);
    while(digitalRead(BTN_AVANCAR) == LOW);
  }

  if (digitalRead(BTN_SELECIONAR) == LOW) {
    delay(250);
    while(digitalRead(BTN_SELECIONAR) == LOW);
    calcularUmidadeFinal(); 
  }
}

void calcularUmidadeFinal() {
  float somaUmidAr = 0;
  float somaTempAr = 0; 
  long somaleituraCapacitiva = 0;

  lcd.clear();
  
  for (int i = 1; i <= 3; i++) {
    lcd.setCursor(0, 1);
    lcd.print("CALCULANDO... " + String(i) + "/3");
    delay(1000); 

    somaUmidAr += dht.readHumidity();
    somaTempAr += dht.readTemperature();
    somaleituraCapacitiva += analogRead(SENSOR_GRAO);
  }

  // CÁLCULOS DAS MÉDIAS
  float mediaUmidAr = somaUmidAr / 3.0;
  float mediaTempAr = somaTempAr / 3.0; 
  int mediaCapacitiva = somaleituraCapacitiva / 3;
  
  float umidInterna = map(mediaCapacitiva, 3200, 1400, 0, 100);
  umidInterna = constrain(umidInterna, 0, 100);

  // FUSÃO
  float umidFinalGrao = (mediaUmidAr + umidInterna) / 2.0;

  // --- MONTAGEM DO JSON E ENVIO HTTP ---
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Prepara o payload JSON exatamente no formato que você pediu
    String dataHora = pegarDataHoraAtual();
    String jsonPayload = "{";
    jsonPayload += "\"grao\": \"" + itens[opcao] + "\",";
    jsonPayload += "\"umidade\": " + String(umidFinalGrao, 1) + ",";
    jsonPayload += "\"temperatura\": " + String(mediaTempAr, 1) + ",";
    jsonPayload += "\"dhrealizado\": \"" + dataHora + "\"";
    jsonPayload += "}";

    // Envia o POST e pega a resposta
    int httpResponseCode = http.POST(jsonPayload);
    
    // Libera a requisição
    http.end();
  }

  // --- EXIBIÇÃO ESTÁTICA ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(itens[opcao]); 
  lcd.print(" T:");
  lcd.print(mediaTempAr, 1); 
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("UMID: ");
  lcd.print(umidFinalGrao, 1);
  lcd.print("% ");

  // TRAVA ATÉ APERTAR HOME
  while (digitalRead(BTN_HOME) == HIGH) {
    delay(10); 
  }

  lcd.clear();
  lcd.print("HOME...");
  delay(500);
  lcd.clear();
}
