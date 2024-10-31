/*
Auteur: Thom

Datum: 15 okt 2024 - nu
Klas: EV1C
Project: Game on

ESPcom v2

Leaphy communicatie module die gebruik maakt van een Wifi accesspoint voor communicatie en onderling via ESP-NOW.
*/


#include "Arduino.h"
#include <WiFi.h>
#include <esp_now.h>
#include "espcom_v2.h"


#define DEBUG true

#define SCANNER_CHANNEL_WAITTIME 2000 // 2000ms = 2 sec (24 sec total scan time for all 12 channels)
#define SCANNER_MAX_CHANNEL 12

//GLOBAL VARIABLES
void (*dataFunc)(communication_struct) = NULL;
bool isaHost = false;


//other callbacks
void (*endFunc)(exit_types) = NULL;
void (*joinFunc)() = NULL;
void (*unprocessable_frame)() = NULL;


//INTERNAL FUNCTIONS
template <size_t N, size_t M>
bool compareArrays_byte(const byte (&array1)[N], const byte (&array2)[M]) { //tested

    if (N != M) {
        return false; // Arrays are different if lengths don't match
    }

    // Step 2: Compare contents
    for (size_t i = 0; i < N; i++) {
        if (array1[i] != array2[i]) {
            return false; // Arrays are different if any byte doesn't match
        }
    }

    // Arrays are identical if lengths and contents are the same
    return true;
}
template <size_t N, size_t M>
bool compareArrays_uint8(const uint8_t (&array1)[N], const uint8_t (&array2)[M]) { //tested
    
    if (N != M) {
        return false; // Arrays are different if lengths don't match
    }

    // Step 2: Compare contents
    for (size_t i = 0; i < N; i++) {
        if (array1[i] != array2[i]) {
            return false; // Arrays are different if any byte doesn't match
        }
    }

    // Arrays are identical if lengths and contents are the same
    return true;
}


bool espcom_espnow_addpeer(uint8_t peerAddr[6], int channel) {
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = channel;
  peerInfo.encrypt = false;

  // Register peer
  memcpy(peerInfo.peer_addr, peerAddr, sizeof(peerAddr));
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    #if DEBUG
      log_e(F("Espcom - could not register espnow peer.."));
    #endif
    return false;
  }
  return true;
}

uint8_t[6] myMacAddress(){
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    return baseMac;
  } else {
    #if DEBUG
      log_e(F("Espcom - an error occured while fetching the board's mac address."));
    #endif
    return {0,0,0,0,0,0};
  }
}



// ESP DATA CALLBACK register
void espcom_ondata_cb(void (*cb)(communication_struct)){
  dataFunc = cb;
}



//SETTINGS
void espcom_set_mode(bool Host){ // Set mode
  isaHost = Host;
}


// SETUP
bool espcom_init_wifi(){
  #if DEBUG
    Serial.begin(9600);
    while(!Serial){}
    log_d(F("ESPCOM debug program started.."));
    log_d(F("Starting espcom as host: "));
    log_d(isaHost);
  #endif

  WiFi.mode(WIFI_AP_STA);

  if (isaHost){
    WiFi.softAP(F("ESP_GAME_HOST"), F("tikkertje"));
  }

  return true;
}

bool espcom_init_espnow(){
  if (esp_now_init() != ESP_OK) {
    #if DEBUG
      log_e(F("Espcom - Critical error initializing ESP-NOW.."));
    #endif

    return false;
  }

  return true;
}

//AUTOMATIC LOGIC

int scanner_currentchannel = 1;
bool scanner_enabled = false;
long scanner_millis = 0;
void (*scanner_cb)(game_struct) = NULL;


void espcom_auto_findgame(void (*cb)(game_struct)){ // scanner
  game_struct game;

  if (isaHost){
    #if DEBUG
      log_e(F("Espcom - A host cannot scan for games."));
    #endif
    return;
  }
  //WiFi.softAPdisconnect(true); // disconnect wifi ap for scanning

  scanner_currentchannel = 1;
  scanner_cb = cb;
  scanner_enabled = true;
}


void espcom_auto_joingame(game_struct);  //TODO



void espcom_tick(){ // non-blocking processor       TODO: stop scanner if beacon frame is received

  if (scanner_enabled && millis() - scanner_millis >= SCANNER_CHANNEL_WAITTIME){
    
     // Set the new wifi channel
    WiFi.begin();
    WiFi.setChannel(scanner_currentchannel);

    // Reinitialize espnow on the new channel
    esp_now_deinit();  // Deinitialize espnow

    if (esp_now_init() != ESP_OK) {
      //TODO: CRITICAL ERROR
    }

    if (scanner_currentchannel < SCANNER_MAX_CHANNEL){
      scanner_currentchannel++;
    } else {
      // all channels scanned
      scanner_enabled = false;
    }
  }

}



//ESPNOW FUNCS

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) { //receive data
  default_struct struct_1;
  memcpy(&struct_1, incomingData, sizeof(struct_1));


  if (!compareArrays_byte(struct_1.protocol_id, protocol_id)){ // Ignore messages that aren't using our protocol..
    #if DEBUG
      log_d(F("Espcom - received unprocessable espnow message."))
    #endif
    return;
  }

  if (struct_1.frame_type == (NULL || 0)){
    #if DEBUG
      log_d(F("Espcom - received malformed frame."))
    #endif
    return;
  }



}
