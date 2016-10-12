#include "StdAfx.h"

#include "WinUsbInterface.h"



CWinUsbInterface::CWinUsbInterface(void)
{
	m_hDeviceHandle = INVALID_HANDLE_VALUE;
	m_hWinUSBHandle = INVALID_HANDLE_VALUE;
}

CWinUsbInterface::~CWinUsbInterface(void)
{
	Close();
}

BOOL CWinUsbInterface::Init()
{
	GUID guidDeviceInterface = OSR_DEVICE_INTERFACE;
	HDEVINFO hDeviceInfo;
	SP_DEVINFO_DATA DeviceInfoData;

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;
	LPTSTR lpDevicePath = NULL;
	//wchar_t lpDevicePath = NULL;

	// Get information about all the installed devices for the specified
	// device interface class.
	hDeviceInfo = SetupDiGetClassDevs(&guidDeviceInterface,	NULL, NULL,	DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (hDeviceInfo == INVALID_HANDLE_VALUE) 
	{ 
		return false;
	}

	//Enumerate all the device interfaces in the device information set.
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD index = 0; SetupDiEnumDeviceInfo(hDeviceInfo, index, &DeviceInfoData); index++)
	{
		//Reset for this iteration
		if (lpDevicePath)
		{
			LocalFree(lpDevicePath);
			lpDevicePath = NULL;
		}
		if (pInterfaceDetailData)
		{
			LocalFree(pInterfaceDetailData);
			pInterfaceDetailData = NULL;
		}

		deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

		//Get information about the device interface.
		if(!SetupDiEnumDeviceInterfaces(hDeviceInfo, &DeviceInfoData, &guidDeviceInterface, index, &deviceInterfaceData)) 
		{
			break;
		}

		// Check if last item
		if (GetLastError () == ERROR_NO_MORE_ITEMS)
		{
			break;
		}

		//Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
		//which we need to allocate, so we have to call this function twice.
		//First to get the size so that we know how much to allocate
		//Second, the actual call with the allocated buffer

		ULONG requiredLength=0;
		if (!SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, NULL, 0,	&requiredLength, NULL)) 
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && requiredLength > 0)
			{
				//we got the size, allocate buffer
				pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, requiredLength);

				if(!pInterfaceDetailData) 
				{ 
					break;
				}
			}
			else
			{
				break;
			}
		}

		//get the interface detailed data
		pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		if(!SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, pInterfaceDetailData, requiredLength, NULL, &DeviceInfoData)) 
		{
			break;
		}

		//copy device path
		size_t nLength = wcslen ((wchar_t *)pInterfaceDetailData->DevicePath) * 2 + 1;
		//hier drüber habe ich den fehler eingebaut
		lpDevicePath = (TCHAR *) LocalAlloc (LPTR, nLength * sizeof(TCHAR));
		StringCchCopy(lpDevicePath, nLength, pInterfaceDetailData->DevicePath);
		lpDevicePath[nLength-1] = 0;
	}

	if(lpDevicePath)
	{
		//Open the device
		m_hDeviceHandle = CreateFile(lpDevicePath, GENERIC_READ | GENERIC_WRITE,	FILE_SHARE_READ | FILE_SHARE_WRITE,	NULL,	OPEN_EXISTING,	FILE_FLAG_OVERLAPPED,	NULL);

		if(m_hDeviceHandle != INVALID_HANDLE_VALUE)
		{
		  if(WinUsb_Initialize(m_hDeviceHandle, &m_hWinUSBHandle))
		  {
				 QueryDeviceEndpoints();
		  }
		}
	}

	LocalFree(lpDevicePath);
	LocalFree(pInterfaceDetailData);    
	SetupDiDestroyDeviceInfoList(hDeviceInfo);

	return m_hDeviceHandle != INVALID_HANDLE_VALUE &&	m_hWinUSBHandle != INVALID_HANDLE_VALUE;
}

BOOL CWinUsbInterface::QueryDeviceEndpoints()
{
	if (m_hDeviceHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	
	BOOL bResult = TRUE;

	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
	ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

	WINUSB_PIPE_INFORMATION  Pipe;
	ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));

	bResult = WinUsb_QueryInterfaceSettings(m_hWinUSBHandle, 0, &InterfaceDescriptor);

	if (bResult)
	{
		for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
		{
			bResult = WinUsb_QueryPipe(m_hWinUSBHandle, 0, index, &Pipe);

			if(bResult)
			{
				if (Pipe.PipeType == UsbdPipeTypeControl)
				{
				}
				if (Pipe.PipeType == UsbdPipeTypeIsochronous)
				{
				}
				if (Pipe.PipeType == UsbdPipeTypeBulk)
				{
					if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
					{
						m_PipeId.PipeInId = Pipe.PipeId;
						unsigned long timeout = 100;
						WinUsb_FlushPipe(m_hWinUSBHandle, m_PipeId.PipeInId);
						WinUsb_SetPipePolicy(m_hWinUSBHandle, m_PipeId.PipeInId, PIPE_TRANSFER_TIMEOUT, sizeof(timeout), &timeout);
					}
					if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
					{
						m_PipeId.PipeOutId = Pipe.PipeId;
						unsigned long timeout = 100;
						WinUsb_FlushPipe(m_hWinUSBHandle, m_PipeId.PipeOutId);
						WinUsb_SetPipePolicy(m_hWinUSBHandle, m_PipeId.PipeOutId, PIPE_TRANSFER_TIMEOUT, sizeof(timeout), &timeout);
					}
				}
				if (Pipe.PipeType == UsbdPipeTypeInterrupt)
				{
				}
			}
			else
			{
				continue;
			}
		}
	}

	return 	bResult;
;
}

BOOL CWinUsbInterface::WriteToBulkEndpoint(unsigned char *data, unsigned long dataLen, unsigned long *dataLenWritten)
{
	if (m_hWinUSBHandle == INVALID_HANDLE_VALUE) return FALSE;

	BOOL bResult = WinUsb_WritePipe(m_hWinUSBHandle, m_PipeId.PipeOutId, data, dataLen, dataLenWritten, 0);

	return bResult;
}

BOOL CWinUsbInterface::ReadFromBulkEndpoint(unsigned char *data, unsigned long dataLen, unsigned long *dataLenRead)
{
	if (m_hWinUSBHandle == INVALID_HANDLE_VALUE) return FALSE;

	BOOL bResult = WinUsb_ReadPipe(m_hWinUSBHandle, m_PipeId.PipeInId, data, dataLen, dataLenRead, 0);

	return bResult;
}

void CWinUsbInterface::Close()
{
	if(m_hDeviceHandle != INVALID_HANDLE_VALUE) CloseHandle(m_hDeviceHandle);
	if(m_hWinUSBHandle != INVALID_HANDLE_VALUE) WinUsb_Free(m_hWinUSBHandle);
	m_hDeviceHandle = INVALID_HANDLE_VALUE;
	m_hWinUSBHandle = INVALID_HANDLE_VALUE;
}