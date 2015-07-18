//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2009 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author    <http://www.chai3d.org>
    \author    Francois Conti
    \version   2.0.0 $Rev: 265 $
*/
//===========================================================================

//---------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <Windows.h>
#include <typeinfo>
#include <msclr/marshal_cppstd.h>
//#include "stdafx.h"
#include <Python.h>
#include <string>
//---------------------------------------------------------------------------
#include "chai3d.h"
#include "logger.h"

//---------------------------------------------------------------------------
// DECLARED NAMESPACES
//---------------------------------------------------------------------------
using namespace std;
using namespace System;
using namespace System::IO::Ports;
using namespace System::ComponentModel;
//using namespace Eigen;


//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// Weight of RATs
//David's Rats: 10/27: 1=320, 11=335, 111=325

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W				= 600;
const int WINDOW_SIZE_H				= 600;

// mouse menu options (right button)
const int OPTION_FULLSCREEN			= 1;
const int OPTION_WINDOWDISPLAY		= 2;

// maximum number of haptic devices supported in this demo
const int MAX_DEVICES				= 8;

// STATES
//const int STATE_CALIBRATION				= -1;
const int STATE_TRIAL_START				= 0;
//const int STATE_TRIAL_RUNNING			= 1;
//const int STATE_SUCCESS					= 2;
//const int STATE_FAILURE					= 999;
//const int STATE_WAIT_ONE_SECOND			= 3;
//const int STATE_RESET_LEVER_POS			= 4;
//const int STATE_CHK_NO_FORCE_APP		= 5;	
const int STATE_LOG_DATA				= 1;
const int STATE_LOGGING					= 2;
const int STATE_FOLLOW_CIRCLE			= 3;
const int STATE_CALC_TRAJ				= 4;
const int STATE_FOLLOW_TRAJ				= 5;

//// STATE_MACHINE_TYPES
//const int TYPE_DISTANCE_OR_TIME		= 0;
//const int TYPE_SIMPLE_DISTANCE		= 1;
//const int TYPE_LEFT_OR_RIGHT		= 2;
//const int TYPE_TWO_PULL				= 3;
//const int TYPE_LEFT					= 4;
//const int TYPE_RIGHT				= 5;
//const int TYPE_LEFT_AND_RIGHT		= 6;

// Home Position (in/out, left/right, up/down)
const cVector3d homePosition (-0.00, 0.0, -0.000); // (-0.013, 0 -0.016) for right

//MatrixXd points;
//points << 1, 1, 1,
//		  3, 2, -1,
//		  2, 7, 0,
//	      -2 -3 -6,
//	      0 0 0;


//---------------------------------------------------------------------------
// DECLARED GLOBAL VARIABLES
//---------------------------------------------------------------------------

// status of the main simulation haptics loop
bool simulationRunning = false;

// has exited haptics simulation thread
bool simulationFinished = false;

// root resource path
string resourceRoot;


// Haptic Variables
cHapticDeviceHandler* handler;		// a haptic device handler
cGenericHapticDevice* hapticDevices[MAX_DEVICES];	// pointers to all haptic devices
int numHapticDevices = 0;	// number of haptic devices detected

// Haptic Forces 
bool useDamping			= false;	// damping mode ON/OFF
bool useTopStiffSpring	= false;	// force field mode ON/OFF
bool useLockHandle		= false;
bool useFollower		= false;

// Graphics
bool usePoints = false;


// statemachine variables
int STATE_MACHINE = 0;
int STATE_MACHINE_TYPE = 0;


// Ardunio Communication
//globalSerialPortObj serialPortObj;
bool ard_serialPortConnected = false;
bool gui_serialPortConnected = false;
bool GLOBAL_FLAG_ard_serialPort_initialized = false;
bool GLOBAL_FLAG_gui_serialPort_initialized = false;


// capacitiveSensors from Ardunio
int ardout_capSen_sipperTube = 0;
int ardout_capSen_handleOutside = 0;
bool ardout_buttonPressed = false;
bool GLOBAL_FLAG_ardin_delieverReward = false;
bool GLOBAL_FLAG_target_left = false;
bool GLOBAL_FLAG_prev_target_left = false;
int reward_counter = 0; 

cPrecisionClock  mm_timer;

// Falcon Variables
cVector3d cur_position;
cVector3d cur_linearVelocity;
cVector3d cur_force;
cVector3d lock_position;

// Keyboard Response from User
string keyboardString;

// Log File
bool GLOBAL_FLAG_WRITE_LOG_TO_DATA = false;
bool GLOBAL_FLAG_LOG_DATA = false;
Logger loggerObj;

//Trial Type 
//Experiment trialType = Experiment();
int subjectID = 999;
int subjectWeight = 0;

//---------------------------------------------------------------------------
// DECLARED GUI VARIABLES
//---------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera that renders the world in a window display
cCamera* camera;

// a light source to illuminate the objects in the virtual scene
cLight *light;

cShapeSphere* guiObj_cursor;

cShapeSphere* guiObj_point1;
//cShapeSphere* guiObj_clstPnt;
cShapeSphere* guiObj_projFromClstPnt;
cShapeLine* guiObj_segmentForce;

// width and height of the current window display
int displayW  = 600;
int displayH  = 600;


//---------------------------------------------------------------------------
// DECLARED MACROS
//---------------------------------------------------------------------------
// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())

#define DEBUG_STATE(a) std::cout << #a << ": " << (a) << std::endl


