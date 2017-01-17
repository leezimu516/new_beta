//kaikk, 1
//2017.1.14

//code the push button in the setup and loop
//automation works fine, slove the splash when third jam moving up
//remount the blender, the height of cleaning cycle need to be changed------->>>changed back
//cancel safety trigger in machine.c, and cancel the sensor when using push button
//11.24 add combination of buttons---reblend:blend and initialize
//                                ---jog pump: blend and clean
//11.25 add unjam func when blender go to the fruits, try to solve the motor stuck issue
//11.28 change machine.h current_step from char to int to slove step over 127
//11.29 add second speed at blend begin
//11.30 slove after initial, blender and pump wont start 
//      cancel ultrasonic sencor for now
//12.05 add top jets at end of blending squence, more shake off
//1.4 try to implement new beta with photo resistor
//1.6 implement elec latch
//1.7 try to implement button debounce and one button multiple functions
//1.11 relay didnt work, becasue machine_stop fun
//1.12 blink button when pushed
//1.13 fix bug of button's interference
//     calibrate the photo resistor
//1.14 calibrate the height(temp)


/*
 * TODO: 
 * LEDs not implelemted
 * 
 */
#include "global.h"


#ifdef __cplusplus 
extern "C" {
#endif
  #include "actions.h"
  #include "machine.h"
  #include "blender.h"
  #include "Button.h"
#ifdef __cplusplus 
}
#endif

#ifdef USB_COMMUNICATION
#include "usb_comm.h"
#endif

machine_t machines[NUMBER_OF_MACHINES];

void auto_cycle_start(char*);
void clean_cycle_start(char*);
void initialize(char*);
void stop_machine(char*);
void machine_reblend(char*);
void disable_keypad(char* message);

hmi_message_t heartbeat_msg;

long last_heartbeat;

//define and declare the physical buttons
#define PUSH_BLEND_BUTTON  26//24;reverse due to the wire
#define PUSH_CLEAN_BUTTON  24//26;
#define PUSH_INITIALIZE  28
#define BLEND_BLINK 30
#define CLEAN_BLINK 32
#define INITIALIZE_BLINK 34

#define PULLUP true
#define INVERT true
#define DEBOUNCE_MS 20
#define LONG_PRESS 1000 
#define BLEND_BLINK 30
#define CLEAN_BLINK 32
#define INITIALIZE_BLINK 34
boolean val,OpenState, OpenState2,OpenState3; 

Button BlendBtn(PUSH_BLEND_BUTTON, PULLUP, INVERT, DEBOUNCE_MS);
Button CleanBtn(PUSH_CLEAN_BUTTON, PULLUP, INVERT, DEBOUNCE_MS);
Button InitialBtn(PUSH_INITIALIZE, PULLUP, INVERT, DEBOUNCE_MS);

void setup() {
  int i;
#ifdef USB_COMMUNICATION
  // set up the serial port for communucation
  Serial.begin(115200);
#endif

  // initialize the patterns
  blend_actions_init(0);
  clean_actions_init();
  initializing_action_init();
 
  // initialize the machine
  for (i = 0; i < NUMBER_OF_MACHINES; i++) {
    machine_init(&machines[i]);
  }
  
  // initialize the mediator
  mediator_init();

  // initialize the logger
  logger_init();

  // For the time being, explicitly initialize the machine
  //machines[0].current_state = MACHINE_STATE_INITIALIZING;
  
  mediator_register(MEDIATOR_AUTO_CYCLE_START, auto_cycle_start);
  mediator_register(MEDIATOR_CLEAN_CYCLE_START, clean_cycle_start);
  mediator_register(MEDIATOR_INITIALIZE, initialize);
  mediator_register(MEDIATOR_STOP_REQUEST, stop_machine);
  mediator_register(MEDIATOR_REBLEND, machine_reblend);
  mediator_register(MEDIATOR_JOG_TOP,machine_jog_top);
  mediator_register(MEDIATOR_JOG_BOTTOM, machine_jog_bottom);
  mediator_register(MEDIATOR_MOVE_UP, machine_move_up);
  mediator_register(MEDIATOR_MOVE_DOWN, machine_move_down);
  mediator_register(MEDIATOR_DISABLE_KEYPAD, disable_keypad);

  heartbeat_msg.message_id = MSG_HEARTBEAT;

  LOG_PRINT(LOGGER_INFO, "Setup complete");
  
  last_heartbeat = millis();
  machines[0].last_step_time = millis();


  /*pinMode(PUSH_BLEND_BUTTON,INPUT_PULLUP);
  pinMode(PUSH_CLEAN_BUTTON,INPUT_PULLUP);
  pinMode(PUSH_INITIALIZE,INPUT_PULLUP);*/

  //add if arduino restart, blender will go to top. temp hack for now
   machines[0].current_state = MACHINE_STATE_INITIALIZING;

   
   //light the led button
   pinMode (BLEND_BLINK, OUTPUT);
   pinMode (CLEAN_BLINK, OUTPUT);
   pinMode (INITIALIZE_BLINK, OUTPUT);
   digitalWrite(BLEND_BLINK,HIGH);
   digitalWrite(CLEAN_BLINK,HIGH);
   digitalWrite(INITIALIZE_BLINK,HIGH);

   



}

