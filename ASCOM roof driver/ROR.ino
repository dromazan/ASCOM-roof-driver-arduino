/*
Name:		ASCOM_Roof_arduino.ino
Created:	12/15/2017 7:05:23 PM
Author:		dromazan

Sketch is developed for Arduino Micro to utilize 4 interrupts on pins 0, 1, 2, 3
internal PULLUP - https://docs.arduino.cc/tutorials/generic/digital-input-pullup 
*/
#include <MsTimer2.h>
#include <EEPROM.h>

// connect motor controller pins to Arduino digital pins
// motor one
//#define enA 10		// enable pin. it's pulled to the +5V with jumper for max speed, so won't use it
#define in1 8  // driver in1 pin
#define in2 9  //driver in2 pin

//Manual control elements
#define btn_open 0 // suitable for interrupts in Arduino Micro
#define btn_close 1
//#define switch_force 10  //to force roof closing if telescope is not parked

#define sensor_open 2   // open sensor pin
#define sensor_close 3  // close sensor pin

//int roof_position;  // roof position
boolean raised = false;
const int eeAddress = 0;

int roof_state = 7;  //roof state
boolean opening = false;
boolean closing = false;

const unsigned long opening_time = 40000;  // time for roof opening in milliseconds

// the setup function runs once when you press reset or power the board
void setup() {

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(sensor_open, INPUT_PULLUP); // LOW when circuit is closed // Pulled to the GND - https://docs.arduino.cc/tutorials/generic/digital-input-pullup
  pinMode(sensor_close, INPUT_PULLUP); // LOW when circuit is closed 
  pinMode(btn_close, INPUT_PULLUP); // HIGH when it's open, and LOW when it's pressed
  pinMode(btn_open, INPUT_PULLUP); // HIGH when it's open, and LOW when it's pressed
  //pinMode(switch_force, INPUT);

  MsTimer2::set(opening_time, error_stop_roof);  //setup timer

  //roof_position = EEPROM.read(eeAddress);
}


void get_state() {
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

  Serial.print(roof_state);
}

void open_roof() {
  
  if (digitalRead(sensor_open) == LOW) {
    roof_state = 0;  // roof opened, do nothing
    Serial.print(roof_state);
  }
  else if (roof_state == 2)  // if roof is opening
  {
    //do nothing
    Serial.print(roof_state);
  }

  else if (digitalRead(sensor_close) == LOW) // if roof is closed -> open
  {
    triger_open();
  }

  else if (roof_state == 3)  // if roof is closing -> open
  {
    stop_roof();
    delay(1500);
    triger_open();
  }

  else if (roof_state == 7 || roof_state == 4)  // if error or stopped -> open
  {
    triger_open();
  }
}

void close_roof() {

  if (digitalRead(sensor_close) == LOW) {
    roof_state = 1;  // roof is closed, do nothing
    Serial.print(roof_state);
  } 
  
  else if (roof_state == 3)  // if closing
  {
    // do nothing
    Serial.print(roof_state);
  }

  else if (digitalRead(sensor_open) == LOW) // if opened -> close
  {
    triger_close();
  }

  else if (roof_state == 2)  // if opening -> close
  {
    stop_roof();
    delay(1500);
    triger_close();
  } 
  
  else if (roof_state == 4 || roof_state == 7)  // if error or stopped -> close
  {
    triger_close();
  }
}

void stop_roof() {
  MsTimer2::stop();
  detachInterrupt(0);
  detachInterrupt(1);
  halt_motor();
  opening = false;
  closing = false;
  roof_state = 4;  // Stop
  Serial.print(roof_state);
}

void error_stop_roof() {
  MsTimer2::stop();
  detachInterrupt(0);
  detachInterrupt(1);
  halt_motor();
  opening = false;
  closing = false;
  roof_state = 7;  // Error
  Serial.print(roof_state);
}

void triger_open() {
  roof_state = 2;  // opening
  Serial.print(roof_state);
  MsTimer2::start();                                                  //start timer
  attachInterrupt(digitalPinToInterrupt(sensor_open), is_open, LOW);  // setup interrupt from open sensor
  turn_motor(1);
}

void triger_close() {
  roof_state = 3;  // closing
  Serial.print(roof_state);
  MsTimer2::start();                   //start timer
  attachInterrupt(digitalPinToInterrupt(sensor_close), is_closed, LOW);  // setup interrupt from closed sensor
  turn_motor(0);
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
}

void halt_motor() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void loop() {

  attachInterrupt(digitalPinToInterrupt(btn_open), open_roof, HIGH);

  attachInterrupt(digitalPinToInterrupt(btn_close), close_roof, HIGH);
}
