/*Terrarium Control System version 2.0 

Written by Justin Garcia
for questions please contact @ justin.d.garcia@gmail.com
Our controlled climate is meant to simulate the conditions of the rainforest
The conditions:
Temperature Range: 68-93 in degrees F
                   20-34 in degrees C
Humidity Range:    77-88 % humidity

Sensors: 2 X DHT_11 (does both temperature and humidity)
LM35 temperature sensor---------went unused, I found it became innacurate as more devices were added to the arduino, even when powered seperately.

Relays were used to power the the three actuators
  A FAN to drive out heat
  A HEATLAMP (125 WATT) to drive in heat, this also drives out humidity in an open system
  A Humidifier to add moisture to the air
*/


/****************************************************/
/****************************************************/
//libraries and library based initializations

#include <dht.h>    // This is the library for the humidity and temperature sensor
                    //This library allows the sensor to receive and transmit data over one wire.
                    
dht DHT;            //This line initializes the temperature/humidity sensor as an object
dht DHT1;           // another object

#include <Wire.h>   // This Arduino included library sets A4 and A5 for the I2C chip
                    // A4 is SDA(data line) and A5 is SCL (clock line) and handles communications to the I2C on the LCD screen
                    
#include <LiquidCrystal_I2C.h> //LCD display Library made by F Malpartida, much better than the stock library.

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3,POSITIVE);
//This assigns the pins on the lcd i2c chip that reduces the lcd pins to four on the Arduino
//note: 0x27 is the serial address of the screen, pin 3 writes the letters to the screen, POSITIVE turns on the backlight.
/***************************************************/
//constants
const int UP_BUTTON = 7;     //used to raise values on the display
const int DOWN_BUTTON = 4;   //used to lower values on the display
const int MAIN_BUTTON = 12;  //used to switch between max/min setting on certain menus
const int ENTER_BUTTON = 8;  //used to change between in order: ten's digit--> one's digit--> set value to chosen variable on screen
int       POT_MENU = A1;     //used to switch between menus
                             //menu order: CURRENT STATE--> USER SET TEMP--> USER SET HUMIDITY--> RANGES--> RELAYS
                             //CURRENT STATE: shows the present temperature and humidiity of the terrarium.
                             //USER SET TEMP: lets the user set a minimum and maximum temperature.
                             //USER SET HUMIDITY; lets the user set a mimimum and maximum humidity.
                             //RANGES: shows all user set parameters for temp and humidity at once
                             //RELAYS: shows which relays are active, used to detect broken relays or broken FAN< HEATLAMP< HUMIDIFIER.
//#define TempLM35 A0 
//LM35 output voltage has a linear relation with the Celsius temperature, output of 0 v when 0 ℃, 
//every increase 1 ℃, the output voltage increase 10 mv
#define DHT_Pin 9            //This is a temperature/humidity sensor
#define DHT_Pin2 10          //This is a temperature/humidity sensor

//Assigning the RELAYS
const int FAN = 3;          
const int HUMIDIFIER = 11;
const int HEAT_LAMP = 2;

//Defining ON and OFF
int ON = LOW;
int OFF = HIGH;
/**************************************************/
//variables
int MAIN_BUTTON_VALUE = 0;         //This starts the menu screen for TEMP and HUMIDITY on user setting the minimum value desired
int ENTER_BUTTON_VALUE = 0;        //This starts the menu screen for TEMP and HUMIDITY on user setting the ten's digit value

int TEMP_USER_MIN_DIGIT_10s = 0;  //The user can set the temperature and humidity to any value 00-99.
int TEMP_USER_MIN_DIGIT_1s = 0;
int TEMP_AVERAGE_MIN = 20;        //minimum is set to start at 20 degrees F

int TEMP_USER_MAX_DIGIT_10s = 0;
int TEMP_USER_MAX_DIGIT_1s = 0;
int TEMP_AVERAGE_MAX = 99;       //maximum is started at 99 degrees F
                                 //NOTE: this wide range means that the heat lamp will turn on when the arduino is restarted