//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// log file thread
void logFileHandler(void);

// statemachine processing
void stateMachineHandler(void);

// timer
double elapsedTimeMilliSec(SYSTEMTIME start_t);

// timer
double elapsedTimeSec(SYSTEMTIME start_t);

// serial port connection handler
void ard_serialThread(SerialPort^ _serialPort);

// serial port connection handler
void gui_serialThread(SerialPort^ _serialPort);

void serialHandler(void);

// callback when a keyboard key is pressed
void keyboardListener(void);

// function called before exiting the application
void close(void);

// main haptics loop
void updateHaptics(void);

// main graphics loop
void updateGraphics(void);

// graphics loop
void updateChai3DGraphics(void);

// returns path of current executable
string returnCurrentPath(void);

// initialization functions
bool initialization_hapticCalibration(void);
SerialPort^ initialization_ardunioPort(void);
bool initialization_subjectID(void);
bool initialization_loggerFile(void);
bool initialization_STATEMACHINE(void);
SerialPort^ initialization_pyGUI(void);
bool initialization_chai3dGUI(void);
bool initialization_openGL(int argc, char* argv);


//---------------------------------------------------------------------------
// INITIALIZATION FUNCTIONS
//---------------------------------------------------------------------------

bool initialization_hapticCalibration()
{
	// create a haptic device handler
    handler = new cHapticDeviceHandler();
	
    // read the number of haptic devices currently connected to the computer
    numHapticDevices = handler->getNumDevices();

    // limit the number of devices to MAX_DEVICES
    numHapticDevices = cMin(numHapticDevices, MAX_DEVICES);

    // for each available haptic device, create a 3D cursor
    // and a small line to show velocity
    int i = 0;
    while (i < numHapticDevices)
    {

        // get a handle to the next haptic device
        cGenericHapticDevice* newHapticDevice;
        handler->getDevice(newHapticDevice, i);

        // open connection to haptic device
        newHapticDevice->open();

		// initialize haptic device
		newHapticDevice->initialize();

        // store the handle in the haptic device table
        hapticDevices[i] = newHapticDevice;

		// retrieve information about the current haptic device
        cHapticDeviceInfo info = newHapticDevice->getSpecifications();


		// read initial position of device to initialize communication
		cVector3d pos;
		hapticDevices[i]->getPosition(pos);
				
/*		hapticDevices[i]->setForce(cVector3d(2,0,0));
		Console::WriteLine("Move the Falcon Around... Press Enter");
		Console::ReadLine();
		
		hapticDevices[i]->setForce(cVector3d(0,2,0));
		Console::WriteLine("Move the Falcon Around... Press Enter");
		Console::ReadLine();

		hapticDevices[i]->setForce(cVector3d(0,0,2));
		Console::WriteLine("Move the Falcon Around... Press Enter");
		Console::ReadLine();

		hapticDevices[i]->setForce(cVector3d(0,0,0)); */   

        // increment counter
        i++;
    }

	return true;
}

SerialPort^ initialization_ardunioPort()
{
	// Get a list of serial port names.
	array<String^>^ serialPorts = nullptr;
    try
    {
        serialPorts = SerialPort::GetPortNames();
    }
    catch (Win32Exception^ ex)
    {
        Console::WriteLine(ex->Message);
    }

    Console::WriteLine("The following serial ports were found:");

    // Display each port name to the console. 
    for each(String^ port in serialPorts)
		Console::WriteLine(port);


	// Get Port
	Console::WriteLine("Please Enter the Serial Port Connected to the Ardunio, (i.e. \"COM5\"):");
	String^ ard_portName = "COM7";
	//String^ ard_portName = Console::ReadLine();
	String^ ard_deviceName;	
			
	// Create a new SerialPort object with default settings.
	int ard_baudRate = 9600;
	SerialPort^ ard_serialPort;
	ard_serialPort = gcnew SerialPort(ard_portName, ard_baudRate);
	//SerialPortReader.port = gcnew SerialPort(ard_portName, ard_baudRate);

	// Set the read/write timeouts
	ard_serialPort->ReadTimeout = 500;
	ard_serialPort->WriteTimeout = 500;

	try 
	{
		ard_serialPort->Open();
		ard_serialPort->WriteLine("Device Status");
		ard_deviceName = ard_serialPort->ReadLine();
		
		Console::WriteLine("Ardunio Communication Established.");
		Console::WriteLine("Ardunio: " + ard_deviceName);
		ard_serialPortConnected = true;
	}
	catch (TimeoutException ^) 
	{	Console::WriteLine("Port Timeout: No Response from Arduino.");	}
	// CATCH: Wrong Port
	catch (IO::IOException^ e) 
	{	Console::WriteLine(e->GetType()->Name+": Port is not ready");	}
	// CATCH: Wrong Port Syntax
	catch (ArgumentException^ e)
	{	Console::WriteLine(e->GetType()->Name+": incorrect port name syntax, must start with COM/com");	}

	ard_serialPort->ReadTimeout = 10;


	return ard_serialPort;
}

