/*
 * FreeModbus Libary: AVR Demo Application
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: demo.c,v 1.7 2006/06/15 15:38:02 wolti Exp $
 */
//=======================================================================================
#define INVBIT(port, bit) port = port ^ (1<<bit);
#define UPBIT(port, bit) port = port | (1<<bit);
#define DOWNBIT(port, bit) port = port & (~(1<<bit));
//=======================================================================================
// ----------------------- AVR includes -------------------------------------------------
//=======================================================================================
#include "avr/io.h"
#include "avr/interrupt.h"
#include <avr/delay.h>
//=======================================================================================
// ----------------------- Modbus includes ----------------------------------------------
//=======================================================================================
#include "mb.h"
#include "mbport.h"
//=======================================================================================
// ----------------------- Defines ------------------------------------------------------
//=======================================================================================
#define REG_INPUT_START 1000
#define REG_INPUT_NREGS 50
//=======================================================================================
USHORT swap_byte(USHORT v)
{
	USHORT r;
	unsigned char * vb = &v;
	unsigned char * rb = &r;
	rb[0] = vb[1];
	rb[1] = vb[0];
	return r;
}
//=======================================================================================
// ----------------------- Static variables ---------------------------------------------
//=======================================================================================
static USHORT   usRegInputStart = REG_INPUT_START;
static USHORT   usRegInputBuf[REG_INPUT_NREGS];
//=======================================================================================
// FOR AUTO WATERING                                                         
//=======================================================================================
#define LED_PORT PORTD
#define LED_DDR_PORT DDRD
#define LED_B_PIN 7
#define LED_R_PIN 6

inline 
void 
led_on(char pin) {UPBIT(LED_PORT, pin);}

inline 
void 
led_off(char pin){DOWNBIT(LED_PORT, pin);}

inline 
void 
led_tagle(char pin){INVBIT(LED_PORT, pin);}

//=======================================================================================
#define VALVE_PORT PORTB
#define VALVE_DDR_PORT DDRB
#define VALVE_PIN 0
#define VALVE_CLOSE_STATE 255
#define VALVE_OPEN_STATE 1

inline 
void 
valve_open(){UPBIT(VALVE_PORT, VALVE_PIN);}

inline 
void 
valve_close(){DOWNBIT(VALVE_PORT, VALVE_PIN);}
//=======================================================================================
void adc_init(void)
{
    ADMUX = 0x40;
    ADCSRA = 0x84;  
    SFIOR = 0x00;
}

int 
readAdc(char channel)
{
	ADMUX = (channel) | (1<<REFS0);
	ADCSRA |=(1<<ADSC);
	while(ADCSRA & (1<<ADSC)){};								
	return ADCW;
}

void 
pinouts_init()
{
	UPBIT(LED_DDR_PORT, LED_B_PIN);
	UPBIT(LED_DDR_PORT, LED_R_PIN);
	UPBIT(VALVE_DDR_PORT, VALVE_PIN);
}
//=======================================================================================
#define LIGHT_NIGHT 250 // default 300
#define LIGHT_DELTA 20

#define LIGHT_PIN 5

//***************************
//***************************
//**                       **
//**  LOGIC  LOGIC  LOGIC  **
//**  LOGIC  LOGIC  LOGIC  **
//**                       **
//***************************
//***************************

#define HUMIDITY_NEED_WATER 850
#define HUMIDITY_STOP_WATER 600
#define HUMIDITY_100_PERSENT 300
#define HUMIDITY_DELTA 50

#define HUMIDITY_PIN_0 1
#define HUMIDITY_PIN_1 2
#define HUMIDITY_PIN_2 3
#define HUMIDITY_PIN_3 4

#define MEASURING_COUNTER 100

#define WAIT_TIME 5 // in seconds (600 default)
#define WATERING_TIME 5 // in seconds (600 default)

#define MBUS_ID 0x01
#define MBUS_SPEED 19200
#define MBUS_PARITY MB_PAR_NONE


#define MBUS_POS_HUMIDITY_0    1
#define MBUS_POS_HUMIDITY_1    2
#define MBUS_POS_HUMIDITY_2    3
#define MBUS_POS_HUMIDITY_3    4
#define MBUS_POS_HUMIDITY_AVGR 5
#define MBUS_POS_LIGGHT 	   6
#define MBUS_POS_VALVE_STATE   7
#define MBUS_POS_MACHINE_STATE 8

