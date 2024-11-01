#include "espcom_v2.h"

void setup() {
  // put your setup code here, to run once:

  
  //Client example:
  espcom_set_mode(false); // host = false

  //(make sure all callbacks are registered before starting otherwise unexpected behavior may occur)
  espcom_init(); //initializes wifi and espnow capabilities.

  espcom_auto_findgame([](game_struct){ // scan wifi channels 1-13 for game beacon frames
    //Do something when a game is found..
    log_d("Found a game on channel: " + String(game_struct.channel));
  });

  //Host example:
  espcom_set_mode(true); // host = true
  espcom_init();

  espcom_sendPairingBeacon(JOINABLE); //send a single game pairing frame (on current wifi channel)

}

void loop() {
  // put your main code here, to run repeatedly:
  espcom_tick(); //important for processing sync tasks
}