bool initialization_subjectID()
{
	std::string user_input;
		
	//Console::WriteLine("Tom's SUBJECT ID Guide: RED = 101, BLACK = 102, GREEN = 103");
	//Console::WriteLine("David's SUBJECT ID Guide: ONE = 111, TWO = 112, THREE = 113");
	//Console::WriteLine("Test Subject = 999");

	//Console::WriteLine("Please Enter 3-Digit Subject ID: ");
	//cin >> user_input;
	//subjectID = atoi(user_input.c_str());
	subjectID = 101;

	//Console::WriteLine("Please Enter Subject Weight [grams]: ");
	//cin >> user_input;
	//subjectWeight = atoi(user_input.c_str());
	//Console::WriteLine("\n");
	//Console::WriteLine("Please Enter Subject Weight [grams]: ");
	//cin >> user_input;
	subjectWeight = 0;
	//Console::WriteLine("\n");

	return true;
}

bool initialization_loggerFile()
{
	Console::WriteLine("Creating Log File... ");

	if (loggerObj.initializeLogger(subjectID, subjectWeight))
		Console::WriteLine("Log File Created. ");
	else
	{
		Console::WriteLine("ERROR: FILE NOT OPENED!");
		Console::ReadLine();
		Console::WriteLine("Please Restart!");
		Console::ReadLine();
		Console::WriteLine("Closing in 3...!");
		Console::ReadLine();
		Console::WriteLine("Closing in 2...!");
		Console::ReadLine();
		Console::WriteLine("Closing in 1...!");
		Console::ReadLine();

		return false;
	}	

	return true;
}

bool initialization_STATEMACHINE()
{
	//std::string user_input;

	//Console::WriteLine("Select a STATE MACHINE TYPE:");
	//Console::WriteLine(" [0] Move a certain distance or wait 30 sec ");
	//Console::WriteLine(" [1] Move a certain distance ");
	//Console::WriteLine(" [2] Move Left or Right ");
	//Console::WriteLine(" [3] Two lever pushes ");
	//Console::WriteLine(" [4] Move Left ");
	//Console::WriteLine(" [5] Move Right ");
	//Console::WriteLine(" [6] Move Left and Right ");
	//cin >> user_input;
	//STATE_MACHINE_TYPE = atoi(user_input.c_str());

	// State machine type
	//printf("STATE MACHINE TYPE: %d\n", STATE_MACHINE_TYPE);
		
	// INITIAL STATE
	STATE_MACHINE = STATE_TRIAL_START;
	printf("INITAL STATE: %d\n", STATE_MACHINE);

	return true;
}

SerialPort^ initialization_pyGUI()
{
	// create a thread that handles gui communication
	cThread* guiThread = new cThread();
	guiThread->set(updateGraphics, CHAI_THREAD_PRIORITY_GRAPHICS);		


	// Get Port
	//Console::WriteLine("Please Enter the Serial Port Connected to the Ardunio, (i.e. \"COM5\"):");
	String^ gui_portName = "COM11";
	String^ gui_deviceName;		
			
	// Create a new SerialPort object with default settings.
	int gui_baudRate = 115200;
	SerialPort^ gui_serialPort;
	gui_serialPort = gcnew SerialPort(gui_portName, gui_baudRate);

	// Set the read/write timeouts
	gui_serialPort->ReadTimeout = 10000;
	gui_serialPort->WriteTimeout = 500;

	try 
	{
		gui_serialPort->Open();
		gui_serialPort->ReadLine();
		gui_serialPort->WriteLine("Device Status");
		gui_deviceName = gui_serialPort->ReadLine();
		
		Console::WriteLine("trainer_bot: GUI Communication Established.");
		Console::WriteLine("trainer_bot: GUI Device Name is \'" + gui_deviceName + "\'");
		gui_serialPortConnected = true;
	}
	catch (TimeoutException ^) 
	{	 	Console::WriteLine("Port Timeout: No Response from GUI.");	}
	
	// CATCH: Wrong Port
	catch (IO::IOException^ e) 
	{	Console::WriteLine(e->GetType()->Name+": Port is not ready");	}
	
	// CATCH: Wrong Port Syntax
	catch (ArgumentException^ e)
	{	Console::WriteLine(e->GetType()->Name+": incorrect port name syntax, must start with COM/com");	}

	gui_serialPort->ReadTimeout = 10;


	return gui_serialPort;
}

bool initialization_chai3dGUI()
{
	// create a new world.
    world = new cWorld();

    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->setBackgroundColor(0.0, 0.0, 0.0);

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and oriente the camera
    camera->set( cVector3d (0.5, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // lookat position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the "up" vector

    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);

    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d( 2.0, 0.5, 1.0));  // position the light source
    light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam


	// GUI OBJECTS
	// create a cursor by setting its radius
    guiObj_cursor = new cShapeSphere(0.01);
	guiObj_cursor->setPos(cVector3d(0,0,0));
    // add cursor to the world
    world->addChild(guiObj_cursor);

	// trajectory points
	guiObj_point1 = new cShapeSphere(.005);
	guiObj_point1->setPos(cVector3d(.02,.01,.01));
    world->addChild(guiObj_point1);

	// spokes
    cShapeLine* guiObj_lineSegment1 = new cShapeLine(homePosition, cVector3d(0, -0.025, +0.00)+homePosition);
    world->addChild(guiObj_lineSegment1);
	cShapeLine* guiObj_lineSegment2 = new cShapeLine(homePosition, cVector3d(0, -0.02, -0.02)+homePosition);
    world->addChild(guiObj_lineSegment2);
	cShapeLine* guiObj_lineSegment3 = new cShapeLine(homePosition, cVector3d(0, +0.00, -0.025)+homePosition);
    world->addChild(guiObj_lineSegment3);
	cShapeLine* guiObj_lineSegment4 = new cShapeLine(homePosition, cVector3d(0, +0.02, -0.02)+homePosition);
    world->addChild(guiObj_lineSegment4);
	cShapeLine* guiObj_lineSegment5 = new cShapeLine(homePosition, cVector3d(0, +0.025, +0.00)+homePosition);
    world->addChild(guiObj_lineSegment5);

	//// create a cursor by setting its radius
 //   guiObj_clstPnt = new cShapeSphere(0.01);
	//guiObj_clstPnt->setPos(cVector3d(0,0,0));
 //   // add cursor to the world
 //   world->addChild(guiObj_clstPnt);

	// create a small line to illustrate velocity
    guiObj_projFromClstPnt = new cShapeSphere(0.01);
	guiObj_projFromClstPnt->setPos(cVector3d(0,0,0));
    // add line to the world
    world->addChild(guiObj_projFromClstPnt);

	guiObj_segmentForce = new cShapeLine(cVector3d(0,0,0), cVector3d(0,0,0));
	world->addChild(guiObj_segmentForce);


	return true;
}

