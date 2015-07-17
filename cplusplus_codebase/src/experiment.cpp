
#include "experiment.h"

//---------------------------------------------------------------------------
// DECLARED NAMESPACES
//---------------------------------------------------------------------------
using namespace std;
using namespace System;

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// Haptic locations
const cVector3d homePosition (0.00, 0.0, -0.023);

// STATES
const int STATE_WAIT_FOR_PRESS		= 1;
const int STATE_RESET_LEVER			= 2;
const int STATE_TRIAL_COMPLETE		= 3;

//---------------------------------------------------------------------------
// DECLARED GLOBAL VARIABLES
//---------------------------------------------------------------------------

bool GLOBALFLAG_INITIALIZED = false;
bool GLOBALFLAG_TRIAL_RUNNING = false;
SYSTEMTIME trial_start_t;

// State Variables
int TRIAL_STATE_MACHINE = 0;
cVector3d CURRENT_POSITION;
cVector3d CURRENT_VELOCITY;
int counter1 = 0;

// Haptic Variables
cSegment3d* segment_list[5];
double segment_width = 1.0;			//[mm]
// Center Sphere Radius
double sph_radius = 2.0;			//[mm]


//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

Experiment::Experiment()
{
	initializeHapticEnvironment();
	GLOBALFLAG_INITIALIZED = true;
}


//---------------------------------------------------------------------------
// PULBIC METHODS
//---------------------------------------------------------------------------

bool Experiment::getTrialSuccess(int capSens_Sipper, int capSens_Handle)
{
	bool success = false;


	if (GLOBALFLAG_TRIAL_RUNNING == false)
	{
		GLOBALFLAG_TRIAL_RUNNING = true;
		GetSystemTime(&trial_start_t);

		TRIAL_STATE_MACHINE = STATE_WAIT_FOR_PRESS;
	}
	
	// Trial Variables
	double elapsedTime = elapsedTimeSec(trial_start_t);						//[seconds]
	double distance_traveled = CURRENT_POSITION.distance(homePosition);		//[meters]
	bool leftTargetSelected = CURRENT_POSITION.y < 0;

	// Trial Constants
	const double movementThreshold = 0.005;					//[meters]
	const double maxMovementTime = 30;						//[seconds]

	switch (TRIAL_STATE_MACHINE)
	{
		case STATE_WAIT_FOR_PRESS:

			//if (capSens_Sipper == true)
			//	success = true;
			
			//else 
			if (capSens_Handle == true)
			{
				// Change this number for #112
				if (counter1 > 10000)
				{
					counter1 = 1;
					success = true;                                     
				}
				else
				{
					counter1 += 1;
				}
			}
			else
				counter1 = 1;
			
			success = success | (distance_traveled > movementThreshold);
			
			break;

		case STATE_RESET_LEVER:
			break;

			//success = (distance_traveled > movementThreshold) | 
			//	      (elapsedTime > maxMovementTime);

			// *************************************************************************
			//case STATE_RESET_LEVER_MID_TRIAL:
			//	useDamping = true;
			//	useTopStiffSpring = true;
			//	useAccelerant = false;
			//	useDownSpring = false;
			//	useOutSpring = false;

			//	useSegment = true;

			//	GLOBAL_FLAG_prev_target_left = GLOBAL_FLAG_target_left;

			//	if (elapsedTime > 0.2) {
			//		STATE_MACHINE = STATE_TRIAL_RUNNING;
			//	}				
			//	break;

		case STATE_TRIAL_COMPLETE:
			break;

			//int num_switch; num_switch = 30;
			//if (STATE_MACHINE_TYPE == TYPE_LEFT) {
			//	if (reward_counter % num_switch == 0) {
			//		STATE_MACHINE_TYPE = TYPE_RIGHT;
			//	}
			//}
			//else if (STATE_MACHINE_TYPE == TYPE_RIGHT) {
			//	if (reward_counter % num_switch == 0) {
			//		STATE_MACHINE_TYPE = TYPE_LEFT;
			//	}
			//}	

		default:
			break;

	}

	if (success)
		GLOBALFLAG_TRIAL_RUNNING = false;

	return success;
}

cVector3d Experiment::generateHapticForce(cVector3d newPosition, cVector3d newVelocity)
{
	cVector3d newForce (0,0,0);

	CURRENT_POSITION = newPosition;
	CURRENT_VELOCITY = newVelocity;

	if (true)
		newForce.add(hapticForce_PhysWalls(newPosition, newVelocity));


	return newForce;
}
 
