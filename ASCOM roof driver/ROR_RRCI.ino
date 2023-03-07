/*
Name:		ASCOM_Roof_arduino.ino
Created:	12/15/2017 7:05:23 PM
Author:		dromazan

Sketch is developed for (Micro, Leonardo, other 32u4-based) to utilize 5 interrupts on pins 0, 1, 2, 3, 7
internal PULLUP - https://docs.arduino.cc/tutorials/generic/digital-input-pullup 
*/
#include <MsTimer2.h>
//#include <EEPROM.h>

// connect motor controller pins to Arduino digital pins
// motor one
//#define enA 10		// enable pin. it's pulled to the +5V with jumper for max speed, so won't use it
#define in1 8  // driver in1 pin
#define in2 9  //driver in2 pin

//Manual control elements
#define btn_open 0  // suitable for interrupts in Arduino Micro
#define btn_close 1
#define btn_stop 7
//#define switch_force 10  //to force roof closing if telescope is not parked

#define sensor_open 2   // open sensor pin
#define sensor_close 3  // close sensor pin
#define sensor_park 4   //telescope park sensor

//int roof_position;  // roof position
bool raised = false;
const int eeAddress = 0;

int roof_state = 7;  //roof state
bool opening = false;
bool closing = false;

const unsigned long opening_time = 40000;  // time for roof opening in milliseconds

String serialin;  //incoming serial data
String str;       //store the state   of opened/closed/safe pins

// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(9600);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(sensor_open, INPUT_PULLUP);   // LOW when circuit is closed // Pulled to the GND - https://docs.arduino.cc/tutorials/generic/digital-input-pullup
  pinMode(sensor_close, INPUT_PULLUP);  // LOW when circuit is closed
  pinMode(btn_close, INPUT_PULLUP);     // HIGH when it's open, and LOW when it's pressed
  pinMode(btn_open, INPUT_PULLUP);      // HIGH when it's open, and LOW when it's pressed
  pinMode(btn_stop, INPUT_PULLUP);      // HIGH when it's open, and LOW when it's pressed

  //pinMode(switch_force, INPUT);

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  MsTimer2::set(opening_time, error_stop_roof);  //setup timer

  Serial.write("RRCI#");  //init string

  //roof_position = EEPROM.read(eeAddress);
}


void get_state() //seems like it's not needed anymore
{ 
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
  } 
  else if (roof_state == 2)  // if roof is opening
  {
    //do nothing
  }
  else if (digitalRead(sensor_close) == LOW)  // if roof is closed -> open
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
  }
  else if (roof_state == 3)  // if closing
  {
    // do nothing
  }
  else if (digitalRead(sensor_open) == LOW)  // if opened -> close
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
}

void error_stop_roof() {
  MsTimer2::stop();
  detachInterrupt(0);
  detachInterrupt(1);
  halt_motor();
  opening = false;
  closing = false;
  roof_state = 7;  // Error
}

void triger_open() {
  roof_state = 2;  // opening
  opening = true;
  MsTimer2::start();                                                  //start timer
  attachInterrupt(digitalPinToInterrupt(sensor_open), is_open, LOW);  // setup interrupt from open sensor
  turn_motor(1);
}

void triger_close() {
  roof_state = 3;  // closing
  closing = true;
  MsTimer2::start();                                                     //start timer
  attachInterrupt(digitalPinToInterrupt(sensor_close), is_closed, LOW);  // setup interrupt from closed sensor
  turn_motor(0);
}

void is_open() {
  MsTimer2::stop();
  detachInterrupt(digitalPinToInterrupt(sensor_open));
  opening = false;
  halt_motor();
  roof_state = 0;  // Open
}

