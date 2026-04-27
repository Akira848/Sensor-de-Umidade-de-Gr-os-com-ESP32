#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BluetoothSerial.h>

// --- CONFIGURAÇÕES DOS SENSORES ---
#define DHTPIN 18
#define DHTTYPE DHT22
#define SENSOR_GRAO 34
DHT dht(DHTPIN, DHTTYPE);

// --- CONFIGURAÇÃO BLUETOOTH ---
BluetoothSerial SerialBT;

// --- CONFIGURAÇÃO LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- PINOS DOS BOTÕES ---
const int BTN_AVANCAR    = 25;
const int BTN_HOME       = 26; 
const int BTN_SELECIONAR = 27;

// --- VARIÁVEIS DE CONTROLE ---
int opcao = 0;
const int totalOpcoes = 4;
String itens[] = {"CAFE", "SOJA", "MILHO", "PERSONALIZADO"};
bool confirmado = false;

void setup() {
  // Inicializa Comunicações
  Wire.begin(32, 33);
  SerialBT.begin("Medidor_Graos_AnaClara"); // Nome do Bluetooth
  lcd.init();
  lcd.backlight();
  dht.begin();

  // Configura Botões
  pinMode(BTN_AVANCAR, INPUT_PULLUP);
  pinMode(BTN_SELECIONAR, INPUT_PULLUP);
  pinMode(BTN_HOME, INPUT_PULLUP);

  lcd.setCursor(0, 0);
  lcd.print("SISTEMA ONLINE");
  lcd.setCursor(0, 1);
  lcd.print("BLUETOOTH ATIVO");
  delay(2000);
  lcd.clear();
}

void loop() {
  // --- LÓGICA DE NAVEGAÇÃO (BOTÕES) ---
  if (digitalRead(BTN_AVANCAR) == LOW) {
    confirmado = false; 
    opcao = (opcao + 1) % totalOpcoes;
    delay(250); 
    while(digitalRead(BTN_AVANCAR) == LOW); 
  }

  if (digitalRead(BTN_SELECIONAR) == LOW) {
    confirmado = true; 
    lcd.clear();
    delay(250);
  }

  if (digitalRead(BTN_HOME) == LOW) {
    confirmado = false; 
    lcd.clear();
    delay(250);
  }

  // --- INTERFACE E PROCESSAMENTO ---
  if (!confirmado) {
    // TELA DE MENU
    lcd.setCursor(0, 0);
    lcd.print("Selecionar:");
    lcd.setCursor(0, 1);
    lcd.print("> " + itens[opcao] + "           ");
  } 
  else {
    // MODO LEITURA E TRANSMISSÃO
    float umidAr = dht.readHumidity();
    int leituraGrao = analogRead(SENSOR_GRAO);
    
    // Calibração: 3200 (seco) a 1400 (úmido) - ajuste conforme teste
    int umidPorcent = map(leituraGrao, 3200, 1400, 0, 100);
    umidPorcent = constrain(umidPorcent, 0, 100);

    // Mostra no LCD
    lcd.setCursor(0, 0);
    lcd.print(itens[opcao] + " Lendo...");
    lcd.setCursor(0, 1);
    lcd.print("G:"); lcd.print(umidPorcent); lcd.print("%  ");
    lcd.print("Ar:"); lcd.print((int)umidAr); lcd.print("% ");

    // TRANSMISSÃO VIA BLUETOOTH
    // Formato CSV (fácil de copiar para Excel/Análise posterior)
    SerialBT.print(itens[opcao]);
    SerialBT.print(",");
    SerialBT.print(umidPorcent);
    SerialBT.print(",");
    SerialBT.println(umidAr);

    delay(500); // Envia dados a cada meio segundo
  }
}