int HUMIDITY_USER_MIN_DIGIT_10s = 0;
int HUMIDITY_USER_MIN_DIGIT_1s = 0;
int HUMIDITY_AVERAGE_MIN = 20;      //started with inital value so humidifier isnt on

int HUMIDITY_USER_MAX_DIGIT_10s = 0;
int HUMIDITY_USER_MAX_DIGIT_1s = 0;
int HUMIDITY_AVERAGE_MAX = 90;     //Started with initial value so humidifier isnt on
                                   //NOTE: this wide range means that the humidifier will not turn on when the arduino is restarted
                                   //NOTE: THESE combine to form a visual check that the Arduino is functioning correctly.
long LAST_DEBOUNCE_TIME = 0;       //This is the debounce time for all buttons so that one button press is registered
long LAST_DEBOUNCE_DHT = 0;        //Both sensors need their own debounce time so they can recover and give good readings
long LAST_DEBOUNCE_DHT2 = 0;
long DEBOUNCE_DELAY = 150;        //Typical time a button is pressed by the user in milliseconds
long DEBOUNCE_DELAY2 = 4000;      //Temperature/Humidity sensors to be pinged no more than once every 2 seconds

float TEMP_AVERAGE_SYSTEM = 0;    //The DHT_11 temperature and Humidity sensor gives readings of float type
float HUMIDITY_AVERAGE_SYSTEM = 0;

float TEM_1 = 0;                  //this is the temperature read by the first sensor
float TEM_2 = 0;                  //this is the temperature read by the second sensor
//long LM_VAL = 0;                //the temperature reading of the LM35 sensor, a long value, to be converted to degrees C.
float HUM_1 = 35;                 //Started with initial value so humidifier isnt on, value of the first sensor
float HUM_2 = 35;                 //Ensures average humidity is started at 35% since values of HUM_1 and HUM-2 are averaged, value of the second sensor.
//float TEM_3 = 0;                //would be the calculated value of the LM35 sensor

void setup() 
{
 lcd.begin(16,2);                //The LCD screen used has 2 ROWS of 16 CHARACTERS
 Serial.begin(9600);             //Opens serial for debugging purposes.
 initial();                      //see intial function
 pinMode(UP_BUTTON,INPUT_PULLUP);//All buttons set with pullup resistor on. This means that when the button is pushed it is grounded and reads LOW 
 pinMode(DOWN_BUTTON,INPUT_PULLUP);//This also allows me to not need resistors on my breadboard for the buttons.
 pinMode(MAIN_BUTTON,INPUT_PULLUP);
 pinMode(ENTER_BUTTON,INPUT_PULLUP);
 pinMode(HUMIDIFIER, OUTPUT); //initialize relay as an output, next commands too.
 pinMode(HEAT_LAMP, OUTPUT);
 pinMode(FAN, OUTPUT);
}

void initial()//Instructs the user to choose boundary limits.
{
lcd.clear();
lcd.setCursor(0,0);  lcd.print("Please choose"); 
lcd.setCursor(0,1);   lcd.print("boundary limits.");   //limits in place of conditions
delay(3000);
lcd.clear();
}

