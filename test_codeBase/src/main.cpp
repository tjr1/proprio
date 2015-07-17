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
//#include "logger.h"


//---------------------------------------------------------------------------
// DECLARED NAMESPACES
//---------------------------------------------------------------------------
using namespace std;
using namespace System;
using namespace System::IO::Ports;
//---------------------------------------------------------------------------

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

// Home Position (in/out, left/right, up/down)
const cVector3d homePosition (-0.00, 0.0, -0.026); // -0.016 for right


//---------------------------------------------------------------------------
// DECLARED GLOBAL VARIABLES
//---------------------------------------------------------------------------

// a haptic device handler
cHapticDeviceHandler* handler;

// a table containing pointers to all haptic devices detected on this computer
cGenericHapticDevice* hapticDevices[MAX_DEVICES];

// number of haptic devices detected
int numHapticDevices = 0;

// status of the main simulation haptics loop
bool simulationRunning = false;
bool simulationFinished = false;

// root resource path
string resourceRoot;

// Haptic Forces 
bool useTwoChannels		= false;	// Channeled Movement 
bool useLeftChannel		= false;
bool useRightChannel	= false;
bool useDamping			= false;	// damping mode ON/OFF
bool useTopStiffSpring	= false;	// force field mode ON/OFF
bool useAccelerant		= false;	// Helper force to depress lever
bool useDownSpring		= false;
bool useOutSpring		= false;
bool useSegment			= false;

// Falcon Variables
cVector3d cur_position;
cVector3d cur_linearVelocity;
cVector3d cur_force;

// Verbose
double verbose_double;
cVector3d verbose_Vector;
cVector3d verbose_Vector2;
cVector3d verbose_Vector3;
cVector3d verbose_Vector4;

// Keyboard Response from User
string keyboardString;


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
cShapeSphere* guiObj_clstPnt;
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


//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// timer
double elapsedTimeMilliSec(SYSTEMTIME start_t);

// timer
double elapsedTimeSec(SYSTEMTIME start_t);

// callback when a keyboard key is pressed
void keyboardListener(void);

// function called before exiting the application
void close(void);

// main haptics loop
void updateHaptics(void);

void mainLoopListener(void);

void updateGraphics(void);

void graphicsHandler(void);


//===========================================================================
/*
    DEMO:    device.cpp

    This application illustrates the use of the haptic device handler
    "cHapticDevicehandler" to access all of the haptic devices
    "cGenericHapticDevice" connected to the computer.

    In this example the application opens an OpenGL window and displays a
    3D cursor for each device. Each cursor (sphere + reference frame)
    represents the position and orientation of its respective device.
    If the operator presses the device user button (if available), the color
    of the cursor changes accordingly.

    In the main haptics loop function  "updateHaptics()" , the position,
    orientation and user switch status of each device are retrieved at
    each simulation iteration. The information is then used to update the
    position, orientation and color of the cursor. A force is then commanded
    to the haptic device to attract the end-effector towards the device origin.
*/
//===========================================================================

