#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <time.h>
#include <Windows.h>
#include "chai3d.h"

class Experiment
{
public:
	Experiment(void);
	bool getTrialSuccess(int capSens_Sipper, int capSens_Handle);
	cVector3d generateHapticForce(cVector3d newPosition, cVector3d newVelocity);
	cVector3d hapticForce_Segment(cVector3d newPosition, cVector3d newVelocity);
	cVector3d hapticForce_TwoChannel(cVector3d newPosition, cVector3d newVelocity);
	cVector3d hapticForce_LeftChannelOnly(cVector3d newPosition, cVector3d newVelocity);
	cVector3d hapticForce_RightChannelOnly(cVector3d newPosition, cVector3d newVelocity);
	cVector3d hapticForce_PhysWalls(cVector3d newPosition, cVector3d newVelocity);

private:
	void initializeHapticEnvironment(void);
	double elapsedTimeMilliSec(SYSTEMTIME start_t);
	double elapsedTimeSec(SYSTEMTIME start_t);
};


#endif