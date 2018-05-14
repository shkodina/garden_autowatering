#include  <avr/sleep.h>
#include  <avr/wdt.h>
#include "ModbusRtu.h"

#define LOG_MODE

#define MODBUS_ID   1

#define LIGHT_NIGHT 800 // default 300
#define LIGHT_DELTA 20

#define LIGHT_PIN A5

#define HUMIDITY_NEED_WATER 600
#define HUMIDITY_STOP_WATER 400
#define HUMIDITY_100_PERSENT 300
#define HUMIDITY_DELTA 50

#define HUMIDITY_PIN_0 A0
#define HUMIDITY_PIN_1 A1
#define HUMIDITY_PIN_2 A2
#define HUMIDITY_PIN_3 A3

#define MEASURING_COUNTER 100

#define WAIT_TIME 5 // in seconds (600 default)
#define WATERING_TIME 5 // in seconds (600 default)

#define VALVE_PIN 5
#define VALVE_OPEN HIGH
#define VALVE_CLOSE LOW

//======================================================================================
void setup() {
  Serial.begin(9600);
  wdt_enable (WDTO_8S); 
}
//======================================================================================
void valveClose()
{
  #ifdef LOG_MODE
  Serial.println("(!)Stop watering my tomatos");
  #endif
  digitalWrite(VALVE_PIN, VALVE_CLOSE);
}
//======================================================================================
void valveOpen()
{
  #ifdef LOG_MODE
  Serial.println("(?)Lets water my tomatos");
  #endif
  digitalWrite(VALVE_PIN, VALVE_OPEN);
}
//======================================================================================
unsigned int getLight()
{
  #ifdef LOG_MODE
  Serial.print("\t\t\t\t\t\t\t\tDay light: ");
  Serial.println(analogRead(LIGHT_PIN));
  #endif
  return analogRead(LIGHT_PIN);
}
//======================================================================================
unsigned int getSummeredHumidity()
{
  unsigned int summ_humidity = 0;
  summ_humidity += analogRead(HUMIDITY_PIN_0);
  summ_humidity += analogRead(HUMIDITY_PIN_1);
  summ_humidity += analogRead(HUMIDITY_PIN_2);
  summ_humidity += analogRead(HUMIDITY_PIN_3);
  summ_humidity /= 4;

  #ifdef LOG_MODE
  {
    char str[128];
    sprintf(str, "\tH0: %d\tH1: %d\tH2: %d\tH3: %d", analogRead(HUMIDITY_PIN_0), analogRead(HUMIDITY_PIN_1), analogRead(HUMIDITY_PIN_2), analogRead(HUMIDITY_PIN_3));
    Serial.println(str);
  }
  #endif
  
  return summ_humidity;
}
//======================================================================================
bool is_need_watering(unsigned int humidity, unsigned int light)
{
  #ifdef LOG_MODE
  {
    char str[128];
    sprintf(str, "Average H: %d L: %d", humidity, light);
    Serial.println(str);
  }
  #endif  
  
  if (light > LIGHT_NIGHT){
    #ifdef LOG_MODE
      Serial.println("Light TOO BRIGHT");
    #endif
    return false; // to light for watering
  }

  if (humidity > HUMIDITY_NEED_WATER){
    #ifdef LOG_MODE
      Serial.println("OVER Humidity TO HIGH. Need Water");
    #endif      
    return true;
  }

  if (humidity < HUMIDITY_STOP_WATER){
    #ifdef LOG_MODE
      Serial.println("OVER Humidity TO LOW. Need time to dry");
    #endif      
    return false;
  }
}
//======================================================================================
enum States {WAITING, WATERING, MEASURING};
void loop() {
  static unsigned char state = MEASURING;
  
  switch (state){
    case WAITING:
    {
      static unsigned int wait_counter = 0;
      if (wait_counter == 0) valveClose();
      delay(1000); // 
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
      delay(1000); // 
      watering_counter++;
      if (watering_counter >= WATERING_TIME){
        watering_counter = 0;
        state = MEASURING;
      }
    }
      break;
      
    case MEASURING:
    {
      static unsigned int measuring_counter = 0;
      static unsigned long averaged_humidity = 0;
      static unsigned long averaged_light = 0;

      if (measuring_counter <= MEASURING_COUNTER){
        averaged_humidity += getSummeredHumidity();
        averaged_light += getLight();
        measuring_counter++;
        state = MEASURING;
      }else{
        unsigned int humidity = averaged_humidity / measuring_counter;
        unsigned int light = averaged_light / measuring_counter;
        
        measuring_counter = 0;
        averaged_humidity = 0;
        averaged_light = 0;

        state = (is_need_watering(humidity, light)) ? WATERING : WAITING;
      }    
    }
      break;
    default:
      break;
  }
  
  wdt_reset();  // pat the dog
}
//======================================================================================

