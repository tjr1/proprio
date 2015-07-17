#include "logger.h"

//---------------------------------------------------------------------------
// DECLARED NAMESPACES
//---------------------------------------------------------------------------
using namespace std;
using namespace System;

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------
const int LOGGER_SAMPLING_RATE = 1000;		// Hz

const int SAME_DATA_DISCARD_TIME_LIMIT = 10;	// milliseconds

//---------------------------------------------------------------------------
// DECLARED GLOBAL VARIABLES
//---------------------------------------------------------------------------
ofstream logFile;
SYSTEMTIME start_t;
bool GLOBAL_FLAG_INITIALIZED = false;

vector< cVector3d > log_position(1,cVector3d(0,0,0));
vector< cVector3d > log_linearVelocity(1,cVector3d(0,0,0));
vector< cVector3d > log_force(1,cVector3d(0,0,0));
vector< double > log_time_stamps_msec(1,0);
vector< int > log_STATE_MACHINE(1,0);
vector< int > log_reward_delievered(1,0);
vector< int > log_cap_sen_sipper(1,0);
vector< int > log_cap_sen_handle(1,0);

// logVariables_sparsely variable
vector< double > same_data_time_stamp;

// flush logged values to file variable
int cur_flushed_index = 0;

//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// see header file

//---------------------------------------------------------------------------
// CONSTRUCTOR
//---------------------------------------------------------------------------

Logger::Logger()
{
	GLOBAL_FLAG_INITIALIZED = false;
}

Logger::Logger(int subjectID, int subjectWeight)
{
	initializeLogger(subjectID, subjectWeight);
}

//---------------------------------------------------------------------------
// PULBIC METHODS
//---------------------------------------------------------------------------
bool Logger::initializeLogger(int subjectID, int subjectWeight)
{
	if (GLOBAL_FLAG_INITIALIZED == false)
	{
		// Getting Date/Time
		time_t rawtime;
		struct tm * timeinfo;
		char buffer [80];

		time (&rawtime);
		timeinfo = localtime (&rawtime);

		strftime (buffer,80,"%Y%m%d_%H%M%S_trainingData.txt",timeinfo);

		// VERBOSE
		//Console::WriteLine("Creating Log File... ");
	
		std::stringstream sstm;
		sstm << "data\\SUBJECT_" << subjectID << "\\" << string(buffer);
		string pathName = sstm.str();

		logFile.open(pathName);
		if (logFile == NULL) 
		{ 
			//TODO: Directory Management
			//CreateDirectory("foo", NULL)

			return false;
		}
		else
		{
			// VERBOSE 
			//Console::WriteLine("Log File Created. ");

			logFile << "HEADER_START\n";
			logFile << "VERSION:";	logFile << "1.0";			logFile << ",";
			logFile << "DATE:";		logFile << "2014-10-28";	logFile << ",";
			logFile << "ID:";		logFile << subjectID;		logFile << ",";
			logFile << "WEIGHT:";	logFile << subjectWeight; 
			logFile << "\n";
			logFile << "FIELDS:";
				logFile << "TIME_STAMP,";
				logFile << "STATE_MACHINE,";
				logFile << "REWARD,";
				logFile << "CAP_SENSOR_SIPPER,";
				logFile << "CAP_SENSOR_HANDLE,";
				logFile << "POS_X,POS_Y,POS_Z,";
				logFile << "FOR_X,FOR_Y,FOR_Z";
				logFile << "\n";
			logFile << "HEADER_END\n";
		}
	
		GLOBAL_FLAG_INITIALIZED = true;
		GetSystemTime(&start_t);
		return true;
	}
	else
		return false;
}

bool Logger::logVariables(double ts, int a, bool b, int c, int d, cVector3d e, cVector3d f, cVector3d g)
{
	if (GLOBAL_FLAG_INITIALIZED == true)
	{
		// if a valid time stamp is given, log it, otherwise generate your own
		if (ts < 0)
			log_time_stamps_msec.push_back(this->getTimeStamp());	
		else
			log_time_stamps_msec.push_back(ts);						

		// Log Variables
		log_STATE_MACHINE.push_back(a);			//STATE_MACHINE

		log_reward_delievered.push_back(b);		//GLOBAL_FLAG_ardin_delieverReward
		log_cap_sen_sipper.push_back(c);		//ardout_capSen_sipperTube
		log_cap_sen_handle.push_back(d);		//ardout_capSen_handleOutside
		
		log_position.push_back(e);				//cur_position
		log_linearVelocity.push_back(f);		//cur_linearVelocity
		log_force.push_back(g);					//cur_force

		return true;
	}
	else
		return false;
}

