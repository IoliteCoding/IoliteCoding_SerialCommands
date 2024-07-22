#include "SerialCommands.h"

Stream *SerialCommands::_serial;

SerialCommands::SerialCommands() : SerialCommands(&Serial) {}

SerialCommands::SerialCommands(Stream *stream)
{
	_buffer = new uint8_t[_bufferSize];
	init(stream);
	addInternalCommand(0, &SerialCommands::setAddressLength);
	addInternalCommand(1, &SerialCommands::setAddressFactor);
	addInternalCommand(2, &SerialCommands::internalCommand2);
	addInternalCommand(3, &SerialCommands::internalCommand3);
	addInternalCommand(4, &SerialCommands::internalCommand4);
	addInternalCommand(5, &SerialCommands::internalCommand5);
	addInternalCommand(6, &SerialCommands::internalCommand6);
	addInternalCommand(7, &SerialCommands::internalCommand7);
	addInternalCommand(8, &SerialCommands::setLog);
	addInternalCommand(9, &SerialCommands::getStatus);
}

void SerialCommands::init(Stream *serial)
{
	_serial = serial;
}

void SerialCommands::begin(unsigned long baudrate)
{
	Serial.begin(baudrate);
}

void SerialCommands::setCommand(CommandRecieved command)
{
	_command = command;
}

void SerialCommands::addCommand(int commandAddress, CommandRecieved func)
{
	if (commandAddress >= _numberOfCommands)
	{
		int newSize = commandAddress + 5;
		CommandRecieved *newCommands = new CommandRecieved[newSize];

		// Fill everything with nullptr
		for (int i=0;i<newSize;i++)
		{
			newCommands[i]=nullptr;
		}

		for (int i = 0; i < _numberOfCommands; i++)
		{
			if (_commands[i] != nullptr)
			{				
				// Serial.println("cmd "+String(i)+" = "+String( _commands[i]!=nullptr));
				newCommands[i] = _commands[i];
			}
		}
		delete[] _commands;
		_commands = newCommands;
		_numberOfCommands = newSize;
	}
	_commands[commandAddress] = func;
}

void SerialCommands::onError(OnErrorHandler handler)
{
	_onErrorPointer=handler;
}

void SerialCommands::readOne()
{
	int c;
	if (_serial and _serial->available() and _serial->peek() >= 0)
	{
		c = _serial->read();
		log(String( c));
		
		if (c == _startBit)
		{
			log("Clearing buffer");
			clearBuffer();
		}
		else if (c == _stopBit)
		{
			log("Stopbit received");
			if (_messageSize == _bufferIndex)
			{
				processCommand(_buffer);
			}
			else
			{
				raiseError("Message size \"" + String(_messageSize) + "\" is not equal to the buffersize: " + String(_bufferIndex), 0, NULL, 0);
			}
		}
		else
		{
			if (_messageSize == 0)
			{
				log("Message size: "+String(c));				
				_messageSize = c;
			}
			else
			{
				addToBuffer(c);
			}
		}
	}
}

void SerialCommands::write(int commandAddress, uint8_t *data, int size)
{
log("Writing command "+String(commandAddress));

	int messageLength = _addressLength + size;
	int totalMessageLength = messageLength + 3; // start byte, length byte & stop byte
	uint8_t message[totalMessageLength];

	int index = 0;
	message[index++] = _startBit;
	message[index++] = messageLength;

	int internalAddress = commandAddress;
	int addressIndex = _addressLength + index - 1;
	for (int i = 0; i < _addressLength; i++)
	{
		log(String(addressIndex) + ": " + String(internalAddress % _addressFactor));
		message[addressIndex--] = internalAddress % _addressFactor;
		internalAddress /= _addressFactor;
		index++;
	}

	for (int i = 0; i < size; i++)
	{
		message[index++] = data[i];
	}

	message[index++] = _stopBit;

	log(toHexString(message, index, " "));
	_serial->write(message, index);
}

void SerialCommands::setLog(bool log)
{
	_log = log;
}

/// @brief The default address length is 1. This supports adresses up to 200.
/// @brief For higher addresses, the overflow will go to a second address byte.
/// @brief For example 314, for this the address to be used, the address lenght must be at least 2
/// @brief and the address bytes will be -> 1 (x 200) + 114
/// @param value
void SerialCommands::setAddressLenght(int value)
{
	_addressLength = value;
}

