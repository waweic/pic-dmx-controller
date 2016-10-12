#pragma once

#include <winusb.h>
#include <Setupapi.h>
#include <strsafe.h>

static const GUID OSR_DEVICE_INTERFACE = 
 { 0x74A4CE97, 0xF99D, 0x49C8, { 0xA3, 0x39, 0x6F, 0x9E, 0xE4, 0x45, 0xB2, 0x46 } };
class CWinUsbInterface
{
public:
	CWinUsbInterface(void);
	~CWinUsbInterface(void);
	
	BOOL Init();
	void Close();
	BOOL QueryDeviceEndpoints();
	BOOL WriteToBulkEndpoint(unsigned char *data, unsigned long dataLen, unsigned long *dataLenWritten);
	BOOL ReadFromBulkEndpoint(unsigned char *data, unsigned long dataLen, unsigned long *dataLenRead);
	
private:
	struct PIPE_ID
	{
		unsigned int PipeInId;
		unsigned int PipeOutId;
	};

	HANDLE m_hDeviceHandle;
	HANDLE m_hWinUSBHandle;
	PIPE_ID m_PipeId;
};