int main(int argc, char* argv[])
{

    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    printf ("\n");
    printf ("-----------------------------------\n");
    printf ("Automated Trainer\n");
    printf ("Moritz Lab\n");
    printf ("Copyright 2014\n");
	printf ("David Bjanes (dbjanes@uw.edu)\n");
	printf ("Tom Richner (tomrichner@gmail.com)\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");
    printf ("Keyboard Options:\n\n");
    printf ("[1] - Start Training\n");
    printf ("[2] - Calibration Falcon\n");
	printf ("[3] - Setup Arduino\n");
	printf ("[...] - ...\n");
    printf ("[x] - Exit application\n");
    printf ("\n\n");

    // parse first arg to try and locate resources
    resourceRoot = string(argv[0]).substr(0,string(argv[0]).find_last_of("/\\")+1);

	//-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------

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

	// create a cursor by setting its radius
    guiObj_clstPnt = new cShapeSphere(0.01);
	guiObj_clstPnt->setPos(cVector3d(0,0,0));
    // add cursor to the world
    world->addChild(guiObj_clstPnt);

	// create a small line to illustrate velocity
    guiObj_projFromClstPnt = new cShapeSphere(0.01);
	guiObj_projFromClstPnt->setPos(cVector3d(0,0,0));
    // add line to the world
    world->addChild(guiObj_projFromClstPnt);


	guiObj_segmentForce = new cShapeLine(cVector3d(0,0,0), cVector3d(0,0,0));
	world->addChild(guiObj_segmentForce);

    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------

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


		// read initial position of device to initialize communication
		Console::WriteLine("Falcon Calibration -------------------------------- ");

		cVector3d pos;
		hapticDevices[i]->getPosition(pos);
				
		hapticDevices[i]->setForce(cVector3d(2,0,0));
		Console::WriteLine("Move the Falcon Around... Press Enter");
		Console::ReadLine();
		
		hapticDevices[i]->setForce(cVector3d(0,2,0));
		Console::WriteLine("Move the Falcon Around... Press Enter");
		Console::ReadLine();

		hapticDevices[i]->setForce(cVector3d(0,0,2));
		Console::WriteLine("Move the Falcon Around... Press Enter");
		Console::ReadLine();

		hapticDevices[i]->setForce(cVector3d(0,0,0));
		Console::WriteLine("Falcon Calibration Complete ----------------------- \n\n");


        // retrieve information about the current haptic device
        cHapticDeviceInfo info = newHapticDevice->getSpecifications();

        // increment counter
        i++;
    }


	//-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------

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
    glutDisplayFunc(updateGraphics);
    //glutKeyboardFunc(keySelect);
    //glutReshapeFunc(resizeWindow);
    glutSetWindowTitle("CHAI 3D");

    // create a mouse menu (right button)
    //glutCreateMenu(menuSelect);
    //glutAddMenuEntry("full screen", OPTION_FULLSCREEN);
    //glutAddMenuEntry("window display", OPTION_WINDOWDISPLAY);
    //glutAttachMenu(GLUT_RIGHT_BUTTON);


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
    cThread* otherThread = new cThread();
    otherThread->set(mainLoopListener, CHAI_THREAD_PRIORITY_GRAPHICS);

	// create a thread which starts the main haptics rendering loop
    cThread* graphicsThread = new cThread();
    graphicsThread->set(graphicsHandler, CHAI_THREAD_PRIORITY_GRAPHICS);


	// start the main graphics rendering loop
	glutMainLoop();

	while (simulationRunning);

    // close everything
    close();

    // exit
    return (0);
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
				simulationRunning = false;
				break;
			}

			if (k_input == "1")
				useTwoChannels = !useTwoChannels;

			if (k_input == "2")
				useLeftChannel = !useLeftChannel;

			if (k_input == "3")
				useRightChannel = !useRightChannel;

			if (k_input == "4")
				useDamping = !useDamping;

			if (k_input == "5")
				useAccelerant = !useAccelerant;

			if (k_input == "6")
				useSegment = !useSegment;

			if (k_input == "7")
				useTopStiffSpring = !useTopStiffSpring;
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

	Console::WriteLine("Exiting Training Program");
}

//---------------------------------------------------------------------------
void mainLoopListener(void)
{
	useTwoChannels		= false;	// Channeled Movement 
	useLeftChannel		= false;
	useRightChannel		= false;
	useDamping			= true;		// damping mode ON/OFF
	useTopStiffSpring	= false;	// force field mode ON/OFF
	useAccelerant		= false;	// Helper force to depress lever
	useDownSpring		= false;
	useOutSpring		= false;
	useSegment			= true;

	// start the serial thread processing loop
	while (simulationRunning)
	{ 
		double a = (float)((int)(verbose_double*1000000))/1000;
		double b = (float)((int)(verbose_Vector.y*1000000))/1000;
		double c = (float)((int)(verbose_Vector.z*1000000))/1000;
		double bb = (float)((int)(verbose_Vector2.y*1000000))/1000;
		double cc = (float)((int)(verbose_Vector2.z*1000000))/1000;

		//Console::WriteLine("Dist: " + a + "|| Position: " + b + " " + c + "|| ClstPnt: " + bb + " " + cc);

		Sleep(100);
	}
}

// graphics Thread
void graphicsHandler(void)
{
	// start the main graphics rendering loop
    //glutMainLoop();
}

