#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

class SerialHandler
{
public:
	SerialHandler(void);
	
	bool initializeHandler(void);
	bool readSerialPort(void)
};

#endif