void MAIN()//This code runs the main button. It switches between the maximum and minimum setting screens in the temperature and humidity menus.
{          //used for boolean logic . Main button has a value of 0 or 1.
int MAIN_BUTTON_STATE = digitalRead(MAIN_BUTTON);
if((millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)
{
    if (MAIN_BUTTON_STATE == LOW)
      {
      MAIN_BUTTON_VALUE++;
      LAST_DEBOUNCE_TIME = millis();
      }
    if (MAIN_BUTTON_VALUE > 1)
      {
      MAIN_BUTTON_VALUE = 0;
      }
}
}

void ENTER()//This code switches the temperature and humidity menus between setting the ten's place digit and one's place digit
{           //once these are set another press stores the value to the correct variable.
            //used for boolean logic. Enter button has a value of 0,1, or 2.
int ENTER_BUTTON_STATE = digitalRead(ENTER_BUTTON);
if((millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)
{
    if (ENTER_BUTTON_STATE == LOW)
      {
      ENTER_BUTTON_VALUE++;
      LAST_DEBOUNCE_TIME = millis();
      }
    if (ENTER_BUTTON_VALUE > 2)
      {
      ENTER_BUTTON_VALUE = 0;
      }
}
}

void TEMP_SET()
/*This function allows the user to set the temperature ranges*/
{
MAIN();//when this function is running the main button is active
ENTER();//when this function is running the enter button is active
int UP_BUTTON_STATE = digitalRead(UP_BUTTON); //checks is up button is pressed
int DOWN_BUTTON_STATE = digitalRead(DOWN_BUTTON);//or if the down button is pressed
if ((ENTER_BUTTON_VALUE == 0 | ENTER_BUTTON_VALUE == 1) && MAIN_BUTTON_VALUE == 0)//As long as the main button is in 0 state
{
lcd.clear();
lcd.setCursor(0,0);   lcd.print("1: Choose Temp");
lcd.setCursor(3,1);   lcd.print("MIN");
lcd.setCursor(7,1);   lcd.print(TEMP_USER_MIN_DIGIT_10s);//the tens place is printed
lcd.setCursor(8,1);   lcd.print(TEMP_USER_MIN_DIGIT_1s);//the on's place is printed, this allowed me to chage them seperately.
lcd.setCursor(9,1);   lcd.print("F");

if (ENTER_BUTTON_VALUE == 0 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)//when the up button is pushed the ten's digit goes up until 9 then goes back to zero
{
  TEMP_USER_MIN_DIGIT_10s++; //the ten's place of the minimum digit goes up one.
  LAST_DEBOUNCE_TIME = millis();//resets the debounce time for the up button
}
if (TEMP_USER_MIN_DIGIT_10s > 9)
{
  TEMP_USER_MIN_DIGIT_10s = 0;//cycles over the number if it goes back nine
}
else if (ENTER_BUTTON_VALUE == 0 && DOWN_BUTTON_STATE == LOW)
{
  TEMP_USER_MIN_DIGIT_10s--;// the ten's place of the minimum digit goes down.
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MIN_DIGIT_10s < 0)
{
  TEMP_USER_MIN_DIGIT_10s = 9;//when the digit goes below zero the value is upcycled to nine.
}

//the rest of the TEMPSET function uses permutations of the above code to set:
//the one's digit of the minimum temperature
//the ten's digit of the maximum temperature
//the one's digit of the maximum temperature

if (ENTER_BUTTON_VALUE == 1 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)//when the enter button is pressed once the up button then changes the one's place digit

{
  TEMP_USER_MIN_DIGIT_1s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MIN_DIGIT_1s > 9)
{
  TEMP_USER_MIN_DIGIT_1s = 0;
}
else if (ENTER_BUTTON_VALUE ==1 && DOWN_BUTTON_STATE == LOW)
{
  TEMP_USER_MIN_DIGIT_1s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MIN_DIGIT_1s < 0)
{
  TEMP_USER_MIN_DIGIT_1s = 9;
}
}
else if (ENTER_BUTTON_VALUE == 2 && MAIN_BUTTON_VALUE == 0)//if the enter button is pressed twice the minimum temperature is set.
{
  lcd.clear();
  lcd.setCursor(0,0);   lcd.print("Min temp is set");
  lcd.setCursor(0,1);   lcd.print(TEMP_USER_MIN_DIGIT_10s);
  lcd.setCursor(1,1);   lcd.print(TEMP_USER_MIN_DIGIT_1s);
  lcd.setCursor(3,1);   lcd.print("degrees F");
  TEMP_AVERAGE_MIN = ((TEMP_USER_MIN_DIGIT_10s * 10) + TEMP_USER_MIN_DIGIT_1s);
  Serial.print("Minimun sytem temperature is set to ");
  Serial.println(TEMP_AVERAGE_MIN); 
}
else if ((ENTER_BUTTON_VALUE == 0 | ENTER_BUTTON_VALUE == 1) && MAIN_BUTTON_VALUE == 1)//if the main button is pressed once the user can change the max temperature
{
lcd.clear();
lcd.setCursor(0,0);   lcd.print("1: Choose Temp");
lcd.setCursor(3,1);   lcd.print("MAX");
lcd.setCursor(7,1);   lcd.print(TEMP_USER_MAX_DIGIT_10s);
lcd.setCursor(8,1);   lcd.print(TEMP_USER_MAX_DIGIT_1s);
lcd.setCursor(9,1);   lcd.print("F");

if (ENTER_BUTTON_VALUE == 0 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)//sets the ten's digit of max temperature, up
{
  TEMP_USER_MAX_DIGIT_10s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MAX_DIGIT_10s > 9)
{
  TEMP_USER_MAX_DIGIT_10s = 0;
}
else if (ENTER_BUTTON_VALUE == 0 && DOWN_BUTTON_STATE == LOW)//set the ten's digit of max temperature, down
{
  TEMP_USER_MAX_DIGIT_10s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MAX_DIGIT_10s < 0)
{
  TEMP_USER_MAX_DIGIT_10s = 9;
}

if (ENTER_BUTTON_VALUE == 1 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)//sets the one's digit of max temperature, up
{
  TEMP_USER_MAX_DIGIT_1s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MAX_DIGIT_1s > 9)
{
  TEMP_USER_MAX_DIGIT_1s = 0;
}
else if (ENTER_BUTTON_VALUE ==1 && DOWN_BUTTON_STATE == LOW)//sets the one's digit of max temperature, down
{
  TEMP_USER_MAX_DIGIT_1s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (TEMP_USER_MAX_DIGIT_1s < 0)
{
  TEMP_USER_MAX_DIGIT_1s = 9;
}
}
else if (ENTER_BUTTON_VALUE == 2 && MAIN_BUTTON_VALUE == 1)//sets the max tmeperature variable
{
  lcd.clear();
  lcd.setCursor(0,0);   lcd.print("Max temp is set");
  lcd.setCursor(0,1);   lcd.print(TEMP_USER_MAX_DIGIT_10s);
  lcd.setCursor(1,1);   lcd.print(TEMP_USER_MAX_DIGIT_1s);
  lcd.setCursor(3,1);   lcd.print("degrees F");
  TEMP_AVERAGE_MAX = ((TEMP_USER_MAX_DIGIT_10s * 10) + TEMP_USER_MAX_DIGIT_1s);
  Serial.print("Maximum sytem temperature is set to ");
  Serial.println(TEMP_AVERAGE_MAX); 
}
}

void HUMIDITY_SET()
/* HUMIDITY_SET works exactly the same way as TEMPSET, except that the varaibles being written to are different and the word TEMP has been replaced by HUMIDITY*/
{
ENTER();
MAIN();
int UP_BUTTON_STATE = digitalRead(UP_BUTTON);
int DOWN_BUTTON_STATE = digitalRead(DOWN_BUTTON);
if ((ENTER_BUTTON_VALUE == 0 | ENTER_BUTTON_VALUE == 1) && MAIN_BUTTON_VALUE == 0)
{
lcd.clear();
lcd.setCursor(0,0);   lcd.print("2: Set Humidity");
lcd.setCursor(3,1);   lcd.print("MIN");
lcd.setCursor(7,1);   lcd.print(HUMIDITY_USER_MIN_DIGIT_10s);
lcd.setCursor(8,1);   lcd.print(HUMIDITY_USER_MIN_DIGIT_1s);
lcd.setCursor(9,1);   lcd.print("%");

if (ENTER_BUTTON_VALUE == 0 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)
{
  HUMIDITY_USER_MIN_DIGIT_10s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MIN_DIGIT_10s > 9)
{
  HUMIDITY_USER_MIN_DIGIT_10s = 0;
}
else if (ENTER_BUTTON_VALUE == 0 && DOWN_BUTTON_STATE == LOW)
{
  HUMIDITY_USER_MIN_DIGIT_10s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MIN_DIGIT_10s < 0)
{
  HUMIDITY_USER_MIN_DIGIT_10s = 9;
}

if (ENTER_BUTTON_VALUE == 1 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)
{
  HUMIDITY_USER_MIN_DIGIT_1s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MIN_DIGIT_1s > 9)
{
  HUMIDITY_USER_MIN_DIGIT_1s = 0;
}
else if (ENTER_BUTTON_VALUE ==1 && DOWN_BUTTON_STATE == LOW)
{
  HUMIDITY_USER_MIN_DIGIT_1s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MIN_DIGIT_1s < 0)
{
  HUMIDITY_USER_MIN_DIGIT_1s = 9;
}
}
else if (ENTER_BUTTON_VALUE == 2 && MAIN_BUTTON_VALUE == 0)
{
  lcd.clear();
  lcd.setCursor(0,0);   lcd.print("Min humidity is");
  lcd.setCursor(0,1);   lcd.print(HUMIDITY_USER_MIN_DIGIT_10s);
  lcd.setCursor(1,1);   lcd.print(HUMIDITY_USER_MIN_DIGIT_1s);
  lcd.setCursor(3,1);   lcd.print("%");  
  HUMIDITY_AVERAGE_MIN = ((HUMIDITY_USER_MIN_DIGIT_10s * 10) + HUMIDITY_USER_MIN_DIGIT_1s);
}
else if ((ENTER_BUTTON_VALUE == 0 | ENTER_BUTTON_VALUE == 1) && MAIN_BUTTON_VALUE == 1)
{
lcd.clear();
lcd.setCursor(0,0);   lcd.print("2: Set Humidity");
lcd.setCursor(3,1);   lcd.print("MAX");
lcd.setCursor(7,1);   lcd.print(HUMIDITY_USER_MAX_DIGIT_10s);
lcd.setCursor(8,1);   lcd.print(HUMIDITY_USER_MAX_DIGIT_1s);
lcd.setCursor(9,1);   lcd.print("%");

if (ENTER_BUTTON_VALUE == 0 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)
{
  HUMIDITY_USER_MAX_DIGIT_10s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MAX_DIGIT_10s > 9)
{
  HUMIDITY_USER_MAX_DIGIT_10s = 0;
}
else if (ENTER_BUTTON_VALUE == 0 && DOWN_BUTTON_STATE == LOW)
{
  HUMIDITY_USER_MAX_DIGIT_10s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MAX_DIGIT_10s < 0)
{
  HUMIDITY_USER_MAX_DIGIT_10s = 9;
}

if (ENTER_BUTTON_VALUE == 1 && UP_BUTTON_STATE == LOW && (millis()-LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY)
{
  HUMIDITY_USER_MAX_DIGIT_1s++;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MAX_DIGIT_1s > 9)
{
  HUMIDITY_USER_MAX_DIGIT_1s = 0;
}
else if (ENTER_BUTTON_VALUE ==1 && DOWN_BUTTON_STATE == LOW)
{
  HUMIDITY_USER_MAX_DIGIT_1s--;
  LAST_DEBOUNCE_TIME = millis();
}
if (HUMIDITY_USER_MAX_DIGIT_1s < 0)
{
  HUMIDITY_USER_MAX_DIGIT_1s = 9;
}
}
else if (ENTER_BUTTON_VALUE == 2 && MAIN_BUTTON_VALUE == 1)
{
  lcd.clear();
  lcd.setCursor(0,0);   lcd.print("Max humidity is");
  lcd.setCursor(0,1);   lcd.print(HUMIDITY_USER_MAX_DIGIT_10s);
  lcd.setCursor(1,1);   lcd.print(HUMIDITY_USER_MAX_DIGIT_1s);
  lcd.setCursor(3,1);   lcd.print("%");  
  HUMIDITY_AVERAGE_MAX = ((HUMIDITY_USER_MAX_DIGIT_10s * 10) + HUMIDITY_USER_MAX_DIGIT_1s);
}
}

void RANGES()// RANGES grabs the user set variables and collects them to display on the screen all at once.
{ 
  lcd.clear();
  lcd.setCursor(0,0);  lcd.print("RANGES");
  lcd.setCursor(7,0);  lcd.print("tem");
  lcd.setCursor(10,0);  lcd.print(TEMP_AVERAGE_MIN);
  lcd.setCursor(12,0);  lcd.print("-");
  lcd.setCursor(13,0);  lcd.print(TEMP_AVERAGE_MAX);
  lcd.setCursor(15,0);  lcd.print("F");
  lcd.setCursor(7,1);   lcd.print("hum");
  lcd.setCursor(10,1);  lcd.print(HUMIDITY_AVERAGE_MIN);
  lcd.setCursor(12,1);  lcd.print("-");
  lcd.setCursor(13,1);  lcd.print(HUMIDITY_AVERAGE_MAX);
  lcd.setCursor(15,1);  lcd.print("%");
}

void loop() 
{
/*This is the continuously running loop of the code. what this loop does is check the state of the potentiometer and maps the value to six cases. 
 * This allows different functions to run when the potentiometer is in different positions.  
 * The default case stands in for mapped value = 0.
  */
int MENU_TYPE = analogRead(POT_MENU);
MENU_TYPE = map(MENU_TYPE, 0, 1024, 0, 5);
switch (MENU_TYPE)
{
  case 1:
  TEMP_SET();
  break;
  case 2:
  HUMIDITY_SET();
  break;
  case 3:
  RANGES();   
  break;
  case 4:
  lcd.clear();
  lcd.setCursor(0,0);  lcd.print("ACTIVE RELAYS");
  lcd.setCursor(0,1);  lcd.print("F");
  lcd.setCursor(6,1);  lcd.print("H");
  lcd.setCursor(11,1);  lcd.print("L");
  if (digitalRead (FAN) == HIGH)
  {
    lcd.setCursor(2,1);  lcd.print("oN"); 
  }
  else
  {
    lcd.setCursor(2,1);  lcd.print("oFF");
  }
   if (digitalRead(HUMIDIFIER) == ON)
  {
    lcd.setCursor(7,1);  lcd.print("oN"); 
  }
  else
  {
    lcd.setCursor(7,1);  lcd.print("oFF");
  }
   if (digitalRead (HEAT_LAMP) == ON)
  {
    lcd.setCursor(12,1);  lcd.print("oN"); 
  }
  else
  {
    lcd.setCursor(12,1);  lcd.print("oFF");
  }
  break;
  default:
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("CURRENT STATES");
  lcd.setCursor(0,1); lcd.print("Temp");
  lcd.setCursor(5,1); lcd.print(TEMP_AVERAGE_SYSTEM,0);//the zero in these lines makes sure that the lcd only prints the whole number value of the float
  lcd.setCursor(7,1); lcd.print("F");
  lcd.setCursor(9,1); lcd.print("Hum");
  lcd.setCursor(13,1); lcd.print(HUMIDITY_AVERAGE_SYSTEM,0);
  lcd.setCursor(15,1); lcd.print("%");
  
  break;
}
/********************************************************************/
//conditions for terrarium

/*The next two if statements are debouncing mechanisms for the DHT_11 temperature and humidity sensors. 
 * the sensors can only be read every two seconds
 * if the sensor communicates that it cannot give a reading or a correct reading the values are ignored and a non-blocking
 * delay of 4 seconds is imposed until the sensor is checked again.
 */
if ((millis()- LAST_DEBOUNCE_DHT) > DEBOUNCE_DELAY2)  
{
   Serial.print("DHT11, \t");
  //read the value returned from sensor
  int chk = DHT.read11(DHT_Pin);
  switch (chk)
  {
    case DHTLIB_OK:  
    Serial.println("OK,\t");
    TEM_1 = DHT.temperature;
    HUM_1 = DHT.humidity;
    Serial.print("TEM_1  ");
    Serial.println(TEM_1);
    Serial.print("HUM_1  ");
    Serial.println(HUM_1);
    LAST_DEBOUNCE_DHT = millis();
    break;
    case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("Checksum error,\t"); 
    LAST_DEBOUNCE_DHT = millis();
    break;
    case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Time out error,\t");
    LAST_DEBOUNCE_DHT = millis(); 
    break;
    default: 
    Serial.println("Unknown error,\t");
    LAST_DEBOUNCE_DHT = millis(); 
    break;
  }
}

if ((millis()- LAST_DEBOUNCE_DHT2) > DEBOUNCE_DELAY2)
{
Serial.print("DHT11_2, \t");
  //read the value returned from sensor
  int chk2 = DHT1.read11(DHT_Pin2);
  switch (chk2)
  {
    case DHTLIB_OK:  
   Serial.println("OK,\t");
   TEM_2 = DHT1.temperature;
   HUM_2 = DHT1.humidity;
   Serial.print("TEM_2  ");
   Serial.println(TEM_2);
   Serial.print("HUM_2 ");
   Serial.println(HUM_2);
   LAST_DEBOUNCE_DHT2 = millis();
    break;
    case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("Checksum error,\t"); 
    break;
    case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Time out error,\t"); 
    break;
    default: 
    Serial.println("Unknown error,\t"); 
    break;
  }
} 

  
  
//  LM_VAL = analogRead(TempLM35);
//  TEM_3 = (LM_VAL * 0.0048828125 * 100);//5/1024=0.0048828125;1000/10=100 
//  Serial.print("TEM_3  ");
//  Serial.println(TEM_3);
// float tempaverageholder = (TEM_1 + TEM_2 + TEM_3)/3; 
float tempaverageholder = (TEM_1+TEM_2)/2;
float tempaveragesystem = 1.8*tempaverageholder+32;//this converts the centigrade value to degrees fahrenheit for the user
TEMP_AVERAGE_SYSTEM = tempaveragesystem;
   
Serial.print("TEMP_AVERAGE_SYSTEM  ");
Serial.println(TEMP_AVERAGE_SYSTEM);
float humidityaveragesystem = (HUM_1 + HUM_2)/2;
HUMIDITY_AVERAGE_SYSTEM = humidityaveragesystem;
Serial.print("HUMIDITY_AVERAGE_SYSTEM  ");
Serial.println(HUMIDITY_AVERAGE_SYSTEM);
 

/*****************************************************/
//Temperature conditions

if (TEMP_AVERAGE_SYSTEM > TEMP_AVERAGE_MAX)
{
 digitalWrite(HEAT_LAMP, OFF);
 digitalWrite(FAN, HIGH);//Fan relay seems to have opposite activator voltage, cannot use int ON
 Serial.println("meh");
}
else if (TEMP_AVERAGE_SYSTEM > TEMP_AVERAGE_MIN && TEMP_AVERAGE_SYSTEM < TEMP_AVERAGE_MAX)
{
 digitalWrite(HEAT_LAMP, ON);
 digitalWrite(FAN, LOW);
 Serial.println("meh2"); 
}
else if (TEMP_AVERAGE_SYSTEM < TEMP_AVERAGE_MIN)
{
 digitalWrite(HEAT_LAMP, ON);
 digitalWrite(FAN, LOW);
 Serial.println("meh3");
}
else
{
Serial.println("Temperature Error"); 
}

/***************************************************/
//humidity conditionals
if (HUMIDITY_AVERAGE_SYSTEM > HUMIDITY_AVERAGE_MAX)
{
 digitalWrite(HUMIDIFIER, OFF);
 Serial.println("Hi");
}
else if (HUMIDITY_AVERAGE_SYSTEM > HUMIDITY_AVERAGE_MIN && HUMIDITY_AVERAGE_SYSTEM < HUMIDITY_AVERAGE_MAX)
{
 digitalWrite(HUMIDIFIER, OFF);
 Serial.println("Hi2");
}
else if (HUMIDITY_AVERAGE_SYSTEM < HUMIDITY_AVERAGE_MIN)
{
 digitalWrite(HUMIDIFIER, ON);
 Serial.println("Hi3");
}
else
{
Serial.println("Humidity Error"); 
}
}

