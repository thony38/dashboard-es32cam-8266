#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// ================== OLED CONFIG ==================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================== WIFI CONFIG ==================
const char* esp32_ssid = "ESP32-AP";
const char* esp32_password = "12345678";
const char* esp32_ip = "192.168.4.1";
const char* esp32_url = "http://192.168.4.1/info";

// ================== BOUTON CONFIG ==================
#define BUTTON_PIN D6 // Bouton 1 (Réinitialisation / Affichage des infos de connexion)
#define BUTTON2_PIN D5 // Bouton 2 (Allumer/Éteindre l'écran)

// Variables pour le débounce
const int debounceDelay = 50;
long lastDebounceTime1 = 0;
long lastDebounceTime2 = 0;
int buttonState1;
int lastButtonState1 = HIGH;
int buttonState2;
int lastButtonState2 = HIGH;

// Variable globale pour l'état de l'écran
bool displayIsOn = true;

// ================== VARIABLES GLOBALES ==================
DynamicJsonDocument doc(512); // "doc" is now a global variable, accessible by all functions.

// ================== PROTOTYPES ==================
void getEsp32Info();
void printWifiStatus(int status);
void resetEverything();
void toggleDisplay();
void displayConnectionInfo(JsonObject doc);

// ================== FONCTION UTILITAIRE ==================
void printWifiStatus(int status) {
  switch (status) {
    case WL_IDLE_STATUS:
      display.println();
      display.println("Initialisation...");
      break;
    case WL_NO_SSID_AVAIL:
      display.println();
      display.println("SSID non trouve");
      break;
    case WL_CONNECTED:
      display.println();
      display.println("Connecte !");
      break;
    case WL_CONNECT_FAILED:
      display.println();
      display.println("Echec de la connexion");
      break;
    case WL_CONNECTION_LOST:
      display.println();
      display.println("Connexion perdue");
      break;
    case WL_DISCONNECTED:
      display.println();
      display.println("Deconnecte");
      break;
    default:
      display.println("Code: " + String(status));
      break;
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  
  Wire.begin(5, 4);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Echec de l'allocation SSD1306"));
    for (;;);
  }
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Chargement...");
  display.display();

  Serial.print("Connexion a ");
  Serial.println(esp32_ssid);
  WiFi.begin(esp32_ssid, esp32_password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (displayIsOn) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Chargement...");
        display.print("Connexion WiFi: ");
        printWifiStatus(WiFi.status());
        display.display();
    }
  }
  
  Serial.println("\nWiFi connecte !");
  Serial.print("Adresse IP du Wemos : ");
  Serial.println(WiFi.localIP());
  
  if (displayIsOn) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("WiFi connecte!");
      display.print("IP Wemos: ");
      display.println(WiFi.localIP());
      display.display();
      delay(1000);
  }
  
  getEsp32Info();
}

// ================== LOOP ==================
void loop() {
  static unsigned long lastCheckTime = 0;

  // Lecture et débounce pour le bouton 1
  int reading1 = digitalRead(BUTTON_PIN);
  if (reading1 != lastButtonState1) {
    lastDebounceTime1 = millis();
  }
  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading1 != buttonState1) {
      buttonState1 = reading1;
      if (buttonState1 == LOW) {
        Serial.println("Bouton 1 pressé, réinitialisation/affichage des infos...");
        resetEverything();
      }
    }
  }
  lastButtonState1 = reading1;

  // Lecture et débounce pour le bouton 2
  int reading2 = digitalRead(BUTTON2_PIN);
  if (reading2 != lastButtonState2) {
    lastDebounceTime2 = millis();
  }
  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading2 != buttonState2) {
      buttonState2 = reading2;
      if (buttonState2 == LOW) {
        Serial.println("Bouton 2 pressé, bascule l'affichage...");
        toggleDisplay();
      }
    }
  }
  lastButtonState2 = reading2;

  // Met à jour les infos toutes les 5 secondes si l'écran est allumé
  if (displayIsOn && (millis() - lastCheckTime > 30000)) {
    getEsp32Info();
    lastCheckTime = millis();
  }
}


// ================== FONCTIONS SÉPARÉES ==================

void resetEverything() {
    getEsp32Info();
}

void toggleDisplay() {
    displayIsOn = !displayIsOn;
    if (displayIsOn) {
        // Logique pour allumer l'écran
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.println("Mise en route de");
        display.println("l'ecran...");
        display.display();
        display.ssd1306_command(SSD1306_DISPLAYON);
        delay(1000);
        
        // Appelle la fonction sans arguments car "doc" est globale
        displayConnectionInfo(); 
    } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.println("Mise en veille de");
        display.println("l'ecran...");
        display.display();
        delay(1000);
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
}

// ================== FONCTIONS AFFICHAGE INFO ==================

void displayConnectionInfo() { // La fonction ne prend plus d'argument
    if (displayIsOn) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Server pret !");
        display.print("IP: ");
        display.println(doc["ip"].as<String>());
        display.print("SSID: ");
        display.println(doc["ssid"].as<String>());
        display.print("MDP: ");
        display.println(doc["password"].as<String>());
        display.display();
    }
}

// ================== FONCTION PRINCIPALE DE REQUÊTE ==================

void getEsp32Info() {
  HTTPClient http;
  WiFiClient wifiClient;
  
  if (displayIsOn) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.println("Connexion...");
      display.display();
  }

  http.begin(wifiClient, esp32_url);

  int httpCode = http.GET();
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String jsonResponse = http.getString();
      
      DeserializationError error = deserializeJson(doc, jsonResponse);
      
      if (error) {
        if (displayIsOn) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Erreur JSON");
            display.println("Voir moniteur serie");
            display.display();
        }
      } else {
        if (displayIsOn) {
            // Premier affichage : statut de l'ESP32-CAM
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0, 0);
            
            display.println("Statut ESP32-CAM :");
            display.print("SD: ");
            display.println(doc["sd_status"].as<String>());
            display.print("Camera: ");
            display.println(doc["camera_status"].as<String>());
            display.println("Fichiers:");
            if (doc["files"].is<JsonObject>()) {
              JsonObject files = doc["files"];
              for (JsonPair p : files) {
                display.print("  ");
                display.print(p.key().c_str());
                display.print(": ");
                display.println(p.value().as<bool>() ? "OK" : "MANQUANT");
              }
            }
            display.display();
            delay(2500);
            // Deuxième affichage : "SERVER OK"
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(WHITE);
            display.setCursor(0, 20);
            display.println("SERVER OK");
            display.display();
            delay(1500);
            // Appelle la fonction dédiée sans arguments
            displayConnectionInfo();
        }
      }
    }
  } else {
    if (displayIsOn) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Echec de la connexion !");
        display.display();
    }
  }
  http.end();
}