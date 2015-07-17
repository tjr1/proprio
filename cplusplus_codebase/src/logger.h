#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <time.h>
#include "chai3d.h"

class Logger
{
public:
	Logger(void);
	Logger(int subjectID, int subjectWeight);
	
	bool initializeLogger(int subjectID, int subjectWeight);
	bool logVariables(double ts, int a, bool b, int c, int d, cVector3d e, cVector3d f, cVector3d g);
	bool logVariables_sparsely(double ts, int a, bool b, int c, int d, cVector3d e, cVector3d f, cVector3d g);
	bool flushLogToFile(void);
	bool writeLogToFile(void);
	bool closeFile();

private:
	double getTimeStamp(void);
};

#endif