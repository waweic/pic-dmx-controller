#pragma once

#include "WinUsbInterface.h"

class CUSBTestInterface
{
public:
	CUSBTestInterface(void);
	virtual ~CUSBTestInterface(void);

	BOOL Connect();
	BOOL IsConnected();
	void Disconnect();

	int SendData(unsigned char *sendData, unsigned char *rcvData, unsigned int &rcvLength);
	int SendData(unsigned char *sendData);
	static enum
	{
		ERR_OK = 0,
		ERR_NOTCONNECTED = -1,
		ERR_SENDERROR = -2,
		ERR_RECEIVEERROR = -3,
		ERR_OUTOFRANGE = -4,
		ERR_BUFFERSIZE = -5,
		ERR_UNKNOWN = -128
	};

private:
	CWinUsbInterface m_UsbInterface;
	BOOL m_Connected;
	
};