void is_closed() {
  MsTimer2::stop();
  detachInterrupt(digitalPinToInterrupt(sensor_close));
  closing = false;
  halt_motor();
  roof_state = 1;  // Closed
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

  // trigger open/close roof if button pressed
  attachInterrupt(digitalPinToInterrupt(btn_open), open_roof, HIGH);
  attachInterrupt(digitalPinToInterrupt(btn_close), close_roof, HIGH);
  attachInterrupt(digitalPinToInterrupt(btn_stop), stop_roof, HIGH);

  while (Serial.available() > 0) {
    //Read   Serial data and alocate on serialin

    serialin = Serial.readStringUntil('#');

    if (serialin == "on") {  // turn scope sensor on
      // I don't have to enable park sensor via relay
      //   digitalWrite(sensor_park, HIGH);
    }

    if (serialin == "off") {  // turn scope sensor off
      // I don't have to disable park sensor via relay
      //digitalWrite(sensor, LOW);
    }

    if (serialin == "x") {
      //this is turning off open relay. does this mean to abort opening?
      //digitalWrite(open, LOW);
      stop_roof();
    }

    if (serialin == "open") {
      // if (digitalRead(sensor) == LOW) {  //turn on   IR scope sensor
      //   digitalWrite(sensor, HIGH);
      // }
      // delay(1000);
      if (digitalRead(sensor_park) == LOW) {  //open only if scope safe
        //digitalWrite(open, HIGH);
        //delay(1000);
        //digitalWrite(sensor, LOW);
        open_roof();
      }
    }
    if (serialin == "y") {
      //this is turning off close relay. does this mean to abort closing?
      //digitalWrite(close, LOW);
      stop_roof();
    }

    else if (serialin == "close") {
      // if (digitalRead(sensor) == LOW) {  //turn on IR scope sensor
      //   digitalWrite(sensor, HIGH);
      // }
      // delay(1000);

      if (digitalRead(sensor_park) == LOW) {  //close only if scope safe
        //digitalWrite(close, HIGH);
        //delay(1000);
        //digitalWrite(sensor, LOW);
        close_roof();
      }
    }
  }

  if (serialin == "Parkstatus") {  // exteranl query command to fetch RRCI data
    Serial.println("0#");
    serialin = "";
  }

  if (serialin == "get") {  // exteranl query command to fetch RRCI data
  //it seens like it's forming a string with 3 coma separated values and sends it wia serial port

    // the first if-else is for getting roof state - open/closed or unknown if it's in any other opsition (moving or stopped) 
    if (digitalRead(sensor_open) == LOW) {
      str += "opened,";
      roof_state = 0;  //opened
    } 
    else if (digitalRead(sensor_close) == LOW) {
      str += "closed,";
      roof_state = 1;  //closed
    }
    else if ((digitalRead(sensor_close) == HIGH) && (digitalRead(sensor_open) == HIGH)) {
      str += "unknown,";
    }

    // the second if-else is for getting tepescope state - parked or not
    if (digitalRead(sensor_park) == LOW) {
      str += "safe,";
    } else if (digitalRead(sensor_park) == HIGH) {
      str += "unsafe,";
    }

    // the thirf if-else is to determine if telescope is moving or not
    if ((digitalRead(sensor_close) == HIGH) && (digitalRead(sensor_open) == LOW) && (roof_state != 7 || roof_state !=4)) {
      str += "not_moving_o#";
    }
    else if ((digitalRead(sensor_close) == LOW) && (digitalRead(sensor_open) == HIGH) && (roof_state != 7 || roof_state !=4)) {
      str += "not_moving_c#";
    }
    else if ((digitalRead(sensor_close) == HIGH) && (digitalRead(sensor_open) == HIGH) && (opening == true || closing == true)) {
      str += "moving#";
    } 
    else if ((digitalRead(sensor_close) == HIGH) && (digitalRead(sensor_open) == HIGH) && (roof_state != 7 || roof_state !=4)) {
      str += "unknown#";
    }
    if (str.endsWith(",")) {
      str += "unknown#";
    }

    Serial.println(str);  //send serial data
    serialin = "";
    str = "";
    //delay(100);
  }

  if (serialin == "Status") {
    Serial.println("RoofOpen#");
  }

  serialin = "";
}
