#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// =====================================================
//  CONFIGURAÇÕES — edite apenas esta seção
// =====================================================
const char* WIFI_SSID     = "iPhone de Rafael";
const char* WIFI_SENHA    = "julia123";
const char* API_URL       = "http://187.108.118.60:9083/Medidor/api/api.php";

const long  GMT_OFFSET_SEC  = -3 * 3600;  // Brasil UTC-3
const int   DAYLIGHT_OFFSET = 0;

const float PESO_SENSOR   = 0.7;
const float PESO_AR       = 0.3;

const int   CAL_SECO      = 3200;
const int   CAL_MOLHADO   = 1400;

const int   HOME_LONGO_MS = 3000;  // ms para considerar pressionamento longo
const int   FILA_MAX      = 30;    // máximo de medições salvas offline
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN      18
#define DHTTYPE     DHT22
#define SENSOR_GRAO 34

DHT dht(DHTPIN, DHTTYPE);

const int BTN_AVANCAR    = 21;
const int BTN_SELECIONAR = 22;
const int BTN_HOME       = 23;

int opcao = 0;
const int totalOpcoes = 4;
String itens[]        = {"Cafe", "Soja", "Milho", "Personalizado"};
String itensDisplay[] = {"CAFE", "SOJA", "MILHO", "PERSON."};

// =====================================================
//  FILA OFFLINE
// =====================================================
struct Medicao {
  String grao;
  float  temperatura;
  float  umidade;
  String dhrealizado;
};

Medicao filaPendente[FILA_MAX];
int filaTamanho = 0;

void adicionarNaFila(String grao, float temp, float umid, String dh) {
  if (filaTamanho >= FILA_MAX) {
    for (int i = 0; i < FILA_MAX - 1; i++) {
      filaPendente[i] = filaPendente[i + 1];
    }
    filaTamanho = FILA_MAX - 1;
    Serial.println("Fila cheia — registro mais antigo descartado.");
  }
  filaPendente[filaTamanho++] = {grao, temp, umid, dh};
  Serial.println("Salvo na fila. Pendentes: " + String(filaTamanho));
}

void enviarFilaPendente() {
  if (filaTamanho == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NADA PENDENTE");
    delay(1500);
    lcd.clear();
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENVIANDO FILA...");

  int enviados = 0;
  int falhas   = 0;
  int i        = 0;

  while (i < filaTamanho) {
    lcd.setCursor(0, 1);
    lcd.print(String(i + 1) + "/" + String(filaTamanho) + " ENV...   ");

    bool ok = enviarParaAPI(
      filaPendente[i].grao,
      filaPendente[i].temperatura,
      filaPendente[i].umidade,
      filaPendente[i].dhrealizado
    );

    if (ok) {
      for (int j = i; j < filaTamanho - 1; j++) {
        filaPendente[j] = filaPendente[j + 1];
      }
      filaTamanho--;
      enviados++;
    } else {
      falhas++;
      i++;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("OK:" + String(enviados) + " FALHA:" + String(falhas));
  lcd.setCursor(0, 1);
  lcd.print("Pendentes: " + String(filaTamanho));
  Serial.println("Fila processada — enviados: " + String(enviados) + ", falhas: " + String(falhas));
  delay(2500);
  lcd.clear();
}

// =====================================================
//  LEITURA DO BOTÃO HOME
//  Retorna: 0 = nada, 1 = toque curto, 2 = pressionamento longo
// =====================================================
int lerBotaoHome() {
  if (digitalRead(BTN_HOME) == HIGH) return 0;

  unsigned long inicio = millis();
  int progresso = 0;

  lcd.setCursor(0, 1);
  lcd.print("                ");

  while (digitalRead(BTN_HOME) == LOW) {
    unsigned long segurando = millis() - inicio;

    int novoProg = map(segurando, 0, HOME_LONGO_MS, 0, 16);
    if (novoProg != progresso) {
      progresso = novoProg;
      lcd.setCursor(0, 1);
      for (int i = 0; i < 16; i++) {
        lcd.print(i < progresso ? "=" : " ");
      }
    }

    if (segurando >= HOME_LONGO_MS) {
      while (digitalRead(BTN_HOME) == LOW) delay(10);
      return 2;
    }
    delay(20);
  }

  return 1;
}

// =====================================================
//  SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  Wire.begin(32, 33);
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(BTN_AVANCAR,    INPUT_PULLUP);
  pinMode(BTN_SELECIONAR, INPUT_PULLUP);
  pinMode(BTN_HOME,       INPUT_PULLUP);

  lcd.setCursor(0, 0);
  lcd.print("SISTEMA PRONTO");
  delay(1200);
  lcd.clear();

  conectarWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, "pool.ntp.org", "time.nist.gov");
    lcd.clear();
    lcd.print("SINCRONIZANDO");
    lcd.setCursor(0, 1);
    lcd.print("RELOGIO NTP...");
    delay(2000);
    lcd.clear();
    Serial.println("NTP sincronizado.");
  }
}

