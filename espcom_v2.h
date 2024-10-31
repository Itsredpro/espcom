#pragma once


//each espnow message should start with this sequence of bytes
byte protocol_id[6] = {2,50,22,184,69,249};
uint8_t mac_all_devices[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

enum exit_types {GAME_ENDED, KICKED, HOST_ERROR};
enum game_status_types {NOT_RUNNING, JOINABLE, STARTING, ONGOING, ENDED};
enum response_types {JOINED, REJECTED};


/*
frame types:

Game frames
1 = game data frame

Game management frames
2 = esp pairing frame
3 = game join request frame
4 = game join response frame
5 = game exit frame
*/


typedef struct {
  // this struct is present in each frame

  //protocol identifier
  byte protocol_id[6] = {2,50,22,184,69,249};

  //comms data
  uint8_t transmitter_id[6] = {0,0,0,0,0,0};
  uint8_t receiver_id[6] = {0,0,0,0,0,0};

  //frame identifier
  byte frame_type;

} default_struct;




// wireless communication structs:
typedef struct {
  //protocol identifier
  byte protocol_id[6] = {2,50,22,184,69,249};

  //comms data
  uint8_t transmitter_id[6] = {0,0,0,0,0,0};
  uint8_t receiver_id[6] = {0,0,0,0,0,0};

  //frame identifier
  byte frame_type = 1;



  //  -<frame data>-

  //number channels
  long channel1;
  long channel2;
  long channel3;
  long channel4;
  long channel5;
  long channel6;
  long channel7;
  long channel8;
  long channel9;
  long channel10;
  long channel11;
  long channel12;
  long channel13;
  long channel14;
  long channel15;
  long channel16;
  long channel17;
  long channel18;

  //bool channels
  bool btn1;
  bool btn2;
  bool btn3;
  bool btn4;
  bool btn5;
  bool btn6;
  bool btn7;
  bool btn8;
  
  //text channels
  char text1[10];
  char text2[20];
  char text3[20];
  char text4[40];
  char text5[40];
} communication_struct;


typedef struct {
  //protocol identifier
  byte protocol_id[6] = {2,50,22,184,69,249};

  //comms data
  uint8_t transmitter_id[6] = {0,0,0,0,0,0};
  uint8_t receiver_id[6] = {0,0,0,0,0,0};

  //frame identifier
  byte frame_type = 2;



  //  -<frame data>-
  int wifi_channel;
  bool isaHost = true;
  game_status_types game_status;

} pairing_struct;


typedef struct {
  //protocol identifier
  byte protocol_id[6] = {2,50,22,184,69,249};

  //comms data
  uint8_t transmitter_id[6] = {0,0,0,0,0,0};
  uint8_t receiver_id[6] = {0,0,0,0,0,0};

  //frame identifier
  byte frame_type = 3;

} join_request_struct;

typedef struct {
  //protocol identifier
  byte protocol_id[6] = {2,50,22,184,69,249};

  //comms data
  uint8_t transmitter_id[6] = {0,0,0,0,0,0};
  uint8_t receiver_id[6] = {0,0,0,0,0,0};

  //frame identifier
  byte frame_type = 4;

  response_types response = JOINED;

} join_response_struct;



// object structs:

typedef struct {
  bool found_game = false;
  uint8_t host_id[6];
  int channel;
} game_struct;



//important functions
void espcom_tick(); //process sync tasks


//callbacks
void espcom_ondata_cb(void (*cb)(communication_struct)); // on data
void espcom_onend_cb(void (*cb)(exit_types)); // on exit
void espcom_onjoin_cb(void (*cb)()); // on join
void espcom_onprocesserror_cb(void (*cb)()); //unprocessable frame

//init functions
void espcom_set_mode(bool Host);
bool espcom_init_wifi();
bool espcom_init_espnow();

//automatic game functions
void espcom_auto_findgame(void (*cb)(game_struct));
void espcom_auto_joingame(game_struct);
