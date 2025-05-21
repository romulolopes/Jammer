#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include "RF24.h"

// Wi-Fi Configurações
const char* ssid = "JammerAP";
const char* password = "12345678";
const char* webPassword = "123456"; // senha da interface web

RF24 radio(2, 4); // CE, CSN
byte i = 45;
ESP8266WebServer server(80);

const int wifiFrequencies[] = {
  2412, 2417, 2422, 2427, 2432,
  2437, 2442, 2447, 2452, 2457, 2462
};

const char* modes[] = {
  "BLE & All 2.4 GHz",
  "Just Wi-Fi",
  "Waiting Idly :("
};
uint8_t attack_type = 2;
bool loggedIn = false;

// Página HTML de login
String loginPage = R"rawliteral(
  <html><body>
    <h2>Login</h2>
    <form action="/login" method="POST">
      Senha: <input type="password" name="password">
      <input type="submit" value="Entrar">
    </form>
  </body></html>
)rawliteral";

// Página principal com botões de seleção
String controlPage() {
  String page = "<html><body><h2>Selecionar Modo</h2>";
  page += "<p>Modo atual: <b>" + String(modes[attack_type]) + "</b></p>";
  for (int i = 0; i < 3; i++) {
    page += "<form action='/set' method='POST'>";
    page += "<input type='hidden' name='mode' value='" + String(i) + "'>";
    page += "<input type='submit' value='" + String(modes[i]) + "'>";
    page += "</form><br/>";
  }
  page += "</body></html>";
  return page;
}

void handleRoot() {
  if (!loggedIn) {
    server.send(200, "text/html", loginPage);
  } else {
    server.send(200, "text/html", controlPage());
  }
}

void handleLogin() {
  if (server.method() == HTTP_POST) {
    if (server.arg("password") == webPassword) {
      loggedIn = true;
      server.sendHeader("Location", "/");
      server.send(303);
    } else {
      server.send(200, "text/html", "<p>Senha incorreta!</p>" + loginPage);
    }
  }
}

void handleSetMode() {
  if (!loggedIn) {
    server.send(403, "text/plain", "Acesso negado");
    return;
  }
  if (server.hasArg("mode")) {
    int mode = server.arg("mode").toInt();
    if (mode >= 0 && mode <= 2) {
      attack_type = mode;
      Serial.println("Modo alterado para: " + String(modes[attack_type]));
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi iniciado");
  Serial.println("IP: " + WiFi.softAPIP().toString());
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/set", HTTP_POST, handleSetMode);
  server.begin();
  Serial.println("Servidor Web iniciado");
}

void fullAttack() {
  for (size_t i = 0; i < 80; i++) {
    radio.setChannel(i);
  }
}

void wifiAttack() {
  for (int i = 0; i < sizeof(wifiFrequencies) / sizeof(wifiFrequencies[0]); i++) {
    radio.setChannel(wifiFrequencies[i] - 2400);
  }
}

void setup() {
  Serial.begin(9600);
  setupWiFi();
  setupWebServer();

  if (radio.begin()) {
    delay(200);
    radio.setAutoAck(false); 
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPayloadSize(5);
    radio.setAddressWidth(3);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, i);
  } else {
    Serial.println("BLE Jammer não pôde ser iniciado!");
  }
}

void loop() {
  server.handleClient();

  switch (attack_type) {
    case 0:
      fullAttack();
      break;
    case 1:
      wifiAttack();
      break;
    case 2:
      // Idle
      break;
  }
}
