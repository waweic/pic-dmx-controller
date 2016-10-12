#include "StdAfx.h"
#include "USBTestInterface.h"

CUSBTestInterface::CUSBTestInterface(void)
{
	m_Connected = FALSE;
}

CUSBTestInterface::~CUSBTestInterface(void)
{
	Disconnect();
}

int CUSBTestInterface::Connect()
{
	if(m_Connected) return TRUE;
	
	m_Connected = m_UsbInterface.Init();

	return m_Connected;
}

BOOL CUSBTestInterface::IsConnected()
{
	return m_Connected;
}

void CUSBTestInterface::Disconnect()
{
	if(!m_Connected) return;

	m_Connected = FALSE;

	m_UsbInterface.Close();
}

int CUSBTestInterface::SendData(unsigned char *sendData, unsigned char *rcvData, unsigned int &rcvLength)
{
	unsigned int sendLength = sizeof(sendData);
	if(!m_Connected) return ERR_NOTCONNECTED;
	if(sendLength < 0 || sendLength > 64) return ERR_OUTOFRANGE;

	unsigned long bytesWritten = 0;
	m_UsbInterface.WriteToBulkEndpoint(sendData, sendLength, &bytesWritten);
	if(bytesWritten != sendLength) return ERR_SENDERROR;

	unsigned char buf[64];
	unsigned long bytesRead = 0;
	m_UsbInterface.ReadFromBulkEndpoint(buf, 64, &bytesRead);
	
	if(bytesRead > rcvLength) 
	{
		rcvLength = bytesRead;
		return ERR_BUFFERSIZE;
	}

	rcvLength = bytesRead;
	memcpy(rcvData, buf, bytesRead);

	return ERR_OK;
}

int CUSBTestInterface::SendData(unsigned char *sendData)
{
	unsigned int sendLength = sizeof(sendData);
	if(!m_Connected) return ERR_NOTCONNECTED;
	if(sendLength < 0 || sendLength > 64) return ERR_OUTOFRANGE;

	unsigned long bytesWritten = 0;
	m_UsbInterface.WriteToBulkEndpoint(sendData, sendLength, &bytesWritten);
	if(bytesWritten != sendLength) return ERR_SENDERROR;

	return ERR_OK;
}