bool Logger::logVariables_sparsely(double ts, int a, bool b, int c, int d, cVector3d e, cVector3d f, cVector3d g)
{
	if (GLOBAL_FLAG_INITIALIZED == true)
	{
		// Get Time Stamp
		double new_time_stamp = this->getTimeStamp();

		// Check if data is redundant
		bool same_data = true;

		if (log_STATE_MACHINE.size() > 0)
		{
			same_data = same_data && (a == log_STATE_MACHINE.back());		//STATE_MACHINE
			same_data = same_data && (b == log_reward_delievered.back());	//GLOBAL_FLAG_ardin_delieverReward
			same_data = same_data && (c == log_cap_sen_sipper.back());		//ardout_capSen_sipperTube
			same_data = same_data && (d == log_cap_sen_handle.back());		//ardout_capSen_handleOutside
			same_data = same_data && e.equals(log_position.back());			//cur_position
			same_data = same_data && f.equals(log_linearVelocity.back());	//cur_linearVelocity
			same_data = same_data && g.equals(log_force.back());			//cur_force
		}
		else
			same_data = false;
		
		if (same_data == true)
			same_data_time_stamp.push_back(new_time_stamp);

		// Data is the same & vector isn't null & time has elapsed longer than limit
		if ( (same_data == true) && \
			 (same_data_time_stamp.size() > 0) && \
			 (new_time_stamp - same_data_time_stamp.front() > SAME_DATA_DISCARD_TIME_LIMIT) )
		{
			// discard information
			return false;
		}
		else
		{
			if ( (same_data_time_stamp.size() > 0) && \
				 (new_time_stamp - same_data_time_stamp.front() > SAME_DATA_DISCARD_TIME_LIMIT) )
			{
				this->logVariables(same_data_time_stamp.back(), \
								   log_STATE_MACHINE.back(), \
								   log_reward_delievered.back(), \
								   log_cap_sen_sipper.back(), \
								   log_cap_sen_handle.back(), \
								   log_position.back(), \
								   log_linearVelocity.back(), \
								   log_force.back());	
				same_data_time_stamp.clear();
			}
			this->logVariables(new_time_stamp, a, b, c, d, e, f, g);

			if (same_data == false)
				same_data_time_stamp.clear();

			// successful logging
			return true;
		}	

		
	}
	
	return true;
}

bool Logger::flushLogToFile()
{
	if (GLOBAL_FLAG_INITIALIZED == true)
	{
		int end_index = log_force.size();

		for(int i=cur_flushed_index; i < end_index; i++)
		{
			logFile << log_time_stamps_msec[i] << ",";
			logFile << log_STATE_MACHINE[i] << ",";
			logFile << log_reward_delievered[i] << ",";
			logFile << log_cap_sen_sipper[i] << ",";
			logFile << log_cap_sen_handle[i] << ",";
			logFile << log_position[i].x << "," << log_position[i].y << "," << log_position[i].z << ",";
			logFile << log_force[i].x << "," << log_force[i].y << "," << log_force[i].z;
			logFile << "\n";
		}

		cur_flushed_index = end_index;
		return true;
	}
	else
		return false;
}


bool Logger::writeLogToFile()
{
	if (GLOBAL_FLAG_INITIALIZED == true)
	{
		//cout << "Dumped " << log_time_stamps.size() << " saved data points. \n";

		// TODO: Write to LOG FILE
		// FIELDS: TIME_STAMP, STATE_MACHINE, REWARD, CAP_SENSOR_SIPPER, CAP_SENSOR_HANDLE, POS_X,POS_Y,POS_Z,FOR_X,FOR_Y,FOR_Z
		for(int i=cur_flushed_index; i < log_time_stamps_msec.size(); i++)
		{
			logFile << log_time_stamps_msec[i] << ",";
			logFile << log_STATE_MACHINE[i] << ",";
			logFile << log_reward_delievered[i] << ",";
			logFile << log_cap_sen_sipper[i] << ",";
			logFile << log_cap_sen_handle[i] << ",";
			logFile << log_position[i].x << "," << log_position[i].y << "," << log_position[i].z << ",";
			logFile << log_force[i].x << "," << log_force[i].y << "," << log_force[i].z;
			logFile << "\n";
		}

		log_time_stamps_msec.clear();
		log_STATE_MACHINE.clear();
		log_reward_delievered.clear();
		log_cap_sen_sipper.clear();
		log_cap_sen_handle.clear();
		log_position.clear();
		log_linearVelocity.clear();
		log_force.clear();

		log_time_stamps_msec.push_back(0);
		log_STATE_MACHINE.push_back(0);
		log_reward_delievered.push_back(0);
		log_cap_sen_sipper.push_back(0);
		log_cap_sen_handle.push_back(0);
		log_position.push_back(cVector3d(0,0,0));
		log_linearVelocity.push_back(cVector3d(0,0,0));
		log_force.push_back(cVector3d(0,0,0));

		cur_flushed_index = 0;

		return true;
	}
	else
		return false;
}

bool Logger::closeFile()
{
	if (GLOBAL_FLAG_INITIALIZED == true)
	{
		logFile.close();
		return true;
	}
	else
		return false;
}

//---------------------------------------------------------------------------
// PRIVATE METHODS
//---------------------------------------------------------------------------
double Logger::getTimeStamp()
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