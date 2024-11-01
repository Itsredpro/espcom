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
#include <esp_wifi.h>
#include <esp_now.h>
#include "espcom_v2.h"


#define DEBUG true

#define SCANNER_CHANNEL_WAITTIME 2000 // 2000ms = 2 sec (26 sec total scan time for all 13 channels)
#define SCANNER_MAX_CHANNEL 13

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

uint8_t* myMacAddress() {
    static uint8_t baseMac[6];  // Make it static
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);  // Get the MAC address for the STA (station) interface
    if (ret == ESP_OK) {
        return baseMac; 
    } else {
        #if DEBUG
            log_e(F("Espcom - an error occurred while fetching the board's MAC address."));
        #endif
        static uint8_t errorMac[6] = {0, 0, 0, 0, 0, 0};
        return errorMac;
    }
}

bool espcom_broadcast_i(uint8_t peerAddr[6], uint8_t* data, size_t data_size) {
  esp_err_t result = esp_now_send(peerAddr, data, data_size);
  return result == ESP_OK;
}

bool espcom_broadcast(void* incomingdata, size_t total_size) {
  default_struct struct_1;
  
  // Copy only the default_struct portion from incomingdata
  memcpy(&struct_1, incomingdata, sizeof(struct_1));

  return espcom_broadcast_i(struct_1.receiver_id, (uint8_t*)incomingdata, total_size);
}



// ESP DATA CALLBACK register
void espcom_ondata_cb(void (*cb)(communication_struct)){
  dataFunc = cb;
}



//SETTINGS
void espcom_set_mode(bool Host){ // Set mode, should only be called before initializing.... (so once init is called)
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

bool espcom_init(){
  return espcom_init_wifi() == true && espcom_init_espnow() == true;
}


// <========> outbound communication functions <============>

void espcom_sendGameJoinFrame(game_struct game){ // (CLIENT)
  join_request_struct struct_1;
  memcpy(struct_1.transmitter_id, myMacAddress(), sizeof(struct_1.transmitter_id));
  memcpy(struct_1.receiver_id, game.host_id, sizeof(struct_1.receiver_id));

  espcom_broadcast((void*) &struct_1, sizeof(struct_1));
}

void espcom_sendPairingBeacon(game_status_types gametype){ // (HOST) broadcast game beacon on current wifi channel
  pairing_struct struct_1;
  memcpy(struct_1.transmitter_id, myMacAddress(), sizeof(struct_1.transmitter_id));
  memcpy(struct_1.receiver_id, mac_all_devices, sizeof(struct_1.receiver_id));

  struct_1.wifi_channel = WiFi.channel();
  struct_1.game_status = gametype;

  espcom_broadcast((void*) &struct_1, sizeof(struct_1));
}

void espcom_sendGameJoinAcceptFrame(uint8_t client_id[6], response_types res){ //(HOST) accept game join request
  join_response_struct struct_1;
  memcpy(struct_1.transmitter_id, myMacAddress(), sizeof(struct_1.transmitter_id));
  memcpy(struct_1.receiver_id, client_id, sizeof(struct_1.receiver_id));

  struct_1.response = res;

  espcom_broadcast((void*) &struct_1, sizeof(struct_1));
}


//<=============> sync functions <=============>

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


void espcom_auto_joingame(game_struct);  //TODO: make this function


//<===============> procces sync tasks <=================>

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



// <===============> ESPNOW FUNCTIONS <=================>

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) { //receive data
  default_struct struct_1;
  memcpy(&struct_1, incomingData, sizeof(struct_1));


  if (!compareArrays_byte(struct_1.protocol_id, protocol_id)){ // Ignore messages that aren't using our protocol..
    #if DEBUG
      log_d(F("Espcom - received unprocessable espnow message."));
    #endif
    return;
  }

  if (struct_1.frame_type == (NULL || 0)){
    #if DEBUG
      log_d(F("Espcom - received malformed frame."));
    #endif
    return;
  }
  //TODO: process frames
}