void loop() {
  int i = 0;
#ifdef USB_COMMUNICATION
  // check if there are any messages to process
  usb_communication_process();
#endif

  // initialize the machine
  for (i = 0; i < NUMBER_OF_MACHINES; i++) {
    machine_check_safety_conditions(&machines[i]);
    machine_process(&machines[i]);

  } 

  // send out heartbeat message
  if (millis() > last_heartbeat + 1000) {
    last_heartbeat = millis(); 
    usb_communication_send_message(heartbeat_msg, 0);
  }
  
  //button implementation
  BlendBtn.read();
  CleanBtn.read();
  InitialBtn.read();

  // move up part 1
  if(BlendBtn.pressedFor(LONG_PRESS))
  {
     machines[0].buttons[MOVE_UP].current_state = 1;
     OpenState3 = 1;   
  }

  //blend
  if(BlendBtn.wasReleased() && !OpenState3)
  {
    for(int i=0; i<10; ++i)
     {
      val = !val;
      digitalWrite(BLEND_BLINK,val);
      delay(200);
     }
     digitalWrite(BLEND_BLINK,HIGH);

    
    if(machines[0].current_state != MACHINE_STATE_CLEANING)
      {
        digitalWrite(DOOR_ADDRESS,0);
        machines[0].current_state = MACHINE_STATE_BLENDING;
      }
  }

  // move up part 2
  if(OpenState3 && BlendBtn.isReleased())
  {
     machines[0].buttons[MOVE_UP].current_state = 0;
     OpenState3 = 0;   
  }

  

  

  //filling jog pump part1
  if(CleanBtn.pressedFor(LONG_PRESS))
  {
    
      digitalWrite(LIQUID_FILLING_VALVE_ADDRESS, 0); 
      //digitalWrite(CLEANING_VALVE_ADDRESS,0);
      digitalWrite(PUMP_ADDRESS, 0);
      OpenState2 = 1;
     
  }

  //clean
  if(!OpenState2 && CleanBtn.wasReleased())
  {
    for(int i=0; i<10; ++i)
     {
      val = !val;
      digitalWrite(CLEAN_BLINK,val);
      delay(200);
     }
     digitalWrite(CLEAN_BLINK,HIGH);
     if(machines[0].current_state != MACHINE_STATE_BLENDING)
        machines[0].current_state = MACHINE_STATE_CLEANING;
        //machines[0].current_state = MACHINE_STATE_INITIALIZING;
  }

  //filling jog pump part2
  if(OpenState2 && CleanBtn.isReleased())
  {
      digitalWrite(LIQUID_FILLING_VALVE_ADDRESS, 1); 
      //digitalWrite(CLEANING_VALVE_ADDRESS,0);
      digitalWrite(PUMP_ADDRESS, 1);
      OpenState2 = 0;
      
  }

  

  //move down part1
  if(InitialBtn.pressedFor(LONG_PRESS))
  {
    machines[0].buttons[MOVE_DOWN].current_state = 1;
    OpenState = 1;   
  }

  //initialize
  if(InitialBtn.wasReleased() && !OpenState)
  {
    /*for(int i=0; i<10; ++i)
     {
      val = !val;
      digitalWrite(INITIALIZE_BLINK,val);
      delay(200);
     }
     digitalWrite(INITIALIZE_BLINK,HIGH);*/
    digitalWrite(DOOR_ADDRESS,1);
    machines[0].current_state = MACHINE_STATE_INITIALIZING;
  }
  // move down part2
  if(OpenState &&  InitialBtn.wasReleased())
  {
    
    machines[0].buttons[MOVE_DOWN].current_state = 0;
    OpenState = 0;  
    
  }

}

// TODO: seriously, we need to add validation to this, otherwise
// we can change from blending to cleaning without stopping.....
void auto_cycle_start(char* args) {
  LOG_PRINT(LOGGER_INFO, "Starting auto cycle");
  machines[0].current_state = MACHINE_STATE_BLENDING;
}

void clean_cycle_start(char*) {
  LOG_PRINT(LOGGER_INFO, "Starting clean cycle");
  machines[0].current_state = MACHINE_STATE_CLEANING;
}

void initialize(char*){
  LOG_PRINT(LOGGER_INFO, "Initializing");
  machines[0].current_state = MACHINE_STATE_INITIALIZING;
}

void stop_machine(char* args) {
  LOG_PRINT(LOGGER_INFO, "Stopping machine");
  machines[0].current_state = MACHINE_STATE_IDLE;
}

void machine_reblend(char* message){
  LOG_PRINT(LOGGER_VERBOSE, "Starting reblending");
  blend_sequence.actions_ptr[5].activate.state = OFF;
  blend_sequence.actions_ptr[51].activate.state = OFF;
  machines[0].current_state = MACHINE_STATE_BLENDING;
}


void machine_jog_top(char* message){
  LOG_PRINT(LOGGER_VERBOSE, "Jogging top");
  machines[0].buttons[JOG_PUMP_BUTTON].current_state = !machines[0].buttons[JOG_PUMP_BUTTON].current_state;
}

void machine_jog_bottom(char* message){
  LOG_PRINT(LOGGER_VERBOSE, "Jogging bottom");
  digitalWrite(PUMP_ADDRESS, 0);
  digitalWrite(CLEANING_VALVE_ADDRESS, 0);
}

void machine_move_up(char* message){
  LOG_PRINT(LOGGER_VERBOSE, "Moving up");
  machines[0].buttons[MOVE_UP].current_state = !machines[0].buttons[MOVE_UP].current_state;
}

void machine_move_down(char* message){
  send_status("Moving down");
  machines[0].buttons[MOVE_DOWN].current_state = !machines[0].buttons[MOVE_DOWN].current_state;
}

void disable_keypad(char* message) {
  send_status("Keypad toggled");
  machines[0].keypad_enabled = ! machines[0].keypad_enabled;
}

