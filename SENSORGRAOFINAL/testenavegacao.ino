#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pinos dos botões (Ajustados para ESP32)
const int BTN_AVANCAR   = 25;
const int BTN_SELECIONAR = 27;
const int BTN_HOME       = 26; 

// Variáveis do Menu
int opcao = 0;
const int totalOpcoes = 4;
String itens[] = {"CAFE", "SOJA", "MILHO", "PERSONALIZADO"};
bool confirmado = false;

void atualizarDisplay() {
  lcd.clear();
  
  if (!confirmado) {
    // TELA DE ESCOLHA
    lcd.setCursor(0, 0);
    lcd.print("Selecione:");
    lcd.setCursor(0, 1);
    lcd.print("> " + itens[opcao]);
  } else {
    // TELA DE CONFIRMAÇÃO (Aqui você depois colocará a leitura dos sensores)
    lcd.setCursor(0, 0);
    lcd.print(itens[opcao]);
    lcd.setCursor(0, 1);
    lcd.print("CALCULANDO...");
  }
}

void setup() {

  Wire.begin(32, 33); 
  lcd.init();
  lcd.backlight();

  pinMode(BTN_AVANCAR, INPUT_PULLUP);
  pinMode(BTN_SELECIONAR, INPUT_PULLUP);
  pinMode(BTN_HOME, INPUT_PULLUP);

  atualizarDisplay();
}

void loop() {
  // LÓGICA DO BOTÃO AVANÇAR
  if (digitalRead(BTN_AVANCAR) == LOW) {
    confirmado = false; 
    opcao++;
    if (opcao >= totalOpcoes) opcao = 0;
    
    atualizarDisplay();
    delay(250); 
    while(digitalRead(BTN_AVANCAR) == LOW); 
  }

  // LÓGICA DO BOTÃO SELECIONAR
  if (digitalRead(BTN_SELECIONAR) == LOW) {
    confirmado = true; 
    atualizarDisplay();
    delay(250);
    while(digitalRead(BTN_SELECIONAR) == LOW);
  }

  // LÓGICA DO BOTÃO HOME (VOLTAR)

  if (digitalRead(BTN_HOME) == LOW) {
    confirmado = false; 
    opcao = 0;          // <--- ADICIONE ESTA LINHA para resetar o menu
    
    atualizarDisplay();
    delay(250);
    while(digitalRead(BTN_HOME) == LOW); // Aguarda soltar o botão
  }
}