// =====================================================
//  LOOP PRINCIPAL
// =====================================================
void loop() {
  lcd.setCursor(0, 0);
  if (filaTamanho > 0) {
    lcd.print("GRAO [" + String(filaTamanho) + " PEND.]  ");
  } else {
    lcd.print("SELECIONAR GRAO:");
  }

  lcd.setCursor(0, 1);
  lcd.print("> " + itensDisplay[opcao] + "          ");

  if (digitalRead(BTN_AVANCAR) == LOW) {
    opcao = (opcao + 1) % totalOpcoes;
    delay(250);
    while (digitalRead(BTN_AVANCAR) == LOW);
  }

  if (digitalRead(BTN_SELECIONAR) == LOW) {
    delay(250);
    while (digitalRead(BTN_SELECIONAR) == LOW);
    calcularUmidadeFinal();
  }

  // HOME longo na tela inicial → reconecta e envia fila pendente
  if (digitalRead(BTN_HOME) == LOW) {
    int btn = lerBotaoHome();
    if (btn == 2) {
      reconectarEEnviarFila();
    }
  }
}

// =====================================================
//  MEDIÇÃO
// =====================================================
void calcularUmidadeFinal() {
  float somaUmidAr     = 0;
  float somaTempAr     = 0;
  long  somaCapacitivo = 0;
  int   leituras       = 0;

  lcd.clear();

  for (int i = 1; i <= 3; i++) {
    lcd.setCursor(0, 0);
    lcd.print("MEDINDO...  " + String(i) + "/3");
    delay(800);

    float umid = dht.readHumidity();
    float temp = dht.readTemperature();
    int   cap  = analogRead(SENSOR_GRAO);

    if (isnan(umid) || isnan(temp)) {
      lcd.setCursor(0, 1);
      lcd.print("ERRO SENSOR AR! ");
      delay(1000);
      continue;
    }

    somaUmidAr     += umid;
    somaTempAr     += temp;
    somaCapacitivo += cap;
    leituras++;
  }

  if (leituras == 0) {
    lcd.clear();
    lcd.print("FALHA NA LEITURA");
    delay(2500);
    lcd.clear();
    return;
  }

  float mediaUmidAr     = somaUmidAr     / leituras;
  float mediaTempAr     = somaTempAr     / leituras;
  int   mediaCapacitivo = somaCapacitivo / leituras;

  float umidSensor = map(mediaCapacitivo, CAL_SECO, CAL_MOLHADO, 0, 100);
  umidSensor = constrain(umidSensor, 0, 100);

  float umidFinal = (umidSensor * PESO_SENSOR) + (mediaUmidAr * PESO_AR);
  umidFinal = constrain(umidFinal, 0, 100);

  String dh = obterDataHora();

  exibirResultado(mediaTempAr, umidFinal);

  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 1);
    lcd.print("ENVIANDO...     ");
    bool ok = enviarParaAPI(itens[opcao], mediaTempAr, umidFinal, dh);
    if (ok) {
      lcd.print("ENVIADO OK!     ");
    } else {
      adicionarNaFila(itens[opcao], mediaTempAr, umidFinal, dh);
      lcd.print("SALVO PENDENTE! ");
    }
    delay(1500);
  } else {
    adicionarNaFila(itens[opcao], mediaTempAr, umidFinal, dh);
    lcd.setCursor(0, 1);
    lcd.print("SALVO PENDENTE! ");
    delay(1500);
  }

  // Aguarda HOME — curto = volta, longo = reconecta + envia fila
  exibirResultado(mediaTempAr, umidFinal);

  while (true) {
    int btn = lerBotaoHome();

    if (btn == 1) {
      lcd.clear();
      lcd.print("VOLTANDO...     ");
      delay(400);
      lcd.clear();
      return;
    }

    if (btn == 2) {
      reconectarEEnviarFila();
      exibirResultado(mediaTempAr, umidFinal);
    }

    delay(10);
  }
}

