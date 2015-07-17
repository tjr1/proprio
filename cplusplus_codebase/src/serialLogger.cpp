// Terminal Serial Test 2.cpp : main project file.

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <exception>
#include <iostream>
#include <math.h>

using namespace System;
using namespace System::IO::Ports;

int initializeSerialLogger(array<System::String ^> ^args)
{
	// *********************************************************
	// DECLARATIONS
	String^ sp_output;
	String^ sp_name;
	int baudRate=9600;
	SYSTEMTIME t;
	SYSTEMTIME sys_t;
	SYSTEMTIME start_t;
	String^ datatowrite = "H\n";


	// *********************************************************
	// Initalization
	Console::WriteLine("Arduino Connection Setup: Enter Port Name");
	sp_name = Console::ReadLine();
	// arduino settings
	SerialPort^ _serialPort;
	_serialPort = gcnew SerialPort(sp_name, baudRate);

	GetSystemTime(&start_t); 
	printf("HEADERFILE,ANIMAL:001,TIME:%02d.%02d.%02d.%03d\n", start_t.wHour, start_t.wMinute, start_t.wSecond, start_t.wMilliseconds);

	// *********************************************************
	// TRY: Open Port
	try
	{
		int counter = 0;
		_serialPort->Open();

		do
		{
			t = elapsedTimeSYSTIME(start_t);

			sp_output = _serialPort->ReadLine();
			printf("%02d.%02d.%03d:%s\n",t.wMinute, t.wSecond,t.wMilliseconds,sp_output);		// t.wMinute, t.wHour
			
			//Console::WriteLine(printOut);
			//Console::WriteLine(t.wSecond + "," + t.wMilliseconds + ":" + sp_output);
			
			counter = counter + 1;

			if (counter % 10 == 0)
			{
				_serialPort->Write(datatowrite);
			}
		}
		while (counter < 100);
		
		// Close Port
		_serialPort->Close();
	}


	// *********************************************************
	// CATCH: Wrong Port
	catch (IO::IOException^ e  ) 
	{ 
		Console::WriteLine(e->GetType()->Name+": Port is not ready");
	}


	// *********************************************************
	// CATCH: Wrong Port Syntax
	catch (ArgumentException^ e)
	{
		Console::WriteLine(e->GetType()->Name+": incorrect port name syntax, must start with COM/com");
	}


	// *********************************************************
	// End Program
	Console::Write("Press enter to close the program");
	Console::Read();
    return 0;
}