#define MBUS_POS_HUMIDITY_NEED_WATER       11
#define MBUS_POS_HUMIDITY_STOP_WATER       12
#define MBUS_POS_HUMIDITY_DELTA            13
#define MBUS_POS_LIGHT_NIGHT               14
#define MBUS_POS_LIGHT_DELTA               15
#define MBUS_POS_COUNT_DOWN_NEXT_CHECK     16
#define MBUS_POS_MEASURED_AVERAGE_HUMIDITY 17
#define MBUS_POS_MEASURED_AVERAGE_LIGHT    18

#define MBUS_POS_STOP				       20


//======================================================================================
uint16_t getSummeredHumidity()
{
  uint16_t summ_humidity = 0;
  usRegInputBuf[MBUS_POS_HUMIDITY_0] = readAdc(HUMIDITY_PIN_0);
  summ_humidity += usRegInputBuf[MBUS_POS_HUMIDITY_0];
  usRegInputBuf[MBUS_POS_HUMIDITY_1] = readAdc(HUMIDITY_PIN_1);
  summ_humidity += usRegInputBuf[MBUS_POS_HUMIDITY_1];
  usRegInputBuf[MBUS_POS_HUMIDITY_2] = readAdc(HUMIDITY_PIN_2);
  summ_humidity += usRegInputBuf[MBUS_POS_HUMIDITY_2];
  usRegInputBuf[MBUS_POS_HUMIDITY_3] = readAdc(HUMIDITY_PIN_3);
  summ_humidity += usRegInputBuf[MBUS_POS_HUMIDITY_3];
  
  summ_humidity /= 4;

  usRegInputBuf[MBUS_POS_HUMIDITY_AVGR] = summ_humidity;
  return summ_humidity;
}
//======================================================================================
unsigned int getLight()
{
  usRegInputBuf[MBUS_POS_LIGGHT] = readAdc(LIGHT_PIN);
  return usRegInputBuf[MBUS_POS_LIGGHT];
}
//======================================================================================
void valveClose()
{
  usRegInputBuf[MBUS_POS_VALVE_STATE] = VALVE_CLOSE_STATE;
  valve_close();
}
//======================================================================================
void valveOpen()
{
  usRegInputBuf[MBUS_POS_VALVE_STATE] = VALVE_OPEN_STATE;
  valve_open();
}
//======================================================================================
char is_need_watering(unsigned int humidity, unsigned int light)
{
  if (light < usRegInputBuf[MBUS_POS_LIGHT_NIGHT]){
    return 0; // to light for watering
  }

  if (humidity > usRegInputBuf[MBUS_POS_HUMIDITY_NEED_WATER]){
    return 1;
  }

  if (humidity <= usRegInputBuf[MBUS_POS_HUMIDITY_STOP_WATER]){
    return 0;
  }
  
  return 2;
}
//=======================================================================================
enum States {WAITING = 11, WATERING = 22, MEASURING = 33};
void 
poll_machine()
{
  static unsigned char state = MEASURING;
  usRegInputBuf[MBUS_POS_MACHINE_STATE] = state;
  switch (state){
    case WAITING:
    {
      static unsigned int wait_counter = 0;
      if (wait_counter == 0) valveClose();
      wait_counter++;
      if (wait_counter >= WAIT_TIME){
        wait_counter = 0;
        state = MEASURING;
      }
    }
      break;
      
    case WATERING:
    {
      static unsigned int watering_counter = 0;
      if (watering_counter == 0) valveOpen();
      watering_counter++;
      if (watering_counter >= WATERING_TIME){
        watering_counter = 0;
        state = MEASURING;
      }
    }
      break;
      
    case MEASURING:
    {
      static unsigned int measuring_counter = MEASURING_COUNTER;
      static unsigned long averaged_humidity = 0;
      static unsigned long averaged_light = 0;
	  usRegInputBuf[MBUS_POS_COUNT_DOWN_NEXT_CHECK] = measuring_counter;

      if (measuring_counter-- > 0){
        averaged_humidity += getSummeredHumidity();
        averaged_light += getLight();
        state = MEASURING;
      }else{
        unsigned int humidity = averaged_humidity / MEASURING_COUNTER;
        unsigned int light = averaged_light / MEASURING_COUNTER;
        usRegInputBuf[MBUS_POS_MEASURED_AVERAGE_HUMIDITY] = humidity;
		usRegInputBuf[MBUS_POS_MEASURED_AVERAGE_LIGHT] = light;
        measuring_counter = MEASURING_COUNTER;
        averaged_humidity = 0;
        averaged_light = 0;

        if (is_need_watering(humidity, light) == 1){
			state = WATERING;
		} 
        if (is_need_watering(humidity, light) == 0){
			state = WAITING;
		} 
      }    
    }
      break;
    default:
      break;
  }
  	
}
//=======================================================================================
// 					Start implementation 
//=======================================================================================
int
main( void )
{
    const UCHAR     ucSlaveID[] = { 0xAA, 0xBB, 0xCC };
    eMBErrorCode    eStatus;

	pinouts_init();
	adc_init();
	 
	//                 type    mbaddress    port  boud        parity
    eStatus = eMBInit( MB_RTU, MBUS_ID,     0,    MBUS_SPEED, MBUS_PARITY );
    eStatus = eMBSetSlaveID( 0x34, TRUE, ucSlaveID, 3 );
    sei(  );

    // Enable the Modbus Protocol Stack. 
    eStatus = eMBEnable(  );

	usRegInputBuf[MBUS_POS_HUMIDITY_NEED_WATER] = HUMIDITY_NEED_WATER;
	usRegInputBuf[MBUS_POS_HUMIDITY_STOP_WATER] = HUMIDITY_STOP_WATER;
	usRegInputBuf[MBUS_POS_HUMIDITY_DELTA] = HUMIDITY_DELTA;
	usRegInputBuf[MBUS_POS_LIGHT_NIGHT] = LIGHT_NIGHT;
	usRegInputBuf[MBUS_POS_LIGHT_DELTA] = LIGHT_DELTA;
	valveClose();

	#define LAZY_TIMER_DEF 80000
	uint32_t lazy_timer = LAZY_TIMER_DEF;
    for( ;; )
    {
        ( void )eMBPoll(  );
		if (lazy_timer-- <= 0) {
			poll_machine();
			lazy_timer = LAZY_TIMER_DEF;
			led_tagle(LED_R_PIN);
		}
		
		
        // Here we simply count the number of poll cycles. 
		//for (char i = 0; i <= 4; i++){
		//	usRegInputBuf[i] = readAdc(i+1);
		//}
		
    }
}
//=======================================================================================
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{	
	
  eMBErrorCode  eStatus = MB_ENOERR;
  if( ( usAddress == 1 ) && ( usNRegs <= REG_INPUT_NREGS ) ) {

  	  static USHORT x = 0;
	  USHORT * ref = pucRegBuffer;
      *ref++ = swap_byte(++x);
	  
	  for(char cc = 0; cc <= MBUS_POS_STOP; cc++){
		*ref++ = swap_byte(usRegInputBuf[cc]);  
	  }
	  
      //*ref++ = swap_byte(usRegInputBuf[0]);
      //*ref++ = swap_byte(usRegInputBuf[1]);
      //*ref++ = swap_byte(usRegInputBuf[2]);
      //*ref++ = swap_byte(usRegInputBuf[3]);
      //*ref++ = swap_byte(usRegInputBuf[4]);

	  led_tagle(LED_B_PIN);
  }
  else {
    eStatus = MB_ENOREG;
  }
  return eStatus;
}
//=======================================================================================
eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
	
  eMBErrorCode  eStatus = MB_ENOERR;
  if( ( usAddress == 1 ) && ( usNRegs <= REG_INPUT_NREGS ) ) {
    if( eMode == MB_REG_READ ) {

  	  USHORT * ref = pucRegBuffer;
	  for(char cc = 0; cc <= MBUS_POS_STOP; cc++){
		*ref++ = swap_byte(usRegInputBuf[cc]);  
	  }
    }
    else {
		USHORT * ref = pucRegBuffer;
	    usRegInputBuf[MBUS_POS_LIGHT_NIGHT] = swap_byte(*ref++);
	    usRegInputBuf[MBUS_POS_HUMIDITY_NEED_WATER] = swap_byte(*ref++);
	    usRegInputBuf[MBUS_POS_HUMIDITY_STOP_WATER] = swap_byte(*ref++);
		led_tagle(LED_R_PIN);
   
      //pucRegBuffer++;
      //PORTB = ~(*pucRegBuffer++);
    }
  }
  else {
    eStatus = MB_ENOREG;
	//eStatus = MB_ENORES;
  }
  return eStatus;
}
//=======================================================================================
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}
//=======================================================================================
eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}
//=======================================================================================