void SerialCommands::processCommand(uint8_t *data)
{
	log("Processing command");

	int address = 0;
	log("AddressLenght: " + String(_addressLength));
	for (int i = 0; i < _addressLength; i++)
	{
		address += data[i] * pow(_addressFactor, _addressLength - 1 - i);
	}

	log("Address: " + String(address));

	int messageSize = _messageSize - _addressLength;
	uint8_t *newBuffer = new uint8_t[messageSize];

	log("Message buffer size: " + String(messageSize));

	for (int i = 0; i < messageSize; i++)
	{
		newBuffer[i] = data[i + _addressLength];
	}

	if (_command != NULL)
	{
		log("Calling command.");
		_command(address, newBuffer, messageSize);
		log("Command called.");
	}

	if (address < _numberOfCommands and _commands[address] != NULL)
	{
		log("Calling command " + String(address) + ".");
		_commands[address](address, newBuffer, messageSize);
		log("Command " + String(address) + " called.");
	}
	else if (address < _numberOfInternalCommands and _internalCommands[address] != NULL)
	{
		log("Calling internal command " + String(address) + ".");
		(this->*_internalCommands[address])(address, newBuffer, messageSize);
		log("Internal command " + String(address) + " called.");
	}

	delete[] newBuffer;

	clearBuffer();
}

void SerialCommands::addToBuffer(int c)
{
	log("Add to buffer index " + String(_bufferIndex) + ": " + String(c));

	if (_bufferIndex >= _bufferSize - 1)
	{
		int newSize = _bufferSize * 2;
		uint8_t *newBuffer = new uint8_t[newSize];
		for (int i = 0; i < _bufferSize; i++)
		{
			newBuffer[i] = _buffer[i];
		}
		delete[] _buffer;
		_buffer = newBuffer;
		_bufferSize = newSize;
	}

	_buffer[_bufferIndex] = c;
	_bufferIndex++;
}

void SerialCommands::clearBuffer()
{
	for (int i = 0; i < sizeof(_buffer); i++)
	{
		_buffer[i] = 0;
	}
	_messageSize = 0;
	_bufferIndex = 0;
}

void SerialCommands::log(String str)
{
	if (_log)
	{
		Serial.println(str);
	}
}

String SerialCommands::toHexString(uint8_t byte)
{
	return _hexChars[byte / 16] + _hexChars[byte % 16];
}

String SerialCommands::toHexString(uint8_t *data, int size, String delimitter)
{
	String result = "";
	for (int i = 0; i < size; i++)
	{
		result += toHexString(data[i]);
		if (i < size - 1)
		{
			result += delimitter;
		}
	}

	return result;
}

int SerialCommands::pow(int value, unsigned int power)
{
	if (power == 0)
	{
		return 1;
	}
	for (int i = 1; i < power; i++)
	{
		value *= value;
	}
	return value;
}

void SerialCommands::raiseError(String message, int commandAddress, byte *data, int messageLenght)
{
	log(message);
	if (_onErrorPointer != NULL)
	{
		_onErrorPointer(message, commandAddress, data, messageLenght);
	}
}

void SerialCommands::addInternalCommand(int commandAddress, void (SerialCommands::*func)(int, byte *, int))
{
	_internalCommands[commandAddress] = func;
	_numberOfInternalCommands++;
}

void SerialCommands::setAddressLength(int commandAddress, byte *data, int messageLength)
{
	if (messageLength > 0)
	{
		int lenght = data[0];
		if (lenght <= 0)
		{
			lenght = 1;
		}
		_addressLength = lenght;
	}
	log("AddressLength is now: " + String(_addressLength));
}

void SerialCommands::setLog(int commandAddress, byte *data, int messageLength)
{
	_log = (messageLength > 0 and (data[0] == 0x01));
	log("Log is ON");
}

void SerialCommands::setAddressFactor(int commandAddress, byte *data, int messageLength)
{
	if (messageLength > 0)
	{
		_addressFactor = data[0];
	}
	log("Address factor is: " + String(_addressFactor));
}

void SerialCommands::internalCommand2(int commandAddress, byte *data, int messageLength)
{
}

void SerialCommands::internalCommand3(int commandAddress, byte *data, int messageLength)
{
}

void SerialCommands::internalCommand4(int commandAddress, byte *data, int messageLength)
{
}

void SerialCommands::internalCommand5(int commandAddress, byte *data, int messageLength)
{
}

void SerialCommands::internalCommand6(int commandAddress, byte *data, int messageLength)
{
}

void SerialCommands::internalCommand7(int commandAddress, byte *data, int messageLength)
{
}

/// @brief Get the status of all the internal parameters. The parts are separated by '|' (0x7C) and the fist byte is the address of the parameter.
/// @param commandAddress
/// @param data
/// @param messageLength
void SerialCommands::getStatus(int commandAddress, byte *data, int messageLength)
{
	// 0x7C -> |
	uint8_t statusData[] = {
		0x00, _addressLength, 0x7C,
		0x01, _addressFactor, 0x7C,
		0x01, _log, 0x7C};

	int statusMessageLenght = sizeof(statusData) / sizeof(statusData[0]);

	log("Status lenght: " + String(statusMessageLenght));

	write(commandAddress, statusData, statusMessageLenght);
}

SerialCommands Cmd;