//---------------------------------------------------------------------------
void updateGraphics(void)
{
	guiObj_cursor->setPos(verbose_Vector);
	guiObj_clstPnt->setPos(verbose_Vector2);
	guiObj_projFromClstPnt->setPos(verbose_Vector3);

	guiObj_segmentForce->m_pointA = verbose_Vector;
    guiObj_segmentForce->m_pointB = cAdd(verbose_Vector, verbose_Vector4);

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
void updateHaptics()
{
	// User Coordinate Space Origin 
	const cVector3d gblCoord_usrOrigin (0.00, 0.0, -0.026); // -0.016 for right
	const cVector3d gblCoord_origin (0,0,0);

	// Constrain along segment
	cVector3d usrCoord_home (0, 0, 0);					//[m]
	cVector3d usrCoord_target_L_90 (0, -0.025, +0.000);	//[m]
	cVector3d usrCoord_target_L_45 (0, -0.020, -0.020);	//[m]
	cVector3d usrCoord_target_BOTT (0, +0.000, -0.025);	//[m]
	cVector3d usrCoord_target_R_45 (0, +0.020, -0.020);	//[m]
	cVector3d usrCoord_target_R_90 (0, +0.025, +0.000);	//[m]

	// Get segment directional vector 
	cVector3d gblCoord_home        = usrCoord_home        + gblCoord_usrOrigin; //[m]
	cVector3d gblCoord_target_L_90 = usrCoord_target_L_90 + gblCoord_usrOrigin;	//[m]
	cVector3d gblCoord_target_L_45 = usrCoord_target_L_45 + gblCoord_usrOrigin;	//[m]
	cVector3d gblCoord_target_BOTT = usrCoord_target_BOTT + gblCoord_usrOrigin;	//[m]
	cVector3d gblCoord_target_R_45 = usrCoord_target_R_45 + gblCoord_usrOrigin;	//[m]
	cVector3d gblCoord_target_R_90 = usrCoord_target_R_90 + gblCoord_usrOrigin;	//[m]
				
	// Segment Definition
	int num_segments = 5;
	cSegment3d* gblCoord_segment_list[5];
	gblCoord_segment_list[0] = new cSegment3d (gblCoord_home, gblCoord_target_L_90);
	gblCoord_segment_list[1] = new cSegment3d (gblCoord_home, gblCoord_target_L_45);
	gblCoord_segment_list[2] = new cSegment3d (gblCoord_home, gblCoord_target_BOTT);
	gblCoord_segment_list[3] = new cSegment3d (gblCoord_home, gblCoord_target_R_45);
	gblCoord_segment_list[4] = new cSegment3d (gblCoord_home, gblCoord_target_R_90);
	double segment_width = 1.0;			//[mm]

	// Center Sphere Radius
	double sph_radius = 2.5;			//[mm]


    // main haptic simulation loop
    while(simulationRunning)
    {
        // for each device
        int i=0;
        while (i < numHapticDevices)
        {
            // read position of haptic device
            cVector3d gblCoord_newPos;
			hapticDevices[i]->getPosition(gblCoord_newPos);
			// Find current position in user coordinate space
			cVector3d usrCoord_newPos = gblCoord_newPos - gblCoord_usrOrigin;

            // read orientation of haptic device
            //cMatrix3d newRotation;
            //hapticDevices[i]->getRotation(newRotation);

            // read linear velocity from device
            cVector3d newlinearVelocity;
            hapticDevices[i]->getLinearVelocity(newlinearVelocity);
		
            // read user button status
            bool buttonStatus;
            hapticDevices[i]->getUserSwitch(0, buttonStatus);


            // compute a reaction force
            cVector3d newForce (0,0,0);

			// Antigravity Force Vector
			newForce.add(cVector3d(0, 0, 0.0));

			// Build Arrest Ceiling
			if (usrCoord_newPos.z > 0)
				newForce.add(cVector3d(0,0,-usrCoord_newPos.z*500));
			
			// Build Arrest Walls
			newForce.add(cVector3d(-usrCoord_newPos.x*1000,0,0));

			
			// Calculate Segment Variables
			double a_t[5];
			cVector3d a_closestPoint[5];
			double dist_to_seg_n[5] = {9999, 9999, 9999, 9999, 9999};
			int maxIndex = 0;
				
			// Find Closest Segment
			for (int i = 1; i < 5; i+=2)
			{
				double sq_dist_to_seg = gblCoord_segment_list[i]->distanceSquaredToPoint(gblCoord_newPos, a_t[i], &a_closestPoint[i]);		//[m^2]
				dist_to_seg_n[i] = sqrt(sq_dist_to_seg);			//[m]
				if (dist_to_seg_n[maxIndex] > dist_to_seg_n[i])
					maxIndex = i;
			}				
							
			cSegment3d* gblCoord_segment = gblCoord_segment_list[maxIndex];
			double dist_along_segment = a_t[maxIndex];
			cVector3d gblCoord_clstPnt = a_closestPoint[maxIndex];

			double dist_to_seg = dist_to_seg_n[maxIndex];
			double rnd_dist_to_cntOfSeg = (float)((int)(dist_to_seg*1000000))/1000;		//[mm]
			double rnd_dist_to_WallOfSeg = rnd_dist_to_cntOfSeg - segment_width;		//[mm]
				

			// Direction of segment wall restoration force
			cVector3d gblCoord_segmentWallForce;
			cVector3d gblCoord_segmentGlidingForce;
				
			// Deliever (Positional) Proportional Orthogonal Force
				// Get direction of orthogonal force of segment wall 
				cVector3d gblCoord_clstPnt_ForceVector = gblCoord_clstPnt - gblCoord_newPos;
				cVector3d gblCoord_norm_clstPnt_ForceVector = gblCoord_clstPnt_ForceVector;
				gblCoord_norm_clstPnt_ForceVector.normalize();

				// Set Segment Force
				gblCoord_segmentWallForce = gblCoord_norm_clstPnt_ForceVector;


			// Deliever (Velocity) Proportional inbetween Orthogonal and Tangential (to wall) Force
				// Get Projection of Linear Velocity Vector onto segment
				cVector3d gblCoord_segmentVector = gblCoord_segment->m_end - gblCoord_segment->m_start;

				cVector3d gblCoord_newVelocity = newlinearVelocity;
				cVector3d gblCoord_prjOnSeg = gblCoord_newVelocity.dot(gblCoord_segmentVector) * gblCoord_segmentVector 
																/ gblCoord_segmentVector.lengthsq();

				// Add projection point to gbl closest point on segment
				cVector3d gblCoord_projClstPnt = gblCoord_clstPnt + gblCoord_prjOnSeg;
				double epsilon = 0.01;
				if (dist_along_segment < epsilon)
					if (gblCoord_segment->m_end.distance(gblCoord_projClstPnt) > gblCoord_segment->m_end.distance(gblCoord_clstPnt))
						gblCoord_projClstPnt = gblCoord_segment->m_start;

				else if (dist_along_segment > 1-epsilon)
					if (gblCoord_segment->m_start.distance(gblCoord_projClstPnt) > gblCoord_segment->m_start.distance(gblCoord_clstPnt))
						gblCoord_projClstPnt = gblCoord_segment->m_end;

				// compute force vector from new position to proj point
				cVector3d gblCoord_projPnt_ForceVector = gblCoord_projClstPnt - gblCoord_newPos;
					
				// Set Segment Force
				gblCoord_segmentGlidingForce = gblCoord_projPnt_ForceVector;
								

			// Distance in Center "free space" Sphere
			double dist_from_sph_center = gblCoord_usrOrigin.distance(gblCoord_newPos);				//[m]
			double rnd_dist_from_sph_center = (float)((int)(dist_from_sph_center*1000000))/1000;	//[mm]
			double rnd_dist_from_sph_edge = rnd_dist_from_sph_center - sph_radius;
			cVector3d gblCoord_sphVector = gblCoord_newPos - gblCoord_usrOrigin;
				
			verbose_double = maxIndex;
			verbose_Vector = gblCoord_newPos;
			verbose_Vector2 = gblCoord_clstPnt;
			verbose_Vector3 = gblCoord_projClstPnt;

			// apply force field
            if (useSegment) 
			{
				// Initialize Forces
				cVector3d segmentForce (0, 0, 0);
				cVector3d assistiveChannelForce (0, 0, 0);
				cVector3d sphForce (0,0,0);
				
				if (rnd_dist_from_sph_edge > 0)
				{
					if (rnd_dist_from_sph_edge < rnd_dist_to_WallOfSeg)
					{
						double Kp = 500; // [N/m]
						sphForce = cMul(-Kp, gblCoord_sphVector);
						newForce.add(sphForce);
					}
					else if (rnd_dist_to_WallOfSeg > 0)
					{
						double Kp = 0.5;
						double K_1 = min(exp(Kp * (rnd_dist_to_WallOfSeg))-1, 1000000);			
						segmentForce = cMul(K_1, gblCoord_segmentWallForce);

						// Add Force
						newForce.add(segmentForce);
					}
				}

				if (rnd_dist_to_cntOfSeg > 0) 
				{
					double max_distance = 10;	// [mm]
					double Kp = 1;
					double Kv = 15;
					double K_2 = Kv + max(Kp * (max_distance - rnd_dist_to_cntOfSeg), 0);
					assistiveChannelForce = cMul(K_2, gblCoord_segmentGlidingForce);

					// Add Force
					newForce.add(assistiveChannelForce);
				}
				
				verbose_Vector4 = assistiveChannelForce;
			}

			// apply force field
            if (useTwoChannels)
            {
				cVector3d channelForce (0,0,0);
				double Kp = 100;

				// Horizontal Forces
				if (usrCoord_newPos.y > 0)
					channelForce.add(cMul(Kp*-usrCoord_newPos.z, cVector3d(0,1,0)));
				
				if (usrCoord_newPos.y < 0)
					channelForce.add(cMul(Kp*-usrCoord_newPos.z, cVector3d(0,-1,0)));

				// Downward Forces
				if (usrCoord_newPos.y > 0)
					channelForce.add(cMul(Kp*usrCoord_newPos.y, cVector3d(0,0,-1)));
				
				if (usrCoord_newPos.y < 0)
					channelForce.add(cMul(Kp*-usrCoord_newPos.y, cVector3d(0,0,-1)));

				// Add Force
				newForce.add(channelForce);
            }

			// Push Left
            if (useLeftChannel)
            {
				cVector3d leftChannelForce (0,0,0);
				double Kp = 100;

				// Horizontal Forces
				leftChannelForce.add(cMul(Kp*-usrCoord_newPos.z, cVector3d(0,-1,0)));

				// Downward Forces
				if (usrCoord_newPos.y > 0)
					leftChannelForce.add(cMul(Kp*usrCoord_newPos.y, cVector3d(0,0,-1)));
				
				if (usrCoord_newPos.y < 0)
					leftChannelForce.add(cMul(Kp*-usrCoord_newPos.y, cVector3d(0,0,-1)));	

				// Add Force
				newForce.add(leftChannelForce);
            }

			// Push Right
            if (useRightChannel)
            {
				cVector3d rightChannelForce (0,0,0);
				double Kp = 100;

				// Horizontal Forces
				rightChannelForce.add(cMul(Kp*-usrCoord_newPos.z, cVector3d(0,1,0)));

				// Downward Forces
				if (usrCoord_newPos.y > 0)
					rightChannelForce.add(cMul(Kp*usrCoord_newPos.y, cVector3d(0,0,-1)));
				
				if (usrCoord_newPos.y < 0)
					rightChannelForce.add(cMul(Kp*-usrCoord_newPos.y, cVector3d(0,0,-1)));

				// Add Force
				newForce.add(rightChannelForce);
            }

            // apply force field
            if (useTopStiffSpring)
            {
                double Kp = 500; // [N/m]
                cVector3d springForce = cMul(-Kp, gblCoord_newPos-homePosition);
                newForce.add(springForce);
            }

			// apply force field
   //         if (useDownSpring)
   //         {
   //             double Kp = 500; // [N/m]
   //             cVector3d downForce = cMul(-Kp, gblCoord_newPos-downPosition);
   //             newForce.add(downForce);
   //         }

			//// apply force field
   //         if (useOutSpring)
   //         {
   //             double Kp = 500; // [N/m]
   //             cVector3d outForce = cMul(-Kp, gblCoord_newPos-outPosition);
   //             newForce.add(outForce);
   //         }
        
			// apply viscosity
			if (useDamping)
            {
                //cHapticDeviceInfo info = hapticDevices[i]->getSpecifications();
				//double Kv = info.m_maxLinearDamping;
                double Kv = 20;		
                cVector3d dampingForce = cMul(-Kv, newlinearVelocity);
                newForce.add(dampingForce);
            }

            // apply 
            if (useAccelerant)
            {
				// compute linear damping force
				double Kv = 20;
				cVector3d newlinearVelocityCopy = newlinearVelocity;
				newlinearVelocityCopy.dot(cVector3d(1, 1, 1));
			
				cVector3d accelerantForce = Kv * newlinearVelocityCopy;
				if (accelerantForce.distance(cVector3d(0, 0, 0)) > 1)
				{
					accelerantForce.normalize();
					accelerantForce.mul(1);
				}
				newForce.add(accelerantForce);
            }

			// send computed force to haptic device
            hapticDevices[i]->setForce(newForce);

			// Update Global Variables
			cur_position		= gblCoord_newPos;
			cur_linearVelocity	= newlinearVelocity;
			cur_force			= newForce;

            // increment counter
            i++;
        }
    }
    
    // exit haptics thread
    simulationFinished = true;
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