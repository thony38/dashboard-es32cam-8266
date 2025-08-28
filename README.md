README pour le projet "Dashboard ESP8266 + ESP32"
📋 Description du projet
Ce projet est un tableau de bord sans fil conçu pour l'IoT. Il utilise un ESP8266 pour afficher sur un écran OLED des informations cruciales sur un autre appareil, un ESP32-CAM.

L'objectif principal est de fournir un accès rapide et visuel au statut d'un appareil distant sans avoir à passer par un ordinateur ou une interface web.

✨ Fonctionnalités
Interface intuitive : Un écran OLED affiche l'état de la carte SD, de la caméra et les informations de connexion (IP, SSID).

Gestion de l'affichage : Un bouton permet d'allumer et d'éteindre l'écran OLED pour économiser de l'énergie.

Rafraîchissement des données : Un second bouton permet de mettre à jour manuellement les informations affichées, en cas de besoin.

Connectivité Wi-Fi : Le tableau de bord se connecte à un point d'accès Wi-Fi créé par l'ESP32 pour récupérer les données.

🛠️ Matériel requis
ESP8266 (Wemos D1 mini ou équivalent)

Écran OLED (128x64 pixels, I2C)

Deux boutons poussoirs

ESP32-CAM (qui agit comme serveur de données)

Câbles de connexion

💻 Installation et utilisation
Préparation de l'ESP32-CAM (Serveur) :

Flashez le firmware sur votre ESP32-CAM pour qu'il crée un point d'accès Wi-Fi et serve une page web ou une API avec l'état de la caméra et de la carte SD.

Assurez-vous que l'API renvoie les informations au format JSON, incluant l'état de la SD, de la caméra, les fichiers, l'IP et le SSID.

Préparation de l'ESP8266 (Client) :

Connectez l'écran OLED et les boutons à votre ESP8266 selon le schéma de câblage.

Téléchargez et ouvrez le code oled_wemos_st.ino dans l'IDE Arduino.

Assurez-vous d'avoir les librairies nécessaires : Adafruit_GFX, Adafruit_SSD1306, ArduinoJson, ESP8266WiFi et ESP8266HTTPClient.

Dans le code, configurez les informations du réseau Wi-Fi de l'ESP32.

Téléchargez le code sur l'ESP8266.

Utilisation :

Bouton 1 : Appuyez pour rafraîchir les informations de la caméra.

Bouton 2 : Appuyez pour allumer ou éteindre l'écran.

🐛 Dépannage
Écran noir au démarrage : Vérifiez les connexions I2C de l'écran (SDA, SCL) et l'alimentation.

Erreur de compilation : Assurez-vous d'avoir toutes les librairies requises et que leur version est compatible.

"Echec de la connexion !" : L'ESP8266 ne parvient pas à se connecter à l'ESP32. Vérifiez que les informations Wi-Fi sont correctes et que l'ESP32 est bien allumé.
