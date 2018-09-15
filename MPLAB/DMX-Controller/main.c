#include <18F4550.h>

#device *=16

#case

#fuses HS,NOWDT,NOPROTECT,NOLVP,NODEBUG,USBDIV,PLL4,CPUDIV1,VREGEN
#use delay(clock=16000000)


////////////////////DMX-TEIL//////////////////////////////////////////
#use rs232(baud=250000,parity=N,xmit=PIN_C6,rcv=PIN_C7, stop=2,stream=DMXX)
#use timer(timer=1,tick=10ms,bits=16,isr)
#define DMXANZA 255 //Die Menge der mit dem Gerät ansprechbaren Kanäle
#define DMXANZB 255 //Wird vermutlich nicht genutzt

////////////////////USB-TEIL//////////////////////////////////////////
#define MAX_USB_PACKET_SIZE 64
#define USB_FULL_SPEED     TRUE
#define USB_HID_DEVICE     FALSE            //disabled HID
#define USB_EP1_TX_ENABLE  USB_ENABLE_BULK  //turn on EP1(EndPoint1) for IN bulk/interrupt transfers
#define USB_EP1_RX_ENABLE  USB_ENABLE_BULK  //turn on EP1(EndPoint1) for OUT bulk/interrupt transfers
#define USB_EP1_TX_SIZE    64               //size to allocate for the tx endpoint 1 buffer
#define USB_EP1_RX_SIZE    64               //size to allocate for the rx endpoint 1 buffer

unsigned char  valuea[DMXANZA]; //Die Werte pro Kanal. Vermutlich entspricht valuea[0] dem Wert für Kanal 1
//unsigned char  valueb[DMXANZB];
unsigned int dmxnum;
unsigned int status;

// ?
#include <pic18_usb.h>
#include "usb_desc_controller.h"
#include <usb.c>
#include <stdlib.h>


void senddata(char* sendstring){ //Sende einen String an den Computer
    usb_puts (1, sendstring, strlen(sendstring), 40);
    //output_toggle(PIN_B0);
}

void getitall(char* test) { //Lies alle über USB empfangenen Daten in einen String
    unsigned int8 rxdata[64];
    unsigned int8 rxdata_len;    
    rxdata_len = usb_get_packet (1, rxdata, 64);
    rxdata[rxdata_len] = 0;
    /*for(int i = 0;i<sizeof(rxdata);i++){
        if(i >= rxdata_len){
            rxdata[i] = '\0';
        }    
    }*/
    strcpy(test, rxdata);
}  

void init_dmx(void)
{
   set_uart_speed(250000, DMXX); //Baudrate für Break bzw. Reset. Vermutlich hier unnötig, weil die später eh geändert wird
   //output_high(sn75176); //RS458-Wandler auf Sendemodus versetzen; high: transmit low: receive //später nachrüsten

   status=1; //mit Break bzw. Reset beginnen
}


#ifdef __PCM_
#bit SPEN = 0x18.7
#else
#bit SPEN = 0xFAB.7
#endif

#define UART_ON  SPEN=1
#define UART_OFF SPEN=0 

#byte TXSTA = 0xFAC
#bit TRMT = TXSTA.1

#include <stdlib.h> //Warum hier?

void main(void)
{  
    int dmxsize = 10;
    for(int i=0; i < DMXANZA; i++) {
	valuea[i] = 0; 
	}
    //for(int i=0; i<DMXANZB; i++) {
	//valueb[i] = 0; 
	//}
   delay_ms(500); //Dem Controller etwas Zeit geben, nur zur Sicherheit
   usb_init();
   usb_wait_for_enumeration();
   // Ab hier haben wir eine USB-Bulk-Verbindung zum Rechner
   init_dmx();
   //set_timer0(0);
   //setup_counters( RTCC_INTERNAL, RTCC_DIV_256 | RTCC_8_BIT);
   //enable_interrupts(INT_RTCC);
   //enable_interrupts(GLOBAL);
   char meintest[64]; //Ist hier der buffer für die über USB eingelesenen Werte
  
   
   while(TRUE)
   {        
      do { 
       
      switch(status)
     {
         case 1:                //RESET (0 senden) //Warum nicht bei 0 starten?
             //UART_OFF;
             //output_low(PIN_C6);
             //delay_us(100);
             //output_high(PIN_C6);
             //delay_us(10);
             //UART_ON;
             delay_ms(10); //Wir haben zwar schon eine USB-Verbindung, geben ihr aber noch etwas Zeit.
             set_uart_speed(80000); //Mit dieser Geschwindigkeit gibt es einen Break der für DMX einen neuen Durchlauf bedeutet.
             putc(0x00, DMXX); //Break senden
             while(!TRMT); //Warten bis der Break gesendet wurde.
             set_uart_speed(250000); //Jetzt die Geschwindigkeit zurück auf das normale Maß
             putc(0x00, DMXX); //Der "nullte" Kanal ist immer 0
             
             status = 2; //Ab jetzt sollen Werte gesendet werden
             dmxnum = 0; //Wir beginnen bei Kanal 1, also 0 in valuea
         break;
         
         case 2: //DMX-Daten senden                   
               
             putc(valuea[dmxnum],DMXX); // Sende den aktuellen DMX Wert
             dmxnum++;
             if (dmxnum == dmxsize)
             {
                 status = 1;
             }
         break;
     }
      } while(status != 1); //Bevor wir den zweiten Durchlauf starten, muss zuerst geschaut werden, ob neue Daten über USB angekommen sind.
      
      if(usb_enumerated()) //Sicherstellen, dass wir eine USB-Verbindung haben
      {
         if(usb_kbhit(1)) //Sicherstellen, dass in Endpoint 1 Daten vorhanden sind
         {      
             getitall(meintest); //Das hier ist potentiell nicht sinnvoll, weil nicht sichergestellt werden kann, dass auch wirklich genug Daten da sind.
             char *ptra;
             char *ptrb;
             char delimiter[] = ";";
             // initialisieren und ersten Abschnitt erstellen
             ptra = strtok(meintest, delimiter); //String aufteilen, erster Wert geht in *ptra
 	     ptrb = strtok(NULL, delimiter); //String erneut aufteilen, alles bzw. alles bis zum nächsten Semikolon geht in *ptrb
 	     if(atoi(ptra) - 1 > dmxsize ) {
 	         dmxsize = atoi(ptra); //Die Anzahl der zu sendenden Kanäle wird erhöht, weil ein neuer Kanal da ist
	     }
 	     valuea[atoi(ptra) - 1] = atoi(ptrb); //Der Krams nach dem Semikolon wird in ein int umgewandelt und in valuea eingespeichert. Die Größe wird hier gar nicht gestestet.
 	     senddata(meintest); //Sende den empfangenen Krams zurück über USB
         }
      }

     
      
   }
}