cVector3d Experiment::hapticForce_Segment(cVector3d newPosition, cVector3d newVelocity)
{
	double a_t[5];
	cVector3d a_closestPoint[5];
	double dist_to_seg_n[5] = {9999, 9999, 9999, 9999, 9999};
	int maxIndex = 0;
				
	// Find Closest Segment
	for (int i = 1; i < 5; i+=2)
	{
		double sq_dist_to_seg = segment_list[i]->distanceSquaredToPoint(newPosition, a_t[i], &a_closestPoint[i]);		//[m^2]
		dist_to_seg_n[i] = sqrt(sq_dist_to_seg);			//[m]
		if (dist_to_seg_n[maxIndex] > dist_to_seg_n[i])
			maxIndex = i;
	}				
							
	cSegment3d* segment = segment_list[maxIndex];
	double dist_along_segment = a_t[maxIndex];
	cVector3d clstPnt = a_closestPoint[maxIndex];

	double dist_to_seg = dist_to_seg_n[maxIndex];
	double rnd_dist_to_cntOfSeg = (float)((int)(dist_to_seg*1000000))/1000;		//[mm]
	double rnd_dist_to_WallOfSeg = rnd_dist_to_cntOfSeg - segment_width;		//[mm]
				

	// Direction of segment wall restoration force
	cVector3d segmentWallForce;
	cVector3d segmentGlidingForce;
				
	// Deliever (Positional) Proportional Orthogonal Force
		// Get direction of orthogonal force of segment wall 
		cVector3d clstPnt_ForceVector = clstPnt - newPosition;
		cVector3d norm_clstPnt_ForceVector = clstPnt_ForceVector;
		norm_clstPnt_ForceVector.normalize();

		// Set Segment Force
		segmentWallForce = norm_clstPnt_ForceVector;


	// Deliever (Velocity) Proportional inbetween Orthogonal and Tangential (to wall) Force
		// Get Projection of Linear Velocity Vector onto segment
		cVector3d segmentVector = segment->m_end - segment->m_start;
		cVector3d prjOnSeg = newVelocity.dot(segmentVector) * segmentVector 
														/ segmentVector.lengthsq();

		// Add projection point to gbl closest point on segment
		cVector3d projClstPnt = clstPnt + prjOnSeg;
		double epsilon = 0.01;
		if (dist_along_segment < epsilon)
			if (segment->m_end.distance(projClstPnt) > segment->m_end.distance(clstPnt))
				projClstPnt = segment->m_start;

		else if (dist_along_segment > 1-epsilon)
			if (segment->m_start.distance(projClstPnt) > segment->m_start.distance(clstPnt))
				projClstPnt = segment->m_end;

		// compute force vector from new position to proj point
		cVector3d projPnt_ForceVector = projClstPnt - newPosition;
					
		// Set Segment Force
		segmentGlidingForce = projPnt_ForceVector;
								

	// Calculate distance in Center "free space" Sphere
	cVector3d sphVector = newPosition - homePosition;
	double dist_from_sph_center = sphVector.length();										//[m]
	double rnd_dist_from_sph_center = (float)((int)(dist_from_sph_center*1000000))/1000;	//[mm]
	double rnd_dist_from_sph_edge = rnd_dist_from_sph_center - sph_radius;
					
				
	// Initialize Forces
	cVector3d segmentForce (0, 0, 0);
	cVector3d assistiveChannelForce (0, 0, 0);
	cVector3d sphForce (0,0,0);
	cVector3d output_hapticForce (0,0,0);
				
	// Gain Scheduling Controller
	//if (rnd_dist_from_sph_edge > 0)
	//{
		//if (rnd_dist_from_sph_edge < rnd_dist_to_WallOfSeg)
		//{
		//	double Kp = 500; // [N/m]
		//	sphForce = cMul(-Kp, sphVector);
		//	output_hapticForce.add(sphForce);
		//}
		//else 
		if (rnd_dist_to_WallOfSeg > 0)
		{
			double Kp = 0.5;
			double K_1 = min(exp(Kp * (rnd_dist_to_WallOfSeg))-1, 1000000);			
			segmentForce = cMul(K_1, segmentWallForce);

			// Add Force
			output_hapticForce.add(segmentForce);
		}
	//}

	if (rnd_dist_to_cntOfSeg > 0) 
	{
		double max_distance = 10;	// [mm]
		double Kp = 1;
		double Kv = 17;
		double K_2 = Kv + max(Kp * (max_distance - rnd_dist_to_cntOfSeg), 0);
		assistiveChannelForce = cMul(K_2, segmentGlidingForce);

		// Add Force
		output_hapticForce.add(assistiveChannelForce);
	}
	
	// Return Haptic Force Vector 
	return output_hapticForce;
}

cVector3d Experiment::hapticForce_TwoChannel(cVector3d newPosition, cVector3d newVelocity)
{
	cVector3d output_hapticForce (0,0,0);
	double Kp = 100;

	cVector3d homeCoord_newPos = newPosition - homePosition;

	// Horizontal Forces
	if (homeCoord_newPos.y > 0)
		output_hapticForce.add(cMul(Kp*-homeCoord_newPos.z, cVector3d(0,1,0)));
				
	if (homeCoord_newPos.y < 0)
		output_hapticForce.add(cMul(Kp*-homeCoord_newPos.z, cVector3d(0,-1,0)));

	// Downward Forces
	if (homeCoord_newPos.y > 0)
		output_hapticForce.add(cMul(Kp*homeCoord_newPos.y, cVector3d(0,0,-1)));
				
	if (homeCoord_newPos.y < 0)
		output_hapticForce.add(cMul(Kp*-homeCoord_newPos.y, cVector3d(0,0,-1)));


	// Return haptic force vector
	return output_hapticForce;
}

cVector3d Experiment::hapticForce_LeftChannelOnly(cVector3d newPosition, cVector3d newVelocity)
{
	cVector3d output_hapticForce (0,0,0);
	double Kp = 100;

	cVector3d homeCoord_newPos = newPosition - homePosition;

	// Horizontal Forces
	output_hapticForce.add(cMul(Kp*-homeCoord_newPos.z, cVector3d(0,-1,0)));

	// Downward Forces
	if (homeCoord_newPos.y > 0)
		output_hapticForce.add(cMul(Kp*homeCoord_newPos.y, cVector3d(0,0,-1)));
				
	if (homeCoord_newPos.y < 0)
		output_hapticForce.add(cMul(Kp*-homeCoord_newPos.y, cVector3d(0,0,-1)));


	// Return haptic force vector
	return output_hapticForce;
}

cVector3d Experiment::hapticForce_RightChannelOnly(cVector3d newPosition, cVector3d newVelocity)
{
	cVector3d output_hapticForce (0,0,0);
	double Kp = 100;

	cVector3d homeCoord_newPos = newPosition - homePosition;

	// Horizontal Forces
	output_hapticForce.add(cMul(Kp*-homeCoord_newPos.z, cVector3d(0,1,0)));

	// Downward Forces
	if (homeCoord_newPos.y > 0)
		output_hapticForce.add(cMul(Kp*homeCoord_newPos.y, cVector3d(0,0,-1)));
				
	if (homeCoord_newPos.y < 0)
		output_hapticForce.add(cMul(Kp*-homeCoord_newPos.y, cVector3d(0,0,-1)));


	// Return haptic force vector
	return output_hapticForce;
}

cVector3d Experiment::hapticForce_PhysWalls(cVector3d newPosition, cVector3d newVelocity)
{
	cVector3d output_hapticForce (0,0,0);

	// compute linear damping force
	double Kv = 20;
	cVector3d newVelocityCopy = newVelocity;
	newVelocityCopy.dot(cVector3d(1, 1, 1));
			
	cVector3d accelerantForce = Kv * newVelocityCopy;
	if (accelerantForce.distance(cVector3d(0, 0, 0)) > 1)
	{
		accelerantForce.normalize();
		accelerantForce.mul(1);
	}
	output_hapticForce.add(accelerantForce);


	// Return haptic force vector
	return output_hapticForce;
}

//---------------------------------------------------------------------------
// PRIVATE METHODS
//---------------------------------------------------------------------------

