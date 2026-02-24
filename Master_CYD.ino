/*
  Basato sul codice di Rui Santos & Sara Santos - Random Nerd Tutorials
  Modificato per Progetto ENES HOME - Master CYD con Touchscreen
*/

#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

// MAC Address BROADCAST (Invia a tutti i dispositivi in ascolto)
uint8_t broadcastAddress[] = {0x20, 0xE7, 0xC8, 0x67, 0x3E, 0xB4};

// --- PIN DEL CYD ---
#define XPT2046_IRQ 36
#define XPT2046_MOSI 13
#define XPT2046_MISO 12
#define XPT2046_CLK 14
#define XPT2046_CS 33
#define CYD_BACKLIGHT 21 

// --- PIN LED RGB INTEGRATO SUL CYD ---
#define CYD_LED_RED   4
#define CYD_LED_GREEN 17 
#define CYD_LED_BLUE  16 

// Oggetti per lo schermo
SPIClass mySpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// Struttura dati (DEVE coincidere esattamente con quella dello Slave)
typedef struct struct_message {
  bool red;
  bool green;
  bool blue;
} struct_message;

// Crea una variabile chiamata ledData per contenere i dati da inviare
struct_message ledData;
esp_now_peer_info_t peerInfo;
unsigned long lastTouch = 0; 

// ==========================================
// Callback: funzione eseguita in automatico quando i dati vengono SPEDITI
// ==========================================
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Stato invio pacchetto: \t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "CONSEGNA OK (Success)" : "CONSEGNA FALLITA (Fail)");
}

// Funzione per disegnare i bottoni sullo schermo
void drawButton(int x, int y, int w, int h, String label, bool state, uint16_t color) {
  uint16_t fillColor = state ? color : TFT_DARKGREY; 
  tft.fillRoundRect(x, y, w, h, 10, fillColor);
  tft.drawRoundRect(x, y, w, h, 10, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(label, x + (w/2), y + (h/2) - 10, 2);
}

// Aggiorna Schermo, LED Locale e INVIA I DATI
void updateSystem() {
  // 1. Aggiorna i bottoni sullo schermo
  drawButton(20, 60, 80, 100, "ROSSO", ledData.red, TFT_RED);
  drawButton(120, 60, 80, 100, "VERDE", ledData.green, TFT_GREEN);
  drawButton(220, 60, 80, 100, "BLU", ledData.blue, TFT_BLUE);

  // 2. Accendi/Spegni il LED RGB dietro al CYD (Funzionano al contrario: LOW = acceso)
  digitalWrite(CYD_LED_RED,   ledData.red ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, ledData.green ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE,  ledData.blue ? LOW : HIGH);

  // 3. Spedisci il pacchetto via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &ledData, sizeof(ledData));
   
  if (result == ESP_OK) {
    Serial.println("Invio partito correttamente...");
  } else {
    Serial.println("Errore di trasmissione radio");
  }
}

void setup() {
  // Inizializza Monitor Seriale
  Serial.begin(115200);

  // Inizializza LED CYD
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  
  // Imposta tutto spento all'inizio
  ledData.red = false; ledData.green = false; ledData.blue = false;
  digitalWrite(CYD_LED_RED, HIGH); digitalWrite(CYD_LED_GREEN, HIGH); digitalWrite(CYD_LED_BLUE, HIGH);

  // Inizializza Schermo e Touch
  pinMode(CYD_BACKLIGHT, OUTPUT); digitalWrite(CYD_BACKLIGHT, HIGH);
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi); ts.setRotation(1);
  tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("CONTROLLO DOMOTICO", 160, 15, 4);

  // Imposta Wi-Fi e disconnettilo per resettare il canale
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Inizializza ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore inizializzazione ESP-NOW");
    tft.drawCentreString("Errore ESP-NOW", 160, 200, 2);
    return;
  }

  // Registra la funzione di Callback per sapere se l'invio è andato a buon fine
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  
  // Registra il dispositivo di destinazione (Peer)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Errore durante l'aggiunta del Peer");
    return;
  }

  // Disegna l'interfaccia per la prima volta
  updateSystem();
}

void loop() {
  // Controlla se lo schermo viene toccato (con anti-rimbalzo di 300ms)
  if (ts.touched() && (millis() - lastTouch > 300)) {
    lastTouch = millis();
    TS_Point p = ts.getPoint();
    
    // Mappatura delle coordinate touch
    int tx = map(p.y, 550, 3600, 0, 320);
    int ty = map(p.x, 400, 3800, 0, 240);
    
    // Assicurati che non escano dallo schermo
    if(tx<0)tx=0; if(tx>320)tx=320; if(ty<0)ty=0; if(ty>240)ty=240;

    // Se tocchi nella riga dei pulsanti
    if (ty > 60 && ty < 160) {
      if (tx > 20 && tx < 100)  ledData.red = !ledData.red;       // Inverti rosso
      if (tx > 120 && tx < 200) ledData.green = !ledData.green;   // Inverti verde
      if (tx > 220 && tx < 300) ledData.blue = !ledData.blue;     // Inverti blu
      
      // Aggiorna grafica e invia dati
      updateSystem();
    }
  }
}