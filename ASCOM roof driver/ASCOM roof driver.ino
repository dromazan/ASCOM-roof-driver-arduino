/*
Name:		ASCOM_Roof_arduino.ino
Created:	12/15/2017 7:05:23 PM
Author:	droma
*/
#include <MsTimer2.h>
int relay_open = 8; // open relay pin
int relay_close = 9; // close relay pin
int sensor_open = 2; // open sensor pin
int sensor_close = 3; // close sensor pin

int state; //roof state
boolean opening = false;
boolean closing = false;

unsigned long opening_time = 40000; // time for roof opening in seconds

unsigned long time_count = 0;

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600);
	Serial.flush();
	pinMode(relay_open, OUTPUT);
	pinMode(relay_close, OUTPUT);
	pinMode(sensor_open, INPUT);
	digitalWrite(sensor_open, HIGH);
	pinMode(sensor_close, INPUT);
	digitalWrite(sensor_close, HIGH);

	MsTimer2::set(opening_time, stop_roof); //set up timer
	attachInterrupt(0, is_open, LOW);
	attachInterrupt(1, is_closed, LOW);
}

// the loop function runs over and over again until power down or reset
void loop() {

	String cmd;

	if (Serial.available() > 0)
	{
		cmd = Serial.readStringUntil('#');

		if (cmd == "GETSTATE")
		{
			get_state();
			Serial.print(state);
			Serial.println("#");
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
	shutterError	4	Dome shutter status error
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
	if (digitalRead(sensor_open) != LOW && digitalRead(sensor_close) != LOW && opening == false && closing == false)
	{
		state = 4; // Error
	}
}

void open_roof()
{
	digitalWrite(relay_close, LOW);
	digitalWrite(relay_open, HIGH);
	MsTimer2::start(); //start timer
	opening = true;

	//time_count = millis();
	
	//while (millis() < (time_count + opening_time))
	//{
	//	if (digitalRead(sensor_open) == LOW)
	//	{
	//		opening = false;
	//		state = 0; // Open
	//		digitalWrite(relay_open, LOW);
	//		break;
	//	}
	//}
	//if (digitalRead(sensor_open) != LOW && digitalRead(sensor_close) != LOW)
	//{
	//	digitalWrite(relay_open, LOW);
	//	state = 4; // Error
	//}
	//else
	//{
	//	digitalWrite(relay_open, LOW);
	//	get_state();
	//}
	//Serial.print(state);
	//Serial.print("#");
}

void close_roof()
{
	digitalWrite(relay_open, LOW);
	digitalWrite(relay_close, HIGH);
	closing = true;

	//time_count = millis();
	//while (millis() < (time_count + opening_time))
	//{
	//	if (digitalRead(sensor_close) == LOW)
	//	{
	//		closing = false;
	//		state = 1; // Closed
	//		digitalWrite(relay_close, LOW);
	//		break;
	//	}
	//}
	//if (digitalRead(sensor_open) != LOW && digitalRead(sensor_close) != LOW)
	//{
	//	digitalWrite(relay_close, LOW);
	//	state = 4; // Error
	//}
	//else
	//{
	//	digitalWrite(relay_close, LOW);
	//	get_state();
	//}
	//Serial.print(state);
	//Serial.print("#");
}

void stop_roof()
{
	MsTimer2::stop();
	digitalWrite(relay_open, LOW);
	digitalWrite(relay_close, LOW);
	opening = false;
	closing = false;
	state = 4; // Error
	Serial.print(state);
	Serial.print("#");
}

void is_open()
{
	MsTimer2::stop();
	opening = false;
	state = 0; // Open
	digitalWrite(relay_open, LOW);
}

void is_closed()
{
	MsTimer2::stop();
	closing = false;
	state = 1; // Closed
	digitalWrite(relay_close, LOW);
}
