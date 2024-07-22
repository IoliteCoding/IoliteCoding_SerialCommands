#include <SerialCommands.h>

bool _blink = false;
bool _ledOn = false;
int _onTime = 2000;
long _onStarted = 0;
int _offTime = 2000;
long _offStarted = 0;

void setup()
{
    Cmd.setCommand(ProcessCommand);
    // First 10 commands (0-9) are system commands.
    // They can be overidden but let us start at address 10.
    Cmd.begin(115200);
    Cmd.setAddressLenght(2); // AddressLenght is default 1
    Cmd.addCommand(10, ProcessCommandBlinkLED);
    Cmd.addCommand(52, ProcessCommandBlinkLEDContinuous);
    Cmd.addCommand(316, logSimpleMessage);

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    // Call readOne as often as possible since this only reads one byte at a time.
    Cmd.readOne();

    if (_blink)
    {
        if (_ledOn && _onStarted + _onTime < millis())
        {
            digitalWrite(LED_BUILTIN, LOW);
            _offStarted = millis();
            _ledOn = false;
        }
        else if (!_ledOn && _offStarted + _offTime < millis())
        {
            digitalWrite(LED_BUILTIN, HIGH);
            _onStarted = millis();
            _ledOn = true;
        }
    }
    else if (_ledOn)
    {
        // Reset LED if _blink= false and _ledOn= true
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void ProcessCommand(int commandAddress, byte *data, int dataLength)
{
    // This method will ALWAYS be called if there is an incoming command.

    // For demo purpose: write the command back to Serial log
    if (commandAddress > 9)
    {
        Cmd.write(commandAddress, data, dataLength);
    }
}

void ProcessCommandBlinkLED(int commandIndex, byte *data, int dataLength)
{
    // This method will only be called if the address of the command is 10.
    // This method is called AFTER ProcessCommand (the method set by Cmd.setCommand()).

    int length = data[0]; // We will send the delay length with the command.
    length *= 100;        // Since we can only use 0 -> 253, it is easier to just give the duration as a tenth of a second.

    digitalWrite(LED_BUILTIN, HIGH);
    delay(length);
    digitalWrite(LED_BUILTIN, LOW);

    // To call this method, send this message as hex: FE 03 00 10 14 FF

    // 0xFE -> 254 (start byte)
    // 0x03 -> 3 (total length of the message, without start & stop byte, so 2 (command address) + 1 (data))

    // command index
    // 0x00 -> 0 (0 * 200)
    // 0x0A -> 10
    // command index is (0 * 200) + 10

    // data
    // 0x14 -> 20 (data[0] -> this will be the waiting time in tenths of a second)

    // 0xFF -> 255 (stop byte)
}

void ProcessCommandBlinkLEDContinuous(int commandIndex, byte *data, int dataLength)
{
    // This method will only be called if the index of the command is 52.
    // The index 52 is purposefully chosen to demonstrate that the index is free to choose.
    // This method is called AFTER ProcessCommand (the method set by Cmd.setCommand()).

    _blink = data[0] == 1; // Start blinking.
    Serial.print("Blinking is ");
    if (_blink)
    {
        Serial.println("ON");
    }
    else
    {
        Serial.println("OFF");
    }

    if (dataLength > 1)
    {
        int onTime = data[1];
        if (onTime > 0)
        {
            _onTime = onTime * 100;
            Serial.println("On time is set to " + String(_onTime) + "ms");
        }
    }

    if (dataLength > 2)
    {
        int offTime = data[2];
        if (offTime > 0)
        {
            _offTime = offTime * 100;
            Serial.println("Off time is set to " + String(_onTime) + "ms");
        }
    }

    // To start blinking the LED with default delay time, send this message as hex: FE 03 00 34 01 FF
    // To stop blinking the LED, send this message as hex: FE 03 00 34 00 FF
    // To change only the "on time" to 1 sec while blinking, send this message as hex: FE 04 00 34 01 0A FF
    // To change only the "off time" to 1 sec while blinking, send this message as hex: FE 05 00 34 01 00 0A FF
    // To change on and off time to 1.5 sec while blinking, send this message as hex: FE 05 00 34 01 0E 0E FF

    // 0xFE -> 254 (start byte)
    // 0x0? -> depends on the length of the data

    // command index
    // 0x00 -> 0 (0 * 200)
    // 0x34 -> 52

    // data
    // 0x0? -> 0 to stop, 1 to start blinking
    // 0x?? -> tenths of a second that the LED is ON, 0 to omit setting this value
    // 0x?? -> tenths of a second that the LED is OFF, 0 to omit setting this value

    // 0xFF -> 255 (stop byte)
}

void logSimpleMessage(int commandAddress, byte *data, int messageSize)
{
    // This command will be executed when the address is 316.

    for (int i = 0; i < messageSize; i++)
    {
        Serial.write(data[i]);
    }
    Serial.println();

    // Send this as hex and find out: FE 0E 01 74 48 65 6C 6C 6F 20 77 6F 72 6C 64 21 FF

    // 0xFE -> 254 (start byte)
    // 0xxx ->  (message lenght)

    // 0x01 -> 200 (1 * 200)
    // 0x74 -> 116
    // Address = (1 * 200) + 116

    // ASCII message

    // 0xFF -> 255 (stop byte)
}
