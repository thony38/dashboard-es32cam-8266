//-----WIFI-----
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h> 


extern "C" {
  #include "user_interface.h"
}
// Taille écran OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Adresse I2C (souvent 0x3C pour OLED 0.96")
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK "thereisnospoon"
#endif

/* Identifiants du point d'accès */
const char *ssid = APSSID;
const char *password = APPSK;

// Définir une adresse IP fixe pour le point d'accès
IPAddress apIP(192, 168, 4, 2); // Vous pouvez choisir une adresse libre
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80);

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  // --- Initialisation du système de fichiers ---
  if (!LittleFS.begin()) {
    Serial.println("Erreur: LittleFS mount failed");
    return;
  }

  // --- Initialisation OLED ---
  Wire.begin(5, 4); // (SDA=D1=GPIO5, SCL=D2=GPIO4)

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // stop si OLED introuvable
  }
  display.clearDisplay();
  display.setRotation(2);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Appliquer la configuration
  WiFi.softAPConfig(apIP, apIP, subnet);

  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // --- NOUVEAU: Gérer les requêtes pour les fichiers ---
  server.on("/", HTTP_GET, [](){
    server.send(200, "text/html", LittleFS.open("/index.html", "r").readString());
  });

  server.on("/style.css", HTTP_GET, [](){
    server.send(200, "text/css", LittleFS.open("/style.css", "r").readString());
  });

  server.onNotFound([](){
    server.send(404, "text/plain", "Fichier non trouvé");
  });

  server.begin();
  Serial.println("HTTP server started");

  display.clearDisplay();
 
}

void loop() {
  server.handleClient();
  
  // Obtenir le nombre de clients connectés
  int connectedClients = WiFi.softAPgetStationNum();

  // --- Affichage OLED ---
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("ESP8266 WiFi AP");
  display.println("----------------");
  display.print("SSID:");
  display.println(ssid);
  display.print("PASS:");
  display.println(password);
  display.print("IP: ");
  display.println(WiFi.softAPIP());

  display.setCursor(120, 55);
  // Affiche le nombre de clients connectés
  display.println(connectedClients);


  display.display();
  delay(10); 
}