bool initialization_openGL(int argc, char* argv[])
{
	// initialize GLUT
	glutInit(&argc, argv);

	// retrieve the resolution of the computer display and estimate the position
	// of the GLUT window so that it is located at the center of the screen
	int screenW = glutGet(GLUT_SCREEN_WIDTH);
	int screenH = glutGet(GLUT_SCREEN_HEIGHT);
	int windowPosX = (screenW - WINDOW_SIZE_W) / 2;
	int windowPosY = (screenH - WINDOW_SIZE_H) / 2;

	// initialize the OpenGL GLUT window
	glutInitWindowPosition(windowPosX, windowPosY);
	glutInitWindowSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow(argv[0]);
	glutDisplayFunc(updateChai3DGraphics);
	//glutKeyboardFunc(keySelect);
	//glutReshapeFunc(resizeWindow);
	glutSetWindowTitle("CHAI 3D");

	// create a mouse menu (right button)
	//glutCreateMenu(menuSelect);
	//glutAddMenuEntry("full screen", OPTION_FULLSCREEN);
	//glutAddMenuEntry("window display", OPTION_WINDOWDISPLAY);
	//glutAttachMenu(GLUT_RIGHT_BUTTON);

	return true;
}


//---------------------------------------------------------------------------
// MAIN FUNCTIONS
//---------------------------------------------------------------------------

int main(int argc, char* argv[])
{

    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    printf ("\n");
    printf ("-----------------------------------\n");
    printf ("Automated Trainer\n");
    printf ("Moritz Lab\n");
    printf ("Copyright 2015\n");
	printf ("David Bjanes (dbjanes@uw.edu)\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");
 //   printf ("Keyboard Options:\n\n");
 //   printf ("[1] - Start Training\n");
 //   printf ("[2] - Calibration Falcon\n");
	//printf ("[3] - Setup Arduino\n");
	//printf ("[...] - ...\n");
 //   printf ("[x] - Exit application\n");
 //   printf ("\n\n");

    // parse first arg to try and locate resources
    resourceRoot = string(argv[0]).substr(0,string(argv[0]).find_last_of("/\\")+1);


    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------
		Console::WriteLine("Falcon Calibration -------------------------------- ");
		bool hapticCalibration_success = initialization_hapticCalibration();
		Console::WriteLine("Falcon Calibration Complete ----------------------- \n\n");

	//-----------------------------------------------------------------------
    // GET SUBJECT IDENTIFICATION
    //-----------------------------------------------------------------------
		Console::WriteLine("SUBJECT IDENDIFICATION: --------------------------- ");
		bool subjectID_success = initialization_subjectID();
		Console::WriteLine("SUBJECT IDENDIFICATION COMPLETE: ------------------ \n\n");

	//-----------------------------------------------------------------------
    // LOGGER - CREATE, INITIALIZE FILE
    //-----------------------------------------------------------------------
		Console::WriteLine("LOGGER FILE INITIALIZATION: ----------------------- ");
		bool logFileCreation_success = initialization_loggerFile();

		// Log File Not Created.
		if (logFileCreation_success == false)
			return (0);

		Console::WriteLine("LOGGER FILE INITIALIZATION COMPLETE --------------- \n\n");

    //-----------------------------------------------------------------------
    // STATE MACHINE
    //-----------------------------------------------------------------------	
		Console::WriteLine("STATE MACHINE INITIALIZATION: --------------------- ");
		bool STATEMACHINE_success = initialization_STATEMACHINE();
		Console::WriteLine("STATE MACHINE INITIALIZATION COMPLETE. ------------ \n\n");

	//-----------------------------------------------------------------------
    // SERIAL PORT INTERFACE - ARDUINO
    //-----------------------------------------------------------------------
		Console::WriteLine("Arduino Calibration ------------------------------- ");

		// create a thread that handles data logging
		cThread* ardunioThread = new cThread();
		ardunioThread->set(serialHandler, CHAI_THREAD_PRIORITY_GRAPHICS);

		while (!GLOBAL_FLAG_ard_serialPort_initialized)
			Sleep(100);

		//SerialPort^ ard_serialPort = initialization_ardunioPort();
		Console::WriteLine("Arduino Calibration Complete ---------------------- \n\n");

	//-----------------------------------------------------------------------
    // PYTHON GUI
    //-----------------------------------------------------------------------
		//Console::WriteLine("pyGUI Handshake Initiation ------------------------ ");
		//
		//while (!GLOBAL_FLAG_gui_serialPort_initialized)
		//	Sleep(100);

		//Console::WriteLine("pyGUI Handshake Complete -------------------------- \n\n");
		
	//-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------
		Console::WriteLine("Cha3d GUI Initialization -------------------------- ");
		bool chai3dGUI_success = initialization_chai3dGUI();		

    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------
		bool openGL_success = initialization_openGL(argc, argv);
		Console::WriteLine("Cha3d GUI Initialization Complete ------------------ \n\n");


    //-----------------------------------------------------------------------
    // START SIMULATION
    //-----------------------------------------------------------------------

	Console::WriteLine("--------------------------------------------------- ");
	Console::WriteLine("Calibration Complete! ----------------------------- ");
	Console::WriteLine("--------------------------------------------------- \n\n");
	
	Console::WriteLine("Press Enter to being Training...");
	Console::ReadLine();


    // simulation in now running
    simulationRunning = true;

    // create a thread which starts the main haptics rendering loop
    cThread* hapticsThread = new cThread();
    hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);

	// create a thread which starts the main haptics rendering loop
    cThread* keyboardThread = new cThread();
    keyboardThread->set(keyboardListener, CHAI_THREAD_PRIORITY_GRAPHICS);

	// create a thread which starts the main haptics rendering loop
    cThread* stateMachineThread = new cThread();
    stateMachineThread->set(stateMachineHandler, CHAI_THREAD_PRIORITY_GRAPHICS);

	// create a thread that handles data logging
	cThread* loggingThread = new cThread();
    loggingThread->set(logFileHandler, CHAI_THREAD_PRIORITY_GRAPHICS);
	 

	//while (simulationRunning);


	// start the main graphics rendering loop
	glutMainLoop();

    // close everything
    close();

    // exit
    return (0);
}

