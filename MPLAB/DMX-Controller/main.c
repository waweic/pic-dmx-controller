#include <18F4550.h>

#device *=16

#case

#fuses HS,NOWDT,NOPROTECT,NOLVP,NODEBUG,USBDIV,PLL4,CPUDIV1,VREGEN
#use delay(clock=16000000)


////////////////////DMX-TEIL//////////////////////////////////////////
#use rs232(baud=250000,parity=N,xmit=PIN_C6,rcv=PIN_C7, stop=2,stream=DMXX)
#use timer(timer=1,tick=10ms,bits=16,isr)
#define MAX_USB_PACKET_SIZE 64
#define USB_FULL_SPEED     TRUE
#define USB_HID_DEVICE     FALSE            //disabled HID
#define USB_EP1_TX_ENABLE  USB_ENABLE_BULK  //turn on EP1(EndPoint1) for IN bulk/interrupt transfers
#define USB_EP1_RX_ENABLE  USB_ENABLE_BULK  //turn on EP1(EndPoint1) for OUT bulk/interrupt transfers
#define USB_EP1_TX_SIZE    64               //size to allocate for the tx endpoint 1 buffer
#define USB_EP1_RX_SIZE    64               //size to allocate for the rx endpoint 1 buffer
#define        DMXANZA 255
#define        DMXANZB 255


unsigned char  valuea[DMXANZA];
//unsigned char  valueb[DMXANZB];
unsigned int   dmxnum;  
unsigned int   status;


#include <pic18_usb.h>
#include "usb_desc_controller.h"
#include <usb.c>
#include <stdlib.h>


void senddata(char* sendstring){
    usb_puts (1, sendstring, strlen(sendstring), 40);
    //output_toggle(PIN_B0);
}    
void getitall(char* test) {
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
   set_uart_speed(250000, DMXX);      //Baudrate für Break bzw. Reset
   //output_high(sn75176);       //RS458-Wandler auf Sendemodus versetzen; high: transmit low: receive //später nachrüsten

   status=1;                  //mit Break bzw. Reset beginnen
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

#include <stdlib.h>   

void main(void)
{  
    int dmxsize = 10;
    for(int i=0; i<DMXANZA; i++) {
	valuea[i] = 0; 
	}
    //for(int i=0; i<DMXANZB; i++) {
	//valueb[i] = 0; 
	//}
   delay_ms(500);
   usb_init();
   usb_wait_for_enumeration();
   init_dmx();
   //set_timer0(0);
   //setup_counters( RTCC_INTERNAL, RTCC_DIV_256 | RTCC_8_BIT);
   //enable_interrupts(INT_RTCC);
   //enable_interrupts(GLOBAL);
   char meintest[64];
  
   
   while(TRUE)
   {        
      do { 
       
      switch(status)
     {
         case 1:                //RESET (0 senden)
             //UART_OFF;
             //output_low(PIN_C6);
             //delay_us(100);
             //output_high(PIN_C6);
             //delay_us(10);
             //UART_ON;
             delay_ms(10);
             set_uart_speed(80000);
             putc(0x00, DMXX);
             while(!TRMT);
             set_uart_speed(250000);
             putc(0x00, DMXX);
             
             status = 2;
             dmxnum = 0;
         break;
         
         case 2:        //DMX-Daten senden                   
               
             putc(valuea[dmxnum],DMXX);    // Sende den aktuellen DMX Wert
             dmxnum++;
             if (dmxnum == dmxsize)
             {
                 status = 1;
             }
         break;
     }
      } while(status != 1);
      
      if(usb_enumerated())
      {
         if(usb_kbhit(1))
         {      
             getitall(meintest); 
             char *ptra;
             char *ptrb;
             char delimiter[] = ";";
             // initialisieren und ersten Abschnitt erstellen
             ptra = strtok(meintest, delimiter);
 	         ptrb = strtok(NULL, delimiter);
 	         if(atoi(ptra) - 1 > dmxsize )
 	         dmxsize = atoi(ptra);
 	         valuea[atoi(ptra) - 1] = atoi(ptrb);
 	         senddata(meintest);
             
            
                        
                 
  
         }
      }

     
      
   }
}