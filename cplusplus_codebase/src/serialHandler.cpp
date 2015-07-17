#include "serialHandler.h"

//---------------------------------------------------------------------------
// DECLARED NAMESPACES
//---------------------------------------------------------------------------
using namespace std;
using namespace System;

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------
//const int SAMPLING_RATE = 100;		// Hz

//---------------------------------------------------------------------------
// DECLARED GLOBAL VARIABLES
//---------------------------------------------------------------------------
String^ arduinoDeviceName;
bool serialPortConnected;

int baudRate = 9600;
SerialPort^ _serialPort;



//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------
bool initializeLogger(void);

//---------------------------------------------------------------------------
// CONSTRUCTOR
//---------------------------------------------------------------------------
SerialHandler::SerialHandler(void)
{
	// Get Port
	Console::WriteLine("Please Enter the Serial Port Connected to the Ardunio, (i.e. \"COM5\"):");
	String^ portName = Console::ReadLine();
	
	// Create a new SerialPort object with default settings.
	
	_serialPort = gcnew SerialPort(portName, baudRate);

    // Set the read/write timeouts
    _serialPort->ReadTimeout = 500;
    _serialPort->WriteTimeout = 500;

	try 
	{
		_serialPort->Open();
		serialPortConnected = true;
		_serialPort->WriteLine("Device Status");
		arduinoDeviceName = _serialPort->ReadLine();
		
		Console::WriteLine("Ardunio Communication Established.");
		Console::WriteLine("Ardunio: " + arduinoDeviceName);
	}
	catch (TimeoutException ^) 
	{ 
		Console::WriteLine("Port Timeout: No Response from Arduino.");
	}
	// CATCH: Wrong Port
	catch (IO::IOException^ e) 
	{ 
		Console::WriteLine(e->GetType()->Name+": Port is not ready");
	}
	// CATCH: Wrong Port Syntax
	catch (ArgumentException^ e)
	{
		Console::WriteLine(e->GetType()->Name+": incorrect port name syntax, must start with COM/com");
	}

	initializeHandler();
}

bool SerialHandler::initializeHandler(void)
{
	// reSet ReadTimeout
	_serialPort->ReadTimeout = 10;
	return true;
}

bool SerialHandler::readSerialPort(void)
{
	try { serialOutput = _serialPort->ReadLine(); }
	catch (TimeoutException ^) { serialOutput = ""; }
		
	if (serialOutput != "")
	{
		keyboardString = msclr::interop::marshal_as< std::string >(serialOutput);
		std::string delimiter = ",";

		size_t pos = 0;
		std::string token;
		while ((pos = keyboardString.find(delimiter)) != std::string::npos) {
			token = keyboardString.substr(0, pos);
			arduino_vars.push_back(atoi(token.c_str()));
			keyboardString.erase(0, pos + delimiter.length());
		}

		if (arduino_vars.size() > 2)
		{
			ardout_buttonPressed		= arduino_vars[0];
			ardout_capSen_sipperTube	= arduino_vars[1];
			ardout_capSen_handleOutside = arduino_vars[2];
			//cout << ardout_buttonPressed << " " << ardout_capSen_sipperTube << " " << ardout_capSen_handleOutside << "\n";
		}

	//	arduino_vars.clear();
	}
