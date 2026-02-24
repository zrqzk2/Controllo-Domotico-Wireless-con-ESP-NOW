/* ==========================================
   SLAVE (ESP32) - RICEVITORE LED
   Riceve comandi via ESP-NOW e accende i LED fisici
   ========================================== */

#include <esp_now.h>
#include <WiFi.h>

// Pin dei LED fisici sulla breadboard
#define LED_ROSSO  13
#define LED_VERDE  12
#define LED_BLU    14

// Struttura (DEVE essere identica a quella del Master)
typedef struct struct_message {
  bool red;
  bool green;
  bool blue;
} struct_message;

struct_message ledData;

// Funzione che scatta in automatico quando arriva un messaggio
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Copia i dati ricevuti
  memcpy(&ledData, incomingData, sizeof(ledData));
  
  // Stampa sul Monitor Seriale per verifica
  Serial.println("--- COMANDO RICEVUTO ---");
  Serial.print("ROSSO: "); Serial.println(ledData.red ? "ON" : "OFF");
  Serial.print("VERDE: "); Serial.println(ledData.green ? "ON" : "OFF");
  Serial.print("BLU:   "); Serial.println(ledData.blue ? "ON" : "OFF");
  
  // Accendi o spegni i LED (Logica standard: HIGH = acceso)
  digitalWrite(LED_ROSSO, ledData.red ? HIGH : LOW);
  digitalWrite(LED_VERDE, ledData.green ? HIGH : LOW);
  digitalWrite(LED_BLU,   ledData.blue ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);

  // Setup Pin LED
  pinMode(LED_ROSSO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_BLU, OUTPUT);

  // Partono tutti spenti
  digitalWrite(LED_ROSSO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_BLU, LOW);

  // Avvio WiFi e reset configurazioni precedenti
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // <- FONDAMENTALE PER EVITARE PROBLEMI DI CANALE

  // Inizializza ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore Inizializzazione ESP-NOW");
    return;
  }

  // Registra la funzione di ascolto
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("SLAVE PRONTO! In attesa di comandi...");
}

void loop() {
  // Il loop è vuoto, lavora tutto con la funzione OnDataRecv
  delay(100); 
}