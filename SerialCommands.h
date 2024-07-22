/*
	SerialCommands.h - Library for receiving commands form other devices.
	Created by Stef Geysels - 2024
*/

#pragma once
#ifndef SerialCommands_h
#define SerialCommands_h

#include "Arduino.h"

typedef void (*CommandRecieved)(int, byte *, int);
typedef void (*OnErrorHandler)(String, int,byte*,int);

class SerialCommands
{
public:
	SerialCommands();
	SerialCommands(Stream *stream);
	void init(Stream *stream);
	void begin(unsigned long baudrate);
	void setCommand(CommandRecieved command);
	void addCommand(int commandAddress, CommandRecieved command);
	void onError(OnErrorHandler handler );
	void readOne();
	void write(int commandAddress, uint8_t *data, int size);
	void setLog(bool log);
	void setAddressLenght(int value);

private:
	bool _log = false;

	uint8_t *_buffer;
	int _messageSize = 0;
	int _bufferSize = 32;
	int _bufferIndex = 0;

	int _startBit = 254;
	int _stopBit = 255;
	int _addressLength = 1;
	int _addressFactor=200;

	static Stream *_serial;
	CommandRecieved _command = NULL;

	int _numberOfCommands = 10;
	CommandRecieved *_commands = new CommandRecieved[_numberOfCommands]{nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};

	int _numberOfInternalCommands = 0;
	void (SerialCommands::*_internalCommands[10])(int, byte *, int);

	OnErrorHandler _onErrorPointer;

	String _hexChars[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

	void processCommand(uint8_t *data);
	void addToBuffer(int c);
	void clearBuffer();
	void log(String str);
	String toHexString(uint8_t byte);
	String toHexString(uint8_t *data, int size, String delimitter);
	int pow(int value, unsigned int power);

	// Error handling
	void raiseError(String message, int commandAddress, byte* data,int messageLenght);

	// Internal commands
	void addInternalCommand(int commandAddress, void (SerialCommands::*func)(int, byte *, int));
	void setAddressLength(int commandAddress, byte *data, int messageLength);
	void setAddressFactor(int commandAddress, byte *data, int messageLength);
	void internalCommand2(int commandAddress, byte *data, int messageLength);
	void internalCommand3(int commandAddress, byte *data, int messageLength);
	void internalCommand4(int commandAddress, byte *data, int messageLength);
	void internalCommand5(int commandAddress, byte *data, int messageLength);
	void internalCommand6(int commandAddress, byte *data, int messageLength);
	void internalCommand7(int commandAddress, byte *data, int messageLength);
	void setLog(int commandAddress, byte *data, int messageLength);
	void getStatus(int commandAddress, byte *data, int messageLength);
};

extern SerialCommands Cmd;

#endif

#pragma hdrstop