//---------------------------------------------------------------------------
void stateMachineHandler()
{
	int prev_STATE_MACHINE = STATE_MACHINE;		// previous state machine state
	int init_STATE_MACHINE = STATE_TRIAL_START;

	SYSTEMTIME last_state_t;
	GetSystemTime(&last_state_t);

	int file_counter = 0;

	int i, j;
	long double value;
	long double sum = 0;
	long double points[100][4][3];


	// start the main code loop 
	while (simulationRunning)
	{
		Sleep(100);

		/////////////////////////////////////////////////////////////////////
		// READ HAPTIC DEVICE and ARDUINO
		/////////////////////////////////////////////////////////////////////
		
		// distance from initial home position
		double distance_traveled = cur_position.distance(homePosition);

		double movementThreshold = 0.010;					// meters
		double elapsedTime = elapsedTimeSec(last_state_t);	// seconds
		double maxMovementTime = 30;						// seconds


		std::string line;
		std::string fileName;
		std::string baseFileName = "test_";
		std::string fileEnding = ".txt";
		std::string fileNumber;          // string which will contain the result
		ostringstream convert;   // stream used for the conversion

		convert << file_counter;      // insert the textual representation of 'Number' in the characters in the stream
		fileNumber = convert.str(); // set 'Result' to the contents of the stream

		fileName = baseFileName + fileNumber + fileEnding;
		
		ifstream myfile (fileName);
		
		// reward success
		bool success = false;

		init_STATE_MACHINE = STATE_MACHINE;


		/////////////////////////////////////////////////////////////////////
		// STATE MACHINE EXECUTION
		/////////////////////////////////////////////////////////////////////

		switch (STATE_MACHINE)
		{

			case STATE_TRIAL_START:
				useTopStiffSpring	= true;
				useDamping			= false;
				GLOBAL_FLAG_LOG_DATA = true;

				if (elapsedTime > 2) {
					STATE_MACHINE = STATE_CALC_TRAJ;
					GLOBAL_FLAG_LOG_DATA = false;
					mm_timer.start(true);
				}
				break;

			//case STATE_FOLLOW_CIRCLE:
			//	useFollower	= true;
			//	useTopStiffSpring	= false;
			//	useDamping			= false;
			//	GLOBAL_FLAG_LOG_DATA = true;

			//	if (elapsedTime > 5) {
			//		STATE_MACHINE = STATE_LOG_DATA;
			//		GLOBAL_FLAG_LOG_DATA = false;
			//		useFollower	= false;
			//	}
			//	break;

			case STATE_CALC_TRAJ:
				useTopStiffSpring	= false;
				useDamping			= false;

				// % read next trajectory file
				i=0;
				while (std::getline(myfile, line))
				{
					std::istringstream iss(line);

					j=0;
					while (iss >> value) 
					{ 
						points[i][j/3][j%3] =value;
						std::string message = std::to_string(points[i][j/3][j%3]);
						std::cout << message;
						std::cout << " ";
						j++;
					}

					std::cout << "\n";
					i++;
				}

				//if (myfile.is_open())
				//{
				//	cout << fileName << "\n";
				//	while ( getline (myfile,line) )
				//	{
				//		cout << line << '\n';
				//	}
				//	myfile.close();
				//	file_counter++;
				//}
				//else 
				//{
				//	cout << "Unable to open file"; 
				//}

				myfile.close();
				file_counter++;

				STATE_MACHINE = STATE_FOLLOW_TRAJ;
				break;

			case STATE_FOLLOW_TRAJ:

				if (elapsedTime > 5) {
					STATE_MACHINE = STATE_LOG_DATA;
					GLOBAL_FLAG_LOG_DATA = false;
					usePoints		    = false;
				}
				break;

			case STATE_LOG_DATA:
				useTopStiffSpring = false;


				if (elapsedTime > 1) {
					STATE_MACHINE = STATE_LOGGING;

					GLOBAL_FLAG_WRITE_LOG_TO_DATA = true;

				}
				break;

			// *************************************************************************
			//
			case STATE_LOGGING:
				// TODO - LOG TRIAL TO *.TXT FILE 
				// TODO - UPDATE GUI

				// Wait for logger to complete
				if(GLOBAL_FLAG_WRITE_LOG_TO_DATA == false)
				{
					STATE_MACHINE = STATE_TRIAL_START;
				}
				break;

			default:
				STATE_MACHINE = STATE_MACHINE;
				break;
		}

		// Update timeStamp upon switching states
		if (init_STATE_MACHINE != STATE_MACHINE)	
		{
			GetSystemTime(&last_state_t);
			printf("STATE: %d\n", STATE_MACHINE);
			prev_STATE_MACHINE = init_STATE_MACHINE;
		}

	}
}