// =====================================================
//  RECONECTA WI-FI + ENVIA FILA PENDENTE
// =====================================================
void reconectarEEnviarFila() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RECONECTANDO... ");

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    delay(300);
    WiFi.begin(WIFI_SSID, WIFI_SENHA);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
      delay(500);
      lcd.setCursor(tentativas % 16, 1);
      lcd.print(".");
      tentativas++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("WI-FI OK!");
    Serial.println("Reconectado. IP: " + WiFi.localIP().toString());
    delay(1000);

    // Aguarda NTP sincronizar de verdade (até 5s)
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, "pool.ntp.org", "time.nist.gov");
    lcd.setCursor(0, 1);
    lcd.print("SINCRON. NTP... ");
    struct tm timeinfo;
    int tentNTP = 0;
    while (!getLocalTime(&timeinfo) && tentNTP < 10) {
      delay(500);
      tentNTP++;
    }

    // Corrige horário dos registros salvos offline com data inválida (1970)
    if (getLocalTime(&timeinfo)) {
      char buf[20];
      for (int i = 0; i < filaTamanho; i++) {
        if (filaPendente[i].dhrealizado.startsWith("1970")) {
          strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
          filaPendente[i].dhrealizado = String(buf);
          Serial.println("Horario corrigido para registro " + String(i) + ": " + filaPendente[i].dhrealizado);
        }
      }
    }

    enviarFilaPendente();
  } else {
    lcd.clear();
    lcd.print("SEM WI-FI");
    lcd.setCursor(0, 1);
    lcd.print("Tente novamente ");
    Serial.println("Reconexao falhou.");
    delay(2000);
    lcd.clear();
  }
}

// =====================================================
//  EXIBIÇÃO NO LCD
// =====================================================
void exibirResultado(float temp, float umid) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(itensDisplay[opcao]);
  lcd.print(" T:");
  lcd.print(temp, 1);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("UMID: ");
  lcd.print(umid, 1);
  lcd.print("%");
}

// =====================================================
//  DATA/HORA via NTP
// =====================================================
String obterDataHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter horario NTP");
    return "1970-01-01 00:00:00";
  }
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

// =====================================================
//  ENVIO PARA A API
// =====================================================
bool enviarParaAPI(String grao, float temp, float umid, String dh) {
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(8000);

  StaticJsonDocument<200> doc;
  doc["grao"]        = grao;
  doc["umidade"]     = round(umid * 10.0) / 10.0;
  doc["temperatura"] = round(temp * 10.0) / 10.0;
  doc["dhrealizado"] = dh;

  String payload;
  serializeJson(doc, payload);

  Serial.println("POST -> " + String(API_URL));
  Serial.println("Body: " + payload);

  int httpCode = http.POST(payload);

  Serial.println("Resposta HTTP: " + String(httpCode));
  if (httpCode > 0) Serial.println(http.getString());
  else Serial.println("Erro: " + http.errorToString(httpCode));

  http.end();
  return (httpCode >= 200 && httpCode < 300);
}

// =====================================================
//  WI-FI (conexão inicial)
// =====================================================
void conectarWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONECTANDO WI-FI");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(300);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    lcd.setCursor(tentativas % 16, 1);
    lcd.print(".");
    tentativas++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WI-FI OK!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    Serial.println("Wi-Fi conectado: " + WiFi.localIP().toString());
  } else {
    lcd.print("SEM WI-FI");
    lcd.setCursor(0, 1);
    lcd.print("Modo offline OK ");
    Serial.println("Wi-Fi falhou - modo offline ativado");
  }
  delay(1800);
  lcd.clear();
}