void Experiment::initializeHapticEnvironment()
{
	// User Coordinate Space Origin 
	const cVector3d origin = homePosition;

	// Constrain along segment
	cVector3d usrCoord_home (0, 0, 0);					//[m]
	cVector3d usrCoord_target_L_90 (0, -0.025, +0.000);	//[m]
	cVector3d usrCoord_target_L_45 (0, -0.020, -0.020);	//[m]
	cVector3d usrCoord_target_BOTT (0, +0.000, -0.025);	//[m]
	cVector3d usrCoord_target_R_45 (0, +0.020, -0.020);	//[m]
	cVector3d usrCoord_target_R_90 (0, +0.025, +0.000);	//[m]

	// Get segment directional vector 
	cVector3d home        = usrCoord_home        + origin; //[m]
	cVector3d target_L_90 = usrCoord_target_L_90 + origin;	//[m]
	cVector3d target_L_45 = usrCoord_target_L_45 + origin;	//[m]
	cVector3d target_BOTT = usrCoord_target_BOTT + origin;	//[m]
	cVector3d target_R_45 = usrCoord_target_R_45 + origin;	//[m]
	cVector3d target_R_90 = usrCoord_target_R_90 + origin;	//[m]
				
	// Segment Definition
	segment_list[0] = new cSegment3d (home, target_L_90);
	segment_list[1] = new cSegment3d (home, target_L_45);
	segment_list[2] = new cSegment3d (home, target_BOTT);
	segment_list[3] = new cSegment3d (home, target_R_45);
	segment_list[4] = new cSegment3d (home, target_R_90);

}

double Experiment::elapsedTimeMilliSec(SYSTEMTIME start_t)
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

double Experiment::elapsedTimeSec(SYSTEMTIME start_t)
{
	return elapsedTimeMilliSec(start_t)/1000;
}

///////////////////////////////////////////////////////////////////////
//// TYPES OF REWARDS
///////////////////////////////////////////////////////////////////////
//switch (TRIAL_STATE_MACHINE_TYPE)
//{
//	// *************************************************************************
//	case TYPE_DISTANCE_OR_TIME:
//		success = (distance_traveled > movementThreshold) | 
//					(elapsedTime > maxMovementTime);
//		break;
//
//	// *************************************************************************
//	case TYPE_SIMPLE_DISTANCE:
//		success = (distance_traveled > movementThreshold);
//		break;
//
//	// *************************************************************************
//	case TYPE_LEFT_OR_RIGHT:
//		useSegment = true;
//		success = (distance_traveled > movementThreshold);
//		break;
//
//	// *************************************************************************
//	case TYPE_TWO_PULL:
//		useSegment = true;
//		if (distance_traveled > movementThreshold)
//		{
//			success = (prev_TRIAL_STATE_MACHINE == STATE_RESET_LEVER_MID_TRIAL);
//			if (!success) {
//				TRIAL_STATE_MACHINE = STATE_RESET_LEVER_MID_TRIAL;
//			}
//		}
//		break;
//
//	// *************************************************************************
//	case TYPE_LEFT:
//		useSegment = true;
//		if (distance_traveled > movementThreshold)
//		{
//			success = GLOBAL_FLAG_target_left;
//			if (!success) {
//				TRIAL_STATE_MACHINE = STATE_RESET_LEVER_MID_TRIAL;
//			}
//		}
//		break;
//
//	// *************************************************************************
//	case TYPE_RIGHT:
//		useSegment = true;
//		if (distance_traveled > movementThreshold)
//		{
//			success = !GLOBAL_FLAG_target_left;
//			if (!success) {
//				TRIAL_STATE_MACHINE = STATE_RESET_LEVER_MID_TRIAL;
//			}
//		}
//		break;
//
//	// *************************************************************************
//	case TYPE_LEFT_AND_RIGHT:
//		if (prev_TRIAL_STATE_MACHINE == STATE_RESET_LEVER_MID_TRIAL)
//		{
//			useTwoChannels = false;
//			if (GLOBAL_FLAG_prev_target_left) { useLeftChannel = false;  useRightChannel = true; }
//			else							  { useLeftChannel = true;  useRightChannel = false; }
//		}
//		else
//		{
//			useTwoChannels = true;
//			useLeftChannel = false;
//			useRightChannel = false;
//		}
//
//		if (distance_traveled > movementThreshold)
//		{
//			success = (prev_TRIAL_STATE_MACHINE == STATE_RESET_LEVER_MID_TRIAL);
//			if (!success) {
//				TRIAL_STATE_MACHINE = STATE_RESET_LEVER_MID_TRIAL;
//			}
//		}
//		break;
//}				