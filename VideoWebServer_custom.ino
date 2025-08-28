/*
 Made by Thony38
*/
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SD_MMC.h"
#include <DHT.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
// Select camera model
#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#include "camera_pins.h"

// ================== DHT ==================
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================== LEDs ==================
#define LED_ROUGE_PIN       33
#define LED_VERTE_PIN       32
#define LED_BLEUE_PIN       12

// ================== AP CONFIG ==================
const char* ssid     = "ESP32-AP";
const char* password = "12345678";

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

WebServer server(80); // Port du serveur web

// ================== Serve SD ==================
String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".json")) {
    return "application/json";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  }
  return "text/plain";
}

void handleFileRead(String path) {
  if (path.endsWith("/")) {
    path = "/index.html";
  }
  
  if (SD_MMC.exists(path)) {
    File file = SD_MMC.open(path, FILE_READ);
    if (file) {
      server.streamFile(file, getContentType(path));
      file.close();
    } else {
      server.send(500, "text/plain", "Erreur de lecture du fichier");
    }
  } else {
    server.send(404, "text/plain", "Fichier non trouvé");
  }
}

// ================== Info debug wemos Handler ==================
void handleInfo() {
  DynamicJsonDocument doc(512); // Taille augmentée pour plus d'infos

  // Informations de connexion
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["ip"] = WiFi.softAPIP().toString();

  // Statut de la carte SD
  bool sdOK = SD_MMC.begin("/sdcard", true);
  doc["sd_status"] = sdOK ? "OK" : "Erreur";
  if (!sdOK) {
    doc["sd_error_msg"] = "Initialisation de la carte SD échouée.";
  }

  // Statut de la caméra
  doc["camera_status"] = "OK"; // La caméra est initialisée au démarrage, donc son statut est supposé OK.

  // Vérification des fichiers sur la carte SD
  doc["files"]["index.html"] = SD_MMC.exists("/index.html");
  doc["files"]["style.css"] = SD_MMC.exists("/style.css");
  doc["files"]["script.js"] = SD_MMC.exists("/script.js");

  // Sérialisation du document JSON
  String jsonOutput;
  serializeJson(doc, jsonOutput);

  server.send(200, "application/json", jsonOutput);
}

// ================== DHT Handler ==================
void handleDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  Serial.printf("Données DHT reçues : Température = %.1f, Humidité = %.1f\n", t, h);
  if (isnan(h) || isnan(t)) {
    server.send(500, "application/json", "{\"error\":\"DHT lecture echouee\"}");
    return;
  }

  String json = "{";
  json += "\"temperature\":" + String(t,1) + ",";
  json += "\"humidity\":" + String(h,1);
  json += "}";

  server.send(200, "application/json", json);
}

void startCameraServer();

// ================== SETUP & LOOP ==================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

// --- Initialisation des LEDs ---
  pinMode(LED_ROUGE_PIN, OUTPUT);
  pinMode(LED_VERTE_PIN, OUTPUT);
  pinMode(LED_BLEUE_PIN, OUTPUT);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_VGA);
  
  // --- DHT ---
  dht.begin();
  Serial.println("Capteur DHT démarré");

  // --- SD ---
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Erreur SD");
  } else {
    Serial.println("Carte SD OK");
  }

  // Gestionnaire pour le contrôle des LEDs simples
  server.on("/led", HTTP_GET, [](){
    // Récupère les valeurs de rouge, vert et bleu
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();

    // Allume ou éteint chaque LED en fonction de la valeur
    // (si la valeur est > 0, on allume, sinon on éteint)
    digitalWrite(LED_ROUGE_PIN, r > 0 ? HIGH : LOW);
    digitalWrite(LED_VERTE_PIN, g > 0 ? HIGH : LOW);
    digitalWrite(LED_BLEUE_PIN, b > 0 ? HIGH : LOW);

    server.send(200, "text/plain", "OK");
  });
  // Gestionnaire pour récuperer les info
  server.on("/info", handleInfo);
  // --- WiFi AP ---
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.println("AP démarré. IP: " + WiFi.softAPIP().toString());

  // --- Serveur ---
  server.on("/", [](){ handleFileRead("/index.html"); });
  server.on("/dht", handleDHT);
  
  // Gère toutes les autres requêtes de fichiers (css, js, etc.)
  server.onNotFound([]() {
    handleFileRead(server.uri());
  });

  delay(2000);
  server.begin();
  Serial.println("Serveur HTTP prêt sur le port 80");

  // AJOUTER CETTE LIGNE
  startCameraServer();
  Serial.println("Serveur de la caméra démarré sur le port 81");
}

void loop() {
  server.handleClient();
}