//---------------------------------------------------------------------------
void logFileHandler()
{
	int TIME_TO_FLUSH = 50;	// msec
	int time_stamp_last_flush = 0;
	SYSTEMTIME last_flush_t;
	GetSystemTime(&last_flush_t);

	// start the main code loop 
	while (simulationRunning)
	{
		Sleep(1);

		if (GLOBAL_FLAG_LOG_DATA)
		{
			bool log_status = loggerObj.logVariables(-1, 
													 STATE_MACHINE, 
													 GLOBAL_FLAG_ardin_delieverReward, 
													 ardout_capSen_sipperTube, 
													 ardout_capSen_handleOutside, 
													 cur_position, 
													 cur_linearVelocity, 
													 cur_force);
			if (log_status == false)
				Console::WriteLine("Log File Error!!!");
		}

		if (GLOBAL_FLAG_WRITE_LOG_TO_DATA)
		{
			loggerObj.writeLogToFile();			

			GLOBAL_FLAG_WRITE_LOG_TO_DATA = false;
		}
		else if(elapsedTimeMilliSec(last_flush_t) > TIME_TO_FLUSH)
		{
			loggerObj.flushLogToFile();
			GetSystemTime(&last_flush_t);
		}
	}
}

//---------------------------------------------------------------------------
void serialHandler()
{
	// Initialize Serial Ports
	SerialPort^ ard_serialPort = initialization_ardunioPort();
	GLOBAL_FLAG_ard_serialPort_initialized = true;

	//SerialPort^ gui_serialPort = initialization_pyGUI();
	//GLOBAL_FLAG_gui_serialPort_initialized = true;


	// Wait until training begins
	while (!simulationRunning)
	{
		Sleep(100);
	}

	// Clear Arduino Serial Buffer
	ard_serialPort->ReadExisting();
	//ard_serialPort->ReadLine();

	// start the serial thread processing loop
	while (simulationRunning)
	{
		ard_serialThread(ard_serialPort);
		//gui_serialThread(gui_serialPort);
 
		Sleep(100);
	}

	// Program is ended
	//if (gui_serialPortConnected)
	//{
	//	gui_serialPort->Write("-9999\n");
	//}
}

//---------------------------------------------------------------------------
void ard_serialThread(SerialPort^ _serialPort)
{
	array<String^>^ ard_outputs= gcnew array< String^ >(100);
	int pcks_rx_ind = 0;

	try {
		while (_serialPort->BytesToRead > 1)
		{
			ard_outputs[pcks_rx_ind] = _serialPort->ReadLine();
			if (pcks_rx_ind < 100)
				pcks_rx_ind	+= 1;
		}
	}
	catch (TimeoutException ^) { 
		// No more packets to read
	}
		
	if (pcks_rx_ind != 0)
	{
		String^ delimStr = ",";
		array<Char>^ delimiter = delimStr->ToCharArray( );
		array<String^>^ ard_vars;
		
		String^ toParse = ard_outputs[pcks_rx_ind-1];
		ard_vars = toParse->Split( delimiter );

		if (ard_vars->Length > 2)
		{
			ardout_buttonPressed		= Convert::ToDouble(ard_vars[0]);
			ardout_capSen_sipperTube	= Convert::ToDouble(ard_vars[1]);
			ardout_capSen_handleOutside = Convert::ToDouble(ard_vars[2]);
		}		
	}

	//Console::WriteLine("Number of Packets RX: " + pcks_rx_ind);

	if (GLOBAL_FLAG_ardin_delieverReward)
	{
		if (GLOBAL_FLAG_target_left) _serialPort->Write("H_L\n");
		if (!GLOBAL_FLAG_target_left) _serialPort->Write("H_R\n");

		GLOBAL_FLAG_ardin_delieverReward = false;
	}
}

//---------------------------------------------------------------------------
void gui_serialThread(SerialPort^ _serialPort)
{
	if (gui_serialPortConnected)
	{
		_serialPort->Write(ardout_capSen_sipperTube + "\n");
	}
}

