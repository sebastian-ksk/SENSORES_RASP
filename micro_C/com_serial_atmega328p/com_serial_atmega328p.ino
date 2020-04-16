/*
 * main.c
 *
 *  Created on: mar 23, 2020
 *      Author:juan sebastian castellanos
 */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



enum FLAGS_STATES {CLEAR,SET};


#define BAUD 9600                                   // define baud
#define BAUD_RATE ((16000000)/(BAUD*16UL)-1)     //valor de UBRR para cargarlo en el registo
#define WAKE_XBEE_PIN PIND2                     //Xbee pin11
#define  TIMER0_1SEC_COUNTER_VAL 3906           //contador de 1seg  
//#define BAUD_RATE ((1200000)/(BAUD*16UL)-1)




// REQUESTS FUNCTIONS STRINGS
//static const char AITASK_STR[] = "AITASK";
//static const char SITASK_STR[] = "SITASK";
//static const char PARAM_STR[] = "PARAM";
//static const char INITSETUP_STR[] = "INITSETUP";

const char*FUNC_STR_ARR[5];
static const int functionsNumber = 5;
unsigned char functionID;


// USART Message Variables
char requestStr[100];  //cadena de 100 caracteres
unsigned char posCounter = 0;
char *requestStrPtr;
char *paramStrPtr;
char *idStrPtr;

char *bufferPtr;
double autoIrrigParamsArr[3];

char buffer[25]={0};
unsigned int bufferSize;

// USART Interrupt Variables
volatile unsigned char receivedByte = 0;
volatile unsigned char receivedByteFlag = 0;

//External Interrupt (Flow Sensors) Variables
volatile unsigned int irrigPulsesNumberArr[4];
volatile unsigned int pulsesCounterArr[4]={0};
volatile unsigned char autoIrrigCompleteFlagArr[4] = {CLEAR};
volatile unsigned char actAutoIrrigFlagArr[4] = {CLEAR};

// Automatic Irrigation Variables
unsigned int valveID = 0;
double et = 0;

//Scheduled Irrigation Variables
volatile unsigned int timer0ItptCounter = 0;
volatile unsigned int secondsCounter = 0;
volatile uint8_t increment;

volatile unsigned int schedTaskTimeArr[4]={0};
volatile unsigned int secondsCounterArr[4]={0};
volatile unsigned char actSchedIrrigFlagArr[4]={CLEAR};

volatile unsigned char busyValveFlagArr[4]={CLEAR};

char VALV_HB1 = 0;
char VALV_HB2 = 0;
char VALV_RPA = 0;
char VALV_IFS = 0;


volatile static uint8_t pcint_temp;
volatile unsigned int testTimerCompare = 0;

char responseBuffer[50];

volatile unsigned int activeStationIndicator = 0;


char requestStrPtrArr[4][50];
char responseBufferArr[4][50];
volatile int responseBufferArrIndex = -1;

char responseAckBuffer[50];

unsigned int requestsNumber = 0;
unsigned int requestsDelayCounter;
char breakDelayFlag = CLEAR;


volatile unsigned int batteryADCValue;

volatile unsigned char remainActSchedTaskFlag = CLEAR;



//************************************************************//

void getBatteryLevelValue();

void USART_Init(unsigned int ubrr);
void USART_Transmit( unsigned char data );
unsigned char USART_Receive( void );

void printParamVal(double param);
void printString(const char *str);


void printInt( unsigned int num);

int main(void)
{
  FUNC_STR_ARR[0] = "AITASK";
  FUNC_STR_ARR[1] = "SITASK";
  FUNC_STR_ARR[2] = "ADCREQ";
  FUNC_STR_ARR[3] = "MARAIN";
  FUNC_STR_ARR[4] = "SBREAK";

  // uC SETUP

  //USART
  USART_Init(BAUD_RATE);


  // Digital Outputs for H Bridge Control
  DDRB = 0b00111111; // Set  PORTB as output
  PORTB = 0b00000000; // Set all PORTB LOW Level
  /*
   * PBO -> VALV1-HB1
   * PB1 -> VALV1-HB2
   * PB2 -> VALV2-HB1
   * PB3 -> VALV2-HB2
   * PB4 -> VALV3-HB1
   * PB5 -> VALV3-HB2
   * PB6 -> VALV1-HB1
   * PB7 -> VALV1-HB2
   */
  //Digital Outputs for Relay/Power Control
  DDRD = (1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7) ;
  PORTD = 0b00000000;
  PORTD |= (1 << PIND4) | (1 << PIND5) ;

  DDRD |= (1 << WAKE_XBEE_PIN);
  PORTD &=~ (1 << WAKE_XBEE_PIN);

 
  
  //ADC Setup
  /*  ADCSRA
    Bit 7 -   ADEN: ADC Enable
    Bit 6 -   ADSC: ADC Start Conversion
    Bit 5 -   ADATE: ADC Auto Trigger Enable
    Bit 4 -   ADIF: ADC Interrupt Flag
    Bit 3 -   ADIE: ADC Interrupt Enable
    Bits 2:0 -  ADPS[2:0]: ADC Prescaler Select Bits (100 to divide by 16)
   */
  ADCSRA = 0b10001111;
  /*  ADMUX
      Bit 7:6 –   REFS[1:0]: Reference Selection Bits
    Bit 5 –   ADLAR: ADC Left Adjust Result
    Bit 4 –   Reserved
    Bits 3:0 –  MUX[3:0]: Analog Channel Selection Bits
   */
  ADMUX = 0b01001111;
  /* Disable Digital Input on ADC Channel 5,4,3 and 2vto reduce power consumption */
  DIDR0 |= (1 << ADC5D) | (1 << ADC4D)| (1 << ADC3D)| (1 << ADC2D);

  //**************************************************************************************
  //  MAIN LOOP
  //**************************************************************************************
  while (1){
    //printInt(1111);
     uint16_t mes[4];
     float vol=0;
     int ad=0x40;
         //printString("ok\n"); 
     
     for(unsigned int x=0;x<4;x++)
       {
         ad=0x45-x; //para leer el puerto
         ADMUX = ad;
         ADCSRA|=((1<<ADEN)|(1<<ADSC));
         while(ADCSRA&(1<<ADSC));
         ADCSRA &=~(1<<ADEN);
         mes[x]=ADC;
       }
     
      sprintf(responseAckBuffer, "%d,%d,%d,%d", mes[0],mes[1],mes[2],mes[3]);
      printString(responseAckBuffer);
//      FILE* fichero;
//      fichero = fopen("adc.txt", "a");
//      int i=0;
//      while (responseAckBuffer[i]!='\0') { fputc(responseAckBuffer[i], fichero); i++; }
//      i=0;
//      fclose(fichero);
      
     //sprintf(responseAckBuffer, "%s,%d\n", "OK", 1);
     //printString(responseAckBuffer);
      _delay_ms(10);

  }
}

// FUNCTIONS LIST
// functionID = 1 -> AITASK
// functionID = 2 -> SITASK
// functionID = 3 -> BLREQ



//*****************************************************************************************
//*****************************************************************************************

void getBatteryLevelValue(){

  if( (idStrPtr = strsep(&requestStrPtr,";")) != NULL ){ //strseo busca un valor delimitado por ";"
    if (strcmp(idStrPtr,"BATT") == 0){

      ADMUX = 0b00000101;
      _delay_ms(1000);
      ADCSRA |= (1 << ADSC);
    }
  }




}








void printInt( unsigned int num){
  USART_Transmit('0'+((num)/1000)); // Thousands
  USART_Transmit('0'+((num%1000)/100)); //Hundreds
  USART_Transmit('0'+((num%100)/10)); // Tens
  USART_Transmit('0'+(num%10)); // Units
  USART_Transmit('\n');
}

void printString(const char *str){
  const char *strPtrCpy = str;
  while(*strPtrCpy != '\0'){
    USART_Transmit(*strPtrCpy);
    strPtrCpy++;
  }
  USART_Transmit('\n');
}


//*************************************************************************************************
/*
 * INTRRUPTS
 */
//*************************************************************************************************







/*USART FUNCTIONS*/

ISR(USART_RX_vect){
  receivedByte = USART_Receive();
  receivedByteFlag = SET;
  //USART_Transmit(receivedByte);
}
void USART_Init( unsigned int ubrr)
{
  /*Set baud rate */
  UBRR0H = (unsigned char)(ubrr>>8);
  UBRR0L = (unsigned char)ubrr;
  /*Enable receiver and transmitter */
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  /* Set frame format: 8data, 2 stop bit */
  //UCSR0C = (1<<USBS0)|(3<<UCSZ00);
  UCSR0C |= (3<<UCSZ00);
}

void USART_Transmit( unsigned char data )
{
/* Wait for empty transmit buffer */
  while ( !( UCSR0A & (1<<UDRE0)) );
  /* Put data into buffer, sends the data */
  UDR0 = data;
}

unsigned char USART_Receive( void )
{
  /* Wait for data to be received */
  while ( !(UCSR0A & (1<<RXC0)) );
  /* Get and return received data from buffer */
  return UDR0;
}


