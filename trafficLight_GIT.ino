#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <Bounce2.h> //for debouncing
#include <Event.h>  //dependency of timer?
#include <Timer.h>  //for timing things, crucial, libraries make coding easy

#define OFF     0x0
#define RED     0x1
#define YELLOW  0x3
#define GREEN   0x2
#define TEAL    0x6
#define BLUE    0x4
#define VIOLET  0x5
#define WHITE   0x7 

//output pins
#define REDPIN    4
#define YELLOWPIN 5
#define GREENPIN  6
#define STATUSPIN 13
//#define POTPIN    A0
//buttons
#define eclick    7
#define eup       9
#define edown     8
#define b1        12
#define b2        11
//natural constants for computations
#define second    1000
#define minute    60000
#define INCREMENT_AMNT 15 //seconds

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
Timer t; //object for timing
Bounce bb1, bb2, ea, eb, bc;
long int greenFor=25*60, yellowFor=4.5*60; //vars for timing light changes (in seconds)
unsigned int redEvent, yellowEvent;  //for holding references to timers so they can be stopped prematurely
unsigned int tickEvent; //event reference 
unsigned long int secondsPassed;  //accumulator for time passed in running set
bool runningSet; //other state variables

enum MODES_enum {AUTO_MODE=0, MAN_MODE=1};
MODES_enum mode;

enum ENCODER_opt {ECLICK=0, INCREASE=1, DECREASE=2};

enum SELECTED_interval {SET_NO_INTERVAL=0, SET_GREEN_INTERVAL=1, SET_YELLOW_INTERVAL=2};
SELECTED_interval selectedInterval = SET_NO_INTERVAL;

