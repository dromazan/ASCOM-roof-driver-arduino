/*
Name:		ASCOM_Roof_arduino.ino
Created:	12/15/2017 7:05:23 PM
Author:	dromazan
*/
#include <MsTimer2.h>
#include <EEPROM.h>

int relay_open = 8; // open relay pin
int relay_close = 9; // close relay pin
int sensor_open = 2; // open sensor pin
int sensor_close = 3; // close sensor pin
int heating = 7; // heating relay pin
int ir_led_pin = 5; //ir led pin
int ir_sensor_pin = 6; // phototransistor pin

int roof_position; // roof position
boolean raised = false;
int eeAddress = 0;

int state = 7; //roof state
int heat_state; //heating state
boolean opening = false;
boolean closing = false;

unsigned long opening_time = 40000; // time for roof opening in seconds

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600);
	Serial.flush();
	pinMode(relay_open, OUTPUT);
	pinMode(relay_close, OUTPUT);
	pinMode(heating, OUTPUT);
	pinMode(sensor_open, INPUT);
	pinMode(sensor_close, INPUT);

	MsTimer2::set(opening_time, error_stop_roof); //setup timer

	roof_position = EEPROM.read(eeAddress);
}

// the loop function runs over and over again until power down or reset
void loop() {

	String cmd;

	if (Serial.available() > 0)
	{
		cmd = Serial.readStringUntil('#');
		//Serial.print(cmd);

		if (cmd == "HANDSHAKE")
		{
			Serial.print("HANDSHAKE");
		}

		if (cmd == "GETSTATE")
		{
			get_state();
		}
		else if (cmd == "OPEN")
		{
			open_roof();
		}
		else if (cmd == "CLOSE")
		{
			close_roof();
		}
		else if (cmd == "STOP")
		{
			stop_roof();
		}
		else if (cmd == "HEATON")
		{
			rails_heating_on();
		}
		else if (cmd == "HEATOFF")
		{
			rails_heating_off();
		}
		else if (cmd == "HEATSTATE")
		{
			get_heat_state();
		}
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
	digitalWrite(relay_close, LOW);
	digitalWrite(relay_open, HIGH);
	roof_progress_increment();
}

void triger_close()
{
	state = 3;
	Serial.print(state);
	MsTimer2::start(); //start timer
	attachInterrupt(1, is_closed, LOW); // setup interrupt from open sensor
	digitalWrite(relay_open, LOW);
	digitalWrite(relay_close, HIGH);
}

void is_open()
{
	MsTimer2::stop();
	detachInterrupt(0);
	opening = false;
	digitalWrite(relay_open, LOW);
	digitalWrite(ir_led_pin, LOW);
	state = 0; // Open
	Serial.print(state);
}

void is_closed()
{
	MsTimer2::stop();
	detachInterrupt(1);
	closing = false;
	digitalWrite(relay_close, LOW);
	digitalWrite(ir_led_pin, LOW);
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