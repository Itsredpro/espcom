#include "espcom_v2.h"

void setup() {

  //start as client (make sure all callbacks are registered before starting otherwise unexpected behavior may occur)
  espcom_set_mode(false); // host = false
  espcom_init_wifi();
  espcom_init_espnow();

  espcom_auto_findgame([](game_struct){
    //Do something when a game is found
    log_d("Found a game on channel: " + String(game_struct.channel));
  });
}

void loop() {
  // put your main code here, to run repeatedly:
  espcom_tick();
}
