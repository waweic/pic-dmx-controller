/*
	Dieses Plugin soll ein Grundger�st f�r eigene Plugins f�r das PC_DIMMER2012 Pluginsystem dienen. Aus diesem
	Grund besitzt dieses Plugin keine nennenswerten Funktionen au�er der Verbindung zwischen PC_DIMMER und Plugin.

	Bitte wenden sie sich an christian.noeding@arcor.de oder an http://www.pcdimmer.de, falls ihr Plugin ver�ffentlicht
	werden soll.

	Vielen Dank,
	Christian N�ding
*/

#include "stdafx.h"
#include "mmsystem.h"
#include "math.h"
#include <oaidl.h>
#include <stdio.h>
#include "USBTestInterface.h"


// CallBacks Definitionen
typedef void (__stdcall cbSetDLLValues) (int address, int startvalue, int endvalue, int fadetime, int delay) ;
typedef void (__stdcall cbSetDLLValueEvent) (int address, int endvalue) ;
typedef void (__stdcall cbSetDLLNames) (int address, char *channelname) ;
typedef int (__stdcall cbGetDLLValue) (int address) ;
typedef void (__stdcall cbSendMessage) (char MSG, VARIANT Data1, VARIANT Data2) ;

// Globale Pointers to Functions
cbSetDLLValues * pSetDLLValues  = NULL ;
cbSetDLLValueEvent * pSetDLLValueEvent  = NULL ;
cbSetDLLNames * pSetDLLNames = NULL ;
cbGetDLLValue * pGetDLLValue = NULL ;
cbSendMessage * pSendMessage = NULL ;

#define MaxChan 254

int channelvalue[MaxChan];
bool IsSending;
CUSBTestInterface myusb;



BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  return true ;
}

// === Die folgenden 5 Funktionen koennen vom Plugin aufgerufen werden (s.o.)
//
// procedure CallbackSetDLLValues(address,startvalue,endvalue,fadetime:integer);
//   Funktion wird aufgerufen, wenn Plugins einen neuen Kanalwert an das
//   Hauptprogramm senden sollen.
//
// procedure CallbackSetDLLValueEvent(address,endvalue:integer);
//   Funktion wird aufgerufen, wenn Plugins statt eines neuen Kanalwertes
//   Ereignisse �ber die Data-In-Funktion ausf�hren sollen.
//
// procedure CallbackSetDLLNames(address:integer; channelname:PChar);
//   Funktion wird aufgerufen, wenn Plugins einen neuen Kanalnamen an das
//   Hauptprogramm senden sollen.
//
// CallbackGetValue(address:integer);
//   Funktion wird aufgerufen, wenn Plugins einen Kanalwert anfordern.
//   Die Funktion ruft (im Hauptprogramm) dann wiederum DLLSendData()
//   bzw. DLLSendMessage:MSG_ACTUALCHANNELVALUE auf
//
// CallbackSendMessage(MSG:Byte; Data1,Data2: Variant);
//   Funktion kann ausgef�hrt werden, um Nachrichten an das Hauptprogramm
//   zu liefern. Eine Liste der M�glichen Nachrichten gibt es hier: http://www.pcdimmer.de/wiki/index.php/Nachrichtensystem

void __stdcall DLLCreate(cbSetDLLValues * CallbackSetDLLValues, cbSetDLLValueEvent * CallbackSetDLLValueEvent,
                           cbSetDLLNames * CallbackSetDLLNames, cbGetDLLValue * CallbackGetDLLValue,
						   cbSendMessage * CallbackSendMessage)
{
  // Funktionsaufruf bei Pluginerstellung

  // Zuweisen der einzelnen Pointer f�r die Callbacks
  pSetDLLValues		= CallbackSetDLLValues ;
  pSetDLLValueEvent = CallbackSetDLLValueEvent ;
  pSetDLLNames		= CallbackSetDLLNames ;
  pGetDLLValue		= CallbackGetDLLValue ;
  pSendMessage		= CallbackSendMessage ;

  IsSending=false;
  if(!myusb.Connect()){
  }
  return ;
}

void __stdcall DLLStart()
{
  
  return ;
}

void __stdcall DLLDestroy()
{
  myusb.Disconnect();
  return ;
}

char * __stdcall DLLIdentify()
{
  // Funktionsaufruf bei Identifizierung ob Programmplugin oder Ausgabeplugin
  return ("Output") ; // Output oder Input
}

char * __stdcall DLLGetVersion()
{
  // Derzeit nur zu Anzeigezwecken verwendet
  return ("v0.1") ;
}

char * __stdcall DLLGetName()
{
  // Dieser Name wird in der Pluginliste angezeigt
  return ("PIC USB-DMX-Controller v0.01") ;
}

void __stdcall DLLAbout()
{
  // Funktionsaufruf bei Klick auf "Info"
  MessageBox(NULL, "TO DO: Add Text", "PC_DIMMER-Plugin", MB_OK | MB_ICONEXCLAMATION);
  return ;
}

void __stdcall DLLConfigure()
{
  // Funktionsaufruf bei Klick auf "Konfigurieren"
  MessageBox(NULL, "TO DO: Add Text", "PC_DIMMER-Plugin", MB_OK | MB_ICONEXCLAMATION) ;
  return ;
}

// fadetime in milliseconds !
void __stdcall DLLSendData(int address, int startvalue, int endvalue, int fadetime, char *name)
{
// Diese Funktion wird bei jeder manuellen Kanal�nderung, bzw. bei jedem Start eines automatischen Dimmvorganges ausgef�hrt
// Hat man eine Hardware, die keine Fadezeiten unterst�tzt und den internen PC_DIMMER2012-Dimmerkernel ben�tigt, nutzt man
// besser die Funktion DLLSendMessage weiter unten, um immer die aktuellen Kanalwerte zu erhalten
  return ;
}

bool __stdcall DLLIsSending()
{
  // Funktionsaufruf um bei Interfaces mit DMX-In keine Feedbackschleife zu erzeugen. Sendet das Plugin einen Wert an
  // den PC_DIMMER, muss entsprechend die Variable "IsSending" gesetzt werden.
  return IsSending;
}

void __stdcall DLLSendMessage(char MSG, VARIANT Data1, VARIANT Data2)
{
//   Funktion wird aufgerufen, wenn im PC_DIMMER Nachrichten erzeugt
//   werden. Dies tritt bei Beat-Impulsen, Speicher-/�ffnen-Anforderungen
//   oder Kanalwert�nderungen des Dimmerkernels auf. F�r DMX Ger�te sollte
//   auf die Nachricht MSG=MSG_ACUTALCHANNELVALUE=14 reagiert und
//   auf Data1=Kanal und Data2=Kanalwert ausgewertet werden.

if (MSG==14) // MSG_ACTUALCHANNELVALUE=14
  {
    // Hier etwas mit den neuen Kanalwerten machen
	char tempstring[256];

	IsSending = true;
    channelvalue[Data1.intVal] = Data2.intVal;
	//tempstring = (unsigned char*)Data1.bstrVal;
	sprintf(tempstring, "%d;%d", Data1.intVal, Data2.intVal);
	//strcat(tempstring, ";");
	//strcat(tempstring, (char *)Data2.bstrVal);
    myusb.SendData((unsigned char *)tempstring);
	IsSending = false;
  }
}