//---------------------------------------------------------------------------
void keyboardListener()
{
	while (simulationRunning)
	{
		String^ k_input;
		k_input = Console::ReadLine();

		if (k_input != nullptr) 
		{
			if (k_input == "x")
			{
				//simulationRunning = false;
				close();
			}
		}

		Sleep(200);
	}
}

//---------------------------------------------------------------------------
void close()
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close all haptic devices
    int i=0;
    while (i < numHapticDevices)
    {
        hapticDevices[i]->close();
        i++;
    }

	// Close Logger File
	loggerObj.closeFile();

	Console::WriteLine("Exiting Training Program");
}

////---------------------------------------------------------------------------
void updateHaptics()
{
	const cVector3d origin = homePosition;		// User Coordinate Space Origin 
	cVector3d integratedErrorVector (0,0,0);	// integrator variable
	
	//time_t timer;

    // main haptic simulation loop
    while(simulationRunning)
    {
        // for each device
        int i=0;
        while (i < numHapticDevices)
        {
            // read position of haptic device
            cVector3d newPosition;
			hapticDevices[i]->getPosition(newPosition);
			// Find current position in user coordinate space
			cVector3d homeCoord_newPos = newPosition - origin;
			cVector3d dist_out_sphere;
			cVector3d dir_cur_pos;

            // read linear velocity from device
            cVector3d newLinearVelocity;
            hapticDevices[i]->getLinearVelocity(newLinearVelocity);
		
            // read user button status
            bool buttonStatus;
            hapticDevices[i]->getUserSwitch(0, buttonStatus);


			///////////////////////////////////////////////////////////////////////
			//// CONTINUOUS HAPTIC FORCES
			///////////////////////////////////////////////////////////////////////

            // compute a reaction force
            cVector3d newForce (0,0,0);

			// Antigravity Force Vector
			newForce.add(cVector3d(0, 0, 0.0));

			//// Build Arrest Ceiling
			//if (homeCoord_newPos.z > 0)
			//	newForce.add(cVector3d(0,0,-homeCoord_newPos.z*1000));
			if (homeCoord_newPos.length() > 0.03){
				dir_cur_pos=homeCoord_newPos;
				dir_cur_pos.normalize();
				dir_cur_pos.mul(.03);
				dist_out_sphere=homeCoord_newPos;
				dist_out_sphere.sub(dir_cur_pos);
				dist_out_sphere.mul(-1000);
				newForce.add(dist_out_sphere);
			}

			if (newLinearVelocity.length()>.15)
            {
                //cHapticDeviceInfo info = hapticDevices[i]->getSpecifications();
				//double Kv = info.m_maxLinearDamping;
                double Kv = 200*(newLinearVelocity.length()-.15);		
                cVector3d dampingForce = cMul(-Kv, newLinearVelocity);
                newForce.add(dampingForce);
            }
						// apply viscosity
			if (useFollower)
            {
				double Kf=700;
				double f=2;
				//time(&timer);
				double timer = mm_timer.getCurrentTimeSeconds();
				double target_x = 0.01*cCosRad(timer*2*3.14*.31*f);
				double target_y = 0.01*cCosRad(timer*2*3.14*f);	
				double target_z = 0.01*cSinRad(timer*2*3.14*f);	
				cVector3d targetVector;
				targetVector.set(target_x,target_y,target_z);
				cVector3d followError = cSub(newPosition,targetVector);
                cVector3d followForce = cMul(-Kf, followError);
                newForce.add(followForce);
            }

			//// Build Arrest Walls
			//if (homeCoord_newPos.x > 0)
			//	newForce.add(cVector3d(-homeCoord_newPos.x*1000,0,0));

			//// Build Arrest Sidewall
			////if (homeCoord_newPos.y > 0)
			////	newForce.add(cVector3d(0,-homeCoord_newPos.y*1000,0));
			//if (homeCoord_newPos.y < 0)
			//	newForce.add(cVector3d(0,-homeCoord_newPos.y*1000,0));


			///////////////////////////////////////////////////////////////////////
			//// VARIABLE HAPTIC FORCES
			///////////////////////////////////////////////////////////////////////			
			
			//// apply force field
			//cVector3d trialForce = trialType.generateHapticForce(newPosition, newLinearVelocity);
   //         if (useTrialHaptics)
			//{				
			//	newForce.add(trialForce);
			//}

			// apply force field with PID controller
            if (useTopStiffSpring)
            {
				cVector3d errorVector = origin - newPosition;

				// Position
                double Kp = 300; // [N/m]
                cVector3d springForce = cMul(Kp, errorVector);
                newForce.add(springForce);

				// Intergrator
				integratedErrorVector.add(errorVector);

				double Ki = 1; // [N/m]
				cVector3d integratorForce = cMul(Ki, integratedErrorVector);
                newForce.add(integratorForce);

				// Velocity
				double Kv = 30;		
                cVector3d dampingForce = cMul(-Kv, newLinearVelocity);
                newForce.add(dampingForce);
            }
			else
				integratedErrorVector = cVector3d(0,0,0);

            // apply force field
            if (useLockHandle)
            {
                double Kp = 1000; // [N/m]
                cVector3d springForce = cMul(-Kp, newPosition - lock_position);
                newForce.add(springForce);

				double Kv = 20;		
                cVector3d dampingForce = cMul(-Kv, newLinearVelocity);
                newForce.add(dampingForce);
            }
       
			// apply viscosity
			if (useDamping)
            {
                //cHapticDeviceInfo info = hapticDevices[i]->getSpecifications();
				//double Kv = info.m_maxLinearDamping;
                double Kv = 5;		
                cVector3d dampingForce = cMul(-Kv, newLinearVelocity);
                newForce.add(dampingForce);
            }

            // apply 

			// send computed force to haptic device
            hapticDevices[i]->setForce(newForce);

			// Update Global Variables
			cur_position			= newPosition;
			cur_linearVelocity		= newLinearVelocity;
			cur_force				= newForce;
			GLOBAL_FLAG_target_left = cur_position.y < 0;

            // increment counter
            i++;
        }
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//---------------------------------------------------------------------------
void updateChai3DGraphics(void)
{
	guiObj_cursor->setPos(cur_position);
	//guiObj_clstPnt->setPos(verbose_Vector2);
	//guiObj_projFromClstPnt->setPos(verbose_Vector3);

	//guiObj_segmentForce->m_pointA = verbose_Vector;
    //guiObj_segmentForce->m_pointB = cAdd(verbose_Vector, verbose_Vector4);

	if (usePoints){
		double Kf=700;
		double f=2;
		//time(&timer);
		double timer = mm_timer.getCurrentTimeSeconds();
		double target_x = 0.01*cCosRad(timer*2*3.14*.31*f);
		double target_y = 0.01*cCosRad(timer*2*3.14*f);	
		double target_z = 0.01*cSinRad(timer*2*3.14*f);	
		cVector3d targetVector;
		targetVector.set(target_x,target_y,target_z);

		guiObj_point1->setPos(targetVector);
	}


    // render world
	camera->renderView(displayW, displayH);

    // Swap buffers
    glutSwapBuffers();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    // inform the GLUT window to call updateGraphics again (next frame)
    if (simulationRunning)
    {
        glutPostRedisplay();
    }
}

//---------------------------------------------------------------------------
void updateGraphics()
{
	string filename = "..\\..\\trainerBot\\python_codebase\\trainer_gui.py";
	string command = "python ";
	string args = " 10";
	command += filename;
	command += args;
	system(command.c_str());

	gui_serialPortConnected = false;

	//printf("%s\n", returnCurrentPath().c_str());
	//printf("Calling Python to find the sum of 2 and 2.\n");
	//// Initialize the Python interpreter.
	//Py_Initialize();
	//// Create some Python objects that will later be assigned values.
	//PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pValue;
	//PyObject* sysPath = PySys_GetObject((char*)"path");
	//PyList_Append(sysPath, PyString_FromString("..\\..\\trainerBot\\python_codebase"));

	//// Convert the file name to a Python string.
	//pName = PyString_FromString("trainer_gui");
	//// Import the file as a Python module.
	//pModule = PyImport_Import(pName);
	//// Create a dictionary for the contents of the module.
	//pDict = PyModule_GetDict(pModule);
	//// Get the add method from the dictionary.
	//pFunc = PyDict_GetItemString(pDict, "main");
	//// Create a Python tuple to hold the arguments to the method.
	//pArgs = PyTuple_New(0);
	//// Convert 2 to a Python integer.
	////pValue = PyInt_FromLong(2);
	//// Set the Python int as the first and second arguments to the method.
	////PyTuple_SetItem(pArgs, 0, pValue);
	////PyTuple_SetItem(pArgs, 1, pValue);
	//// Call the function with the arguments.
	//PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
	//// Print a message if calling the method failed.
	//if(pResult == NULL)
	//	printf("Calling the add method failed.\n");
	//// Convert the result to a long from a Python object.
	//long result = PyInt_AsLong(pResult);
	//// Destroy the Python interpreter.
	//Py_Finalize();
	//// Print the result.
	//printf("The result is %d.\n", result); 
	//std::cin.ignore(); 
	//return 0;
}


//-----------------------------------------------------------------------
// HELPER FUNCTIONS
//-----------------------------------------------------------------------

//---------------------------------------------------------------------------
double elapsedTimeMilliSec(SYSTEMTIME start_t)
{
	SYSTEMTIME cur_t;
	
	// or GetLocalTime(&t)
	GetSystemTime(&cur_t);
	
	double cur_msec, start_msec, diff_msec;		// seconds
	cur_msec = (cur_t.wHour*60*60 + cur_t.wMinute*60 + cur_t.wSecond)*1000 + (cur_t.wMilliseconds);
	start_msec = (start_t.wHour*60*60 + start_t.wMinute*60 + start_t.wSecond)*1000 + (start_t.wMilliseconds);
	diff_msec = cur_msec - start_msec;

	return diff_msec;
}

//---------------------------------------------------------------------------
double elapsedTimeSec(SYSTEMTIME start_t)
{
	return elapsedTimeMilliSec(start_t)/1000;
}

//---------------------------------------------------------------------------
string returnCurrentPath()
{
	char ownPth[MAX_PATH]; 

	// Will contain exe path
	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		// When passing NULL to GetModuleHandle, it returns handle of exe itself
		GetModuleFileName(hModule, ownPth, (sizeof(ownPth))); 

		// Use above module handle to get the path using GetModuleFileName()
		//cout << ownPth << endl ;
		//std::cin.ignore();
		//return 0;
		string copy = ownPth;
		return copy;
	}
	else
	{
		//cout << "Module handle is NULL" << endl ;
		//std::cin.ignore();
		//return 0;
		return NULL;
	}
}