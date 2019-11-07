/*
Name:		ASCOM_Roof_arduino.ino
Created:	12/15/2017 7:05:23 PM
Author:	dromazan
*/
#include <MsTimer2.h>
#include <EEPROM.h>

// connect motor controller pins to Arduino digital pins
// motor one
//#define enA 10		// enable pin. it's pulled to the +5V with jumper for max speed, so won't use it
#define in1 8		// driver in1 pin
#define in2 9		//driver in2 pin

//Manual control elements
#define btn_open 11
#define btn_close 12
#define switch_force 10
#define park_sensor 14


// #define relay_open 8	// open relay pin
// #define relay_close 9	// close relay pin
#define sensor_open 2	// open sensor pin
#define sensor_close 3	// close sensor pin
#define heating 7		// heating relay pin
#define ir_led_pin 5	//ir led pin
#define ir_sensor_pin 6	// phototransistor pin

int roof_position;		// roof position
boolean raised = false;
int eeAddress = 0;

int state = 7; //roof state
int heat_state; //heating state
boolean opening = false;
boolean closing = false;
boolean force = false;

unsigned long opening_time = 40000; // time for roof opening in seconds

									// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600);
	Serial.flush();
	pinMode(in1, OUTPUT);
	pinMode(in2, OUTPUT);
	pinMode(heating, OUTPUT);
	pinMode(sensor_open, INPUT);
	pinMode(sensor_close, INPUT);
	pinMode(btn_close, INPUT);
	pinMode(btn_open, INPUT);
	pinMode(switch_force, INPUT);
	pinMode(park_sensor, INPUT);
	
	MsTimer2::set(opening_time, error_stop_roof); //setup timer

	roof_position = EEPROM.read(eeAddress);
}

// the loop function runs over and over again until power down or reset
void loop() {

	String str;
	String cmd;
	String param;

	if (Serial.available() > 0)
	{
		str = Serial.readStringUntil('#');
		//Serial.print(cmd);
		if (str.length() == 3)
		{
			cmd = str;
		}

		if (str.length() == 6)
		{
			cmd = str.substring(0, 4);
			param = str.substring(4);
		}

		if (cmd == "HSH") // handshake
		{
			Serial.print("HSH");
		}

		else if (cmd == "GST") // get state
		{
			get_state();
		}
		else if (cmd == "OPN") //open
		{
			open_roof();
		}
		else if (cmd == "CLS") //close
		{
			close_roof();
		}
		else if (cmd == "STP") //stop
		{
			stop_roof();
		}
		else if (cmd == "HON") //heat on
		{
			rails_heating_on();
		}
		else if (cmd == "HOF") //heat off
		{
			rails_heating_off();
		}
		else if (cmd == "HST") //heat state
		{
			get_heat_state();
		}
		else
		{
			Serial.print("Bad request");
		}
	}

	if (digitalRead(btn_open) == HIGH)
	{
		open_roof();
	}

	if (digitalRead(btn_close) == HIGH)
	{
		close_roof();
	}
}

void get_state()
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
	if (digitalRead(sensor_open) == LOW)
	{
		state = 0;
	}
	if (digitalRead(sensor_close) == LOW)
	{
		state = 1;
	}
	if (opening == true)
	{
		state = 2;
	}
	if (closing == true)
	{
		state = 3;
	}
	if (digitalRead(sensor_open) != LOW && digitalRead(sensor_close) != LOW && opening == false && closing == false && state != 4)
	{
		state = 7; // Error
	}

	Serial.print(state);
}

void open_roof()
{
	if (digitalRead(sensor_open) == LOW)
	{
		state = 0;
		Serial.print(state);
	}
	else if (digitalRead(sensor_close) == LOW)
	{
		triger_open();
	}
	else if (state == 2)
	{
		Serial.print(state);
	}
	else if (state == 3)
	{
		stop_roof();
		delay(1500);
		triger_open();
	}
	else if (state == 7 || state == 4)
	{
		triger_open();
	}
}

void close_roof()
{
	if (digitalRead(sensor_close) == LOW)
	{
		state = 1;
		Serial.print(state);
	}
	else if (digitalRead(sensor_open) == LOW)
	{
		triger_close();
	}
	else if (state == 3)
	{
		Serial.print(state);
	}
	else if (state == 2)
	{
		stop_roof();
		delay(1500);
		triger_close();
	}
	else if (state == 4 || state == 7)
	{
		triger_close();
	}
}

void stop_roof()
{
	MsTimer2::stop();
	detachInterrupt(0);
	detachInterrupt(1);
	digitalWrite(relay_open, LOW);
	digitalWrite(relay_close, LOW);
	opening = false;
	closing = false;
	state = 4; // Stop
	Serial.print(state);
}

void error_stop_roof()
{
	MsTimer2::stop();
	detachInterrupt(0);
	detachInterrupt(1);
	digitalWrite(relay_open, LOW);
	digitalWrite(relay_close, LOW);
	opening = false;
	closing = false;
	state = 7; // Error
	Serial.print(state);
}

void triger_open()
{
	state = 2;
	Serial.print(state);
	MsTimer2::start(); //start timer
	attachInterrupt(0, is_open, LOW); // setup interrupt from open sensor
	digitalWrite(in1, HIGH);
	digitalWrite(in2, LOW);
	roof_progress_increment();
}

void triger_close()
{
	state = 3;
	Serial.print(state);
	MsTimer2::start(); //start timer
	attachInterrupt(1, is_closed, LOW); // setup interrupt from open sensor
	digitalWrite(in1, LOW);
	digitalWrite(in2, HIGH);
}

void is_open()
{
	MsTimer2::stop();
	detachInterrupt(0);
	opening = false;
	digitalWrite(in1, LOW);
	digitalWrite(in2, LOW);
	state = 0; // Open
	Serial.print(state);
}

void is_closed()
{
	MsTimer2::stop();
	detachInterrupt(1);
	closing = false;
	digitalWrite(in1, LOW);
	digitalWrite(in2, LOW);
	state = 1; // Closed
	Serial.print(state);
}

void rails_heating_on()
{
	digitalWrite(heating, HIGH);
	heat_state = 5;
	Serial.print(heat_state);
}

void rails_heating_off()
{
	digitalWrite(heating, LOW);
	heat_state = 6;
	Serial.print(heat_state);
}

void get_heat_state()
{
	if (digitalRead(heating) == HIGH)
	{
		heat_state = 5;
	}
	else if (digitalRead(heating) == LOW)
	{
		heat_state = 6;
	}
	Serial.print(heat_state);
}

void roof_progress_increment()
{
	digitalWrite(ir_led_pin, HIGH);
	if (ir_sensor_pin == HIGH && raised == false)
	{
		++roof_position;
		EEPROM.update(eeAddress, roof_position);
		Serial.print("p" + roof_position);
		raised = true;
	}
}

void roof_progress_decrement()
{
	digitalWrite(ir_led_pin, HIGH);
	if (ir_sensor_pin == HIGH && raised == false)
	{
		--roof_position;
		EEPROM.update(eeAddress, roof_position);
		Serial.print("p" + roof_position);
		raised = true;
	}
}