void setup() {
  // Debugging output
  //Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);

  mode = AUTO_MODE;
  
  //call constructors for bonces
  bb1 = Bounce();
  bb2 = Bounce();
  ea = Bounce();
  eb = Bounce();
  bc = Bounce();

  //output setup
  pinMode(REDPIN, OUTPUT);
  pinMode(YELLOWPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(STATUSPIN, OUTPUT);

  //button settup
  pinMode(eclick, INPUT);
  digitalWrite(eclick, HIGH);
  bc.attach(eclick);
  bc.interval(5);
  pinMode(eup, INPUT);
  digitalWrite(eup, HIGH);
  ea.attach(eup);
  ea.interval(0);
  pinMode(edown, INPUT);
  digitalWrite(edown, HIGH);
  eb.attach(edown);
  eb.interval(0);
  pinMode(b2, INPUT);
  digitalWrite(b2, HIGH);
  bb2.attach(b2);
  bb2.interval(5);
  pinMode(b1, INPUT);
  digitalWrite(b1, HIGH);
  bb1.attach(b1);
  bb1.interval(5);

  //power on self test haha
  all_on();
  delay(second/2);
  all_off();


  lcd.setCursor(0,0);
  lcd.print("Created by");
  lcd.setCursor(0,1);
  lcd.print("Sam Nelson");
  delay(1000);
  lcd.clear();
  for(int i=20; i>=0; i--){
    lcd.setCursor(i,0);
    lcd.print("Brian sucks.");
    delay(50);
  }
  delay(100);
  
  update_screen();
  on(STATUSPIN);
}

void loop() {

  if(mode == AUTO_MODE){//start stop check button (BUTTON1 HANDLER)
    if(bb1.fell()){
      if(!runningSet){
        start_set();
       }
      else if(runningSet){
        stop_set();
      }
    }

  }
  
  if(bb2.fell()){//BUTTON 2 HANDLER mode
    if(!runningSet){
      changeMode();   
    }
  }
  else if(bc.fell()){//encoder click HANDLER
    //encoder click
    set_delays(ECLICK);
  }

//for potentiometer setting
//  if(selectedInterval != SET_NO_INTERVAL){
//    analog_update_interval(map(analogRead(POTPIN), 1023, 0, 0, 6000));
//  }
  
  t.update(); //necessary to update timer
  bb1.update();
  bb2.update();
  bc.update();
  ea.update();
  eb.update();
  
  //rotary encoder
  if(ea.rose() && eb.read() == LOW) set_delays(INCREASE);
  if(eb.fell() && ea.read() == HIGH) set_delays(DECREASE);
}

void changeMode(){
  if(mode == AUTO_MODE){
    selectedInterval = SET_NO_INTERVAL;
    mode = MAN_MODE;
    if(runningSet){
      stop_set();
    }
  }
  else{
    mode = AUTO_MODE;
    all_off();
  }
  update_screen();
}

//void analog_update_interval(int value){
//  if(selectedInterval == SET_GREEN_INTERVAL){
//    greenFor=value;
//  }
//  else if(selectedInterval == SET_YELLOW_INTERVAL){
//    yellowFor=value;
//  }
//  update_screen();
//}

void set_delays(ENCODER_opt command){
  //0-next interval 1-increase 2-decrease
  if(!runningSet && mode==AUTO_MODE){
    switch(command){
      case 0:
        switch(selectedInterval){
          case SET_NO_INTERVAL:
            selectedInterval=SET_GREEN_INTERVAL;
            break;
          case SET_GREEN_INTERVAL:
            selectedInterval=SET_YELLOW_INTERVAL;
            break;
          case SET_YELLOW_INTERVAL:
            selectedInterval=SET_NO_INTERVAL;
            break;
        }
        break;
      case 1: //increase
        switch(selectedInterval){
          case SET_GREEN_INTERVAL:
            greenFor+=INCREMENT_AMNT;
            break;
          case SET_YELLOW_INTERVAL:
            yellowFor+=INCREMENT_AMNT;
            break;
        }
        break;
      case 2:
        switch(selectedInterval){
          case SET_GREEN_INTERVAL:
            greenFor-=INCREMENT_AMNT;
            if(greenFor<0) greenFor=0;
            break;
          case SET_YELLOW_INTERVAL:
            yellowFor-=INCREMENT_AMNT;
            if(yellowFor<0) yellowFor=0;
            break;
        }
        break;
    }
    update_screen();
    //UPDAE SCREEN DISPLAY
    
  }
  else if(mode == MAN_MODE){
    //all_off();
    switch(command){
      case 0:
        if(is_high(GREENPIN) && is_high(YELLOWPIN) && is_high(REDPIN)){
          all_on();
        }
        else{
          all_off();
        }
        break;
      case 1:
        if(!is_high(REDPIN)) {all_off();}
        else if(!is_high(YELLOWPIN)) {switch_red();}
        else if(!is_high(GREENPIN) ) {switch_yellow();}
        else{switch_green();}
        break;
      case 2:
        if(!is_high(REDPIN)) {switch_yellow();}
        else if(!is_high(YELLOWPIN)) {switch_green();}
        else if(!is_high(GREENPIN)) {all_off();}
        else{switch_red();}
        break;
    }
  }
}

void update_screen(){
  lcd.setCursor(0,0);
  //Serial.print("MODE: ");
  lcd.print("Mode:");

  if(mode==AUTO_MODE){
    //Serial.print("Auto  ");
    if(!runningSet) on(STATUSPIN);
    lcd.print("Auto ");
    if(selectedInterval != SET_NO_INTERVAL){
      //Serial.println();
      lcd.print("Setup");
      lcd.setCursor(0,1);
      switch(selectedInterval){
        case SET_GREEN_INTERVAL:
          //Serial.print("Green For: ");
          //Serial.print(greenFor/60);
          //Serial.print(":");
          //Serial.println(greenFor%60);
          
          lcd.print("Grn}");
          lcd.print(greenFor/60);
          lcd.print(":");
          lcd.print(greenFor%60);
          lcd.print("          ");
          break;
        case SET_YELLOW_INTERVAL:
          //Serial.print("Yellow For: ");
          //Serial.print(yellowFor/60);
          //Serial.print(":");
          //Serial.println(yellowFor%60);

          lcd.print("Ylw}");
          lcd.print(yellowFor/60);
          lcd.print(":");
          lcd.print(yellowFor%60);
          lcd.print("          ");
          break;
      } 
    }
    else if(runningSet){
      //Serial.print("RUNNING  Elapsed Time: ");
      //Serial.print(secondsPassed/60);
      //Serial.print(":");
      //Serial.println(secondsPassed%60);
      //Serial.print("Yellow at:");
      //Serial.print(greenFor/60);
      //Serial.print(":");
      //Serial.print(greenFor%60);
      //Serial.print(" Red at:");
      //Serial.print((yellowFor+greenFor)/60);
      //Serial.print(":");
      //Serial.println((yellowFor+greenFor)%60);
      
      //lcd.print("RUN"); //overflows line
      lcd.print(" ");
      lcd.print(secondsPassed/60);
      lcd.print(":");
      lcd.print(secondsPassed%60);
      lcd.print("  ");
      lcd.setCursor(0,1);
      lcd.print("Y}");
      lcd.print(greenFor/60);
      lcd.print(":");
      lcd.print(greenFor%60);
      lcd.print(" R}");
      lcd.print((yellowFor+greenFor)/60);
      lcd.print(":");
      lcd.print((yellowFor+greenFor)%60);
      }
    else{
       //Serial.println("Stopped");
       //Serial.print("Green For:");
       //Serial.print(greenFor/60);
       //Serial.print(":");
       //Serial.print(greenFor%60);
       //Serial.print(" Yellow For:");
       //Serial.print(yellowFor/60);
       //Serial.print(":");
       //Serial.print(yellowFor%60);

       lcd.print("Stop    ");
       lcd.setCursor(0,1);
       lcd.print("G}");
       lcd.print(greenFor/60);
       lcd.print(":");
       lcd.print(greenFor%60);
       lcd.print(" Y}");
       lcd.print(yellowFor/60);
       lcd.print(":");
       lcd.print(yellowFor%60);
       lcd.print("       ");
    }
    //Serial.println();
  }
  else if(mode==MAN_MODE){
    off(STATUSPIN);
    //Serial.print("MANUAL");
    lcd.print("MANUAL       ");
    lcd.setCursor(0,1);
    lcd.print("                   ");
  }
  //Serial.println();
}

/*
 * Starts the timers to activate the lights at the proper intervals
 */
void start_set(){
  unsigned long int ye = (greenFor*second);
  unsigned long int re = ((greenFor*second)+(yellowFor*second));
  //Serial.println(ye);
  //Serial.println(re);
  yellowEvent = t.after(ye, switch_yellow);
  redEvent = t.after(re, switch_red);
  secondsPassed = 0;
  tickEvent = t.every(second, count_seconds);
  switch_green();
  runningSet=true;
  selectedInterval=SET_NO_INTERVAL;  
}


void stop_set(){
  t.stop(redEvent);
  t.stop(yellowEvent);
  t.stop(tickEvent);
  all_off();
  runningSet=false;
  update_screen();
  on(STATUSPIN);
}

/*
 * counts seconds while set is running and displays them on lcd
 */
void count_seconds(){
  secondsPassed++;
  if(secondsPassed < 60){
    //print secondsPassed on screen 
  }
  else{
    //print secondsPassed/60 + ":" + secondsPassed%60 to screen
  }
  
  if(secondsPassed%2 == 0){
    on(STATUSPIN);
  }
  else{
    off(STATUSPIN);
  }
  update_screen();
}

/*
 * shut off all lights
 */
void all_off(){
  off(REDPIN);
  off(YELLOWPIN);
  off(GREENPIN);
  lcd.setBacklight(WHITE);
}

/*
 * turn on all lights
 */
void all_on(){
  on(REDPIN);
  on(YELLOWPIN);
  on(GREENPIN);
  lcd.setBacklight(VIOLET);
}

/*
*Switch to Display Yellow only
*/
void switch_green(){//turn off all turn on yellow
  off(REDPIN);
  off(YELLOWPIN);
  on(GREENPIN);
  lcd.setBacklight(GREEN);
}

/*
*Switch to Display Yellow only
*/
void switch_yellow(){//turn off all turn on yellow
  off(REDPIN);
  off(GREENPIN);
  on(YELLOWPIN);
  lcd.setBacklight(YELLOW);
}

/*
*Switch to Display Red only
*/
void switch_red(){//turn off all turn on red
  off(GREENPIN);
  off(YELLOWPIN);
  on(REDPIN);
  lcd.setBacklight(RED);
}

//Turn off a pin, specifically a light
void on(int pin){
  switch(pin){
    case REDPIN:
    case YELLOWPIN:
    case GREENPIN:  //intentional fallthrough 2x
      digitalWrite(pin, LOW);
      break;
    default:
      digitalWrite(pin, HIGH);
      break;
  }
}

//Turn on a pin, specifically a light
void off(int pin){
  switch(pin){
    case REDPIN:
    case YELLOWPIN:
    case GREENPIN:  //intentional fallthrough 2x
      digitalWrite(pin, HIGH);
      break;
    default:
      digitalWrite(pin, LOW);
      break;
  }
}

boolean is_high(int pin){
  switch (digitalRead(pin)){
    case HIGH:
      return true;
    default:
      return false;
  }
}

