/*
Name:		ASCOM_Roof_arduino.ino
Created:	12/15/2017 7:05:23 PM
Author:		dromazan

Sketch is developed for Arduino Mega 2560 to utilize 5 interrupts on pins 2, 3, 18, 19, 20, 21
internal PULLUP - https://docs.arduino.cc/tutorials/generic/digital-input-pullup 
*/
#include <MsTimer2.h>
//#include <EEPROM.h>

// connect motor controller pins to Arduino digital pins
// motor one
//#define enA 10		// enable pin. it's pulled to the +5V with jumper for max speed, so won't use it
#define in1 8  // driver in1 pin
#define in2 9  //driver in2 pin
#define enA 7 //driver enable LOW/HIGH. pwm doesn't work

//Manual control elements
#define btn_open 2  // suitable for interrupts in Arduino Micro
#define btn_close 3
#define btn_stop 18
//#define switch_force 10  //to force roof closing if telescope is not parked

#define sensor_open 21   // open sensor pin
#define sensor_close 20  // close sensor pin
#define sensor_park 19   //telescope park sensor

//int roof_position;  // roof position
//bool raised = false;
//const int eeAddress = 0;

int roof_state = 1;  //roof state
int current_state;
bool opening = false;
bool closing = false;
bool lost = false;  //roof not reporting state

const unsigned long opening_time = 40000;  // time for roof opening in milliseconds
const unsigned long pooltime = 2000;       // period between status broadcasts (2 seconds)

unsigned long pooltimer;  // variable to prompt status update

String cmd;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  Serial.flush();

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enA, OUTPUT);
  pinMode(sensor_open, INPUT_PULLUP);   // LOW when circuit is closed // Pulled to the GND - https://docs.arduino.cc/tutorials/generic/digital-input-pullup
  pinMode(sensor_close, INPUT_PULLUP);  // LOW when circuit is closed
  pinMode(btn_close, INPUT_PULLUP);     // HIGH when it's open, and LOW when it's pressed
  pinMode(btn_open, INPUT_PULLUP);      // HIGH when it's open, and LOW when it's pressed
  pinMode(btn_stop, INPUT_PULLUP);
  pinMode(sensor_park, INPUT_PULLUP);
  //pinMode(switch_force, INPUT);

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(enA, LOW);  

  MsTimer2::set(opening_time, error_stop_roof);  //setup timer

  attachInterrupt(digitalPinToInterrupt(btn_open), open_roof, LOW);
  attachInterrupt(digitalPinToInterrupt(btn_close), close_roof, LOW);
  attachInterrupt(digitalPinToInterrupt(btn_stop), stop_roof, LOW);
  attachInterrupt(digitalPinToInterrupt(sensor_open), is_open, LOW);     // setup interrupt from open sensor
  attachInterrupt(digitalPinToInterrupt(sensor_close), is_closed, LOW);  // setup interrupt from closed sensor

  pooltimer = millis();
  //roof_position = EEPROM.read(eeAddress);
}


int get_state() {
  /*
	For ASCOM Driver:
	shutterOpen		0	Dome shutter status open
	shutterClosed	1	Dome shutter status closed
	shutterOpening	2	Dome shutter status opening
	shutterClosing	3	Dome shutter status closing
	stop			4
	shutterError	7	Dome shutter status error
	*/
  if (digitalRead(sensor_open) == LOW) {
    roof_state = 0;  //opened
  }
  if (digitalRead(sensor_close) == LOW) {
    roof_state = 1;  //closed
  }
  if (opening == true) {
    roof_state = 2;  //opening
  }
  if (closing == true) {
    roof_state = 3;  //closing
  }
  if (digitalRead(sensor_open) != LOW && digitalRead(sensor_close) != LOW && opening == false && closing == false && roof_state != 4) {
    roof_state = 7;  // Error
  }

  Serial.println(roof_state);
  return roof_state;
}

void open_roof() {

  current_state = get_state();

  switch(current_state){
    case 0:
      // roof opened, do nothing
      break;
    case 1:
      // foof closed -> trigger open
      triger_open();
      break;
    case 2:
      // roof is opening, do nothing
      break;
    case 3:
      // roof is closing -> stop and trigger open
      stop_roof();
      delay(1500);
      triger_open();
      break;
    case 7:
      triger_open();
    default:
      triger_open();
  }
}

void close_roof() {

  current_state = get_state();

  switch(current_state){

    case 0:
      // roof opened -> close
      triger_close();
      break;
    case 1:
      // roof is closed, do nothing
      break;
    case 2:
      // roof is opening -> stop and trigger close
      stop_roof();
      delay(1500);
      triger_close();
      break;
    case 3:
      // roof is closing, do nothing
      break;
    case 7:
      triger_close();
    default:
      triger_close();
  }
}

void stop_roof() {
  MsTimer2::stop();
  halt_motor();
  opening = false;
  closing = false;
  roof_state = 4;  // Stop
  Serial.print("stop_roof - ");
  Serial.println(roof_state);
}

void error_stop_roof() {
  MsTimer2::stop();
  halt_motor();
  opening = false;
  closing = false;
  roof_state = 7;  // Error
  Serial.print(roof_state);
}

void triger_open() {
  opening = true;
  roof_state = 2;  // opening
  Serial.print(roof_state);
  MsTimer2::start();  //start timer
  turn_motor(1);
  attachInterrupt(digitalPinToInterrupt(sensor_open), is_open, LOW);
  attachInterrupt(digitalPinToInterrupt(sensor_close), is_closed, LOW);

}

void triger_close() {
  closing = true;
  roof_state = 3;  // closing
  Serial.print(roof_state);
  MsTimer2::start();  //start timer
  turn_motor(0);
  attachInterrupt(digitalPinToInterrupt(sensor_close), is_closed, LOW);
  attachInterrupt(digitalPinToInterrupt(sensor_open), is_open, LOW);

}

void is_open() {
  MsTimer2::stop();
  detachInterrupt(digitalPinToInterrupt(sensor_open));
  opening = false;
  halt_motor();
  roof_state = 0;  // Open
  Serial.print(roof_state);
}

void is_closed() {
  MsTimer2::stop();
  detachInterrupt(digitalPinToInterrupt(sensor_close));
  closing = false;
  halt_motor();
  roof_state = 1;  // Closed
  Serial.print(roof_state);
}

void turn_motor(int dir) {
  if (dir == 1)  //forward
  {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }

  if (dir == 0)  //backward
  {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }

  digitalWrite(enA, HIGH);
}

void halt_motor() {
  
  digitalWrite(enA, LOW);

}

void loop() {

  /*
  Accepted cmd comands:
  OPEN - open roof
  CLOSE - close roof
  STOP - stop roof
  */

  if (Serial.available() > 0)
	{
		cmd = Serial.readStringUntil('#');
		//Serial.print(cmd);


		if (cmd == "OPEN") // handshake
		{
			triger_open();
		}
    if (cmd == "CLOSE") // handshake
		{
			triger_close();
		}
    if (cmd == "STOP") // handshake
		{
			stop_roof();
		}
  }

  if ((millis() - pooltimer) > pooltime) {
    get_state();              // broadcast serial
    pooltimer = millis();  // reset communication timer
  }
}
