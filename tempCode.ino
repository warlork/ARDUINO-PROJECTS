/* Temperature-Humidity-Dewpoint Monitor
This sketch gathers temperature and humidity data via a DHT22 sensor,
and also calculates dew point based on those measurements.
*/
//#include <SoftwareSerial.h>
#include <stdlib.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <stdio.h>
//#include <sql.h>
//#include <sqlext.h>
#include <SPI.h>
#include <sha1.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ctype.h>
#include <mysql.h>
//#include <time.h>

#define REDLITE 5
#define GREENLITE 3
#define BLUELITE 6
#define DHT22_ERROR_VALUE -99.5
#define DHT22_PIN 4

LiquidCrystal lcd(7, 8, 9, A2, A1, A0);
int brightness = 0;
byte mac_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress server_addr(192,168,137,2);
IPAddress ip(192,168,137,3);
char user[] = "efodela";
char password[] = "deldan2k";
float tempC;
float tempF;
float humid;
float dewPoint;

typedef enum {
    DHT_ERROR_NONE = 0,
    DHT_BUS_HUNG,
    DHT_ERROR_NOT_PRESENT,
    DHT_ERROR_ACK_TOO_LONG,
    DHT_ERROR_SYNC_TIMEOUT,
    DHT_ERROR_DATA_TIMEOUT,
    DHT_ERROR_CHECKSUM,
    DHT_ERROR_TOOQUICK
} DHT22_ERROR_t;

class DHT22 {
            private:
                    uint8_t _bitmask;
                    volatile uint8_t *_baseReg;
            
            unsigned long _lastReadTime;
            float _lastHumidity;
            float _lastTemperature;
            public:
                  DHT22(uint8_t pin);
                  DHT22_ERROR_t readData(void);
            float getHumidity();
            float getTemperatureC();
            void clockReset();
};

byte customChar[8] ={
                     0b01111110, // _____
                     0b01110010, // _____
                     0b01111110, // _____
                     0b00000000, // _____
                     0b00000000, // _____
                     0b00000000, // _____
                     0b00000000, // _____
                     0b00000000  // _____ 
                    };
  
// Setup a DHT22 instance
DHT22 myDHT22(DHT22_PIN);
#define SerialIn 2
#define SerialOut 3
#define WDelay 900

Connector my_conn;        // The Connector/Arduino reference

 //char INSERT_TEXT[512] = "INSERT INTO templock.temp (celcius, fahrenheit) VALUES ('";
char buffer[512];
char one[512];
char two[512];

void checkSqlConn(){
  Serial.println("Trying to connect to database ....");
  if (my_conn.mysql_connect(server_addr, 3306, user, password)) {
          delay(1000);
          Serial.println("Connected to the Database!!!");
  }
  else{
   Serial.println("Connection failed.");
  }
}

void insertSQL(){
  char INSERT_TEXT[128];// = "INSERT INTO templock.temp (celcius) VALUES ('";
  char tBuffer[16]; char fBuffer[16]; char hBuffer[16]; char dBuffer[16];
  dtostrf(tempC,5,2,tBuffer);
  dtostrf(tempF,5,2,fBuffer);
  dtostrf(humid,5,2,hBuffer); 
  dtostrf(dewPoint,5,2,dBuffer);
  
  sprintf(INSERT_TEXT,"INSERT INTO templock.temp (CELCIUS, FAHRENHEIT, HUMIDITY, DEW) VALUES ('%s','%s','%s','%s')",tBuffer,fBuffer,hBuffer,dBuffer);

  my_conn.cmd_query(INSERT_TEXT);
}

void setup(void){
        	Ethernet.begin(mac_addr, ip);
        	Serial.begin(9600);
        	delay(2000);
        	lcd.begin(16,2);
        	delay(2000);
                Serial.print("Device Ip Address: ");
                Serial.print(Ethernet.localIP()); Serial.println(); //display the local ip address of the device
                checkSqlConn();
                lcd.print("STARTING UP...");
                delay(2000);
                lcd.createChar(1, customChar);
                pinMode(REDLITE, OUTPUT);
                pinMode(GREENLITE, OUTPUT);
                pinMode(BLUELITE, OUTPUT);
                brightness = 100;
                setBacklight(0,255,0);
}

void loop(void){
    
                DHT22_ERROR_t errorCode;
                delay(2000);
                errorCode = myDHT22.readData();
                lcd.print(errorCode);
    
    switch(errorCode){
                    case DHT_ERROR_NONE:
        
                            tempC = myDHT22.getTemperatureC(); delay(WDelay); Serial.print("Celcius: "); Serial.print(tempC);
                            tempF = (tempC*1.8)+32; delay(WDelay); Serial.print("Fahrenheit: "); Serial.print(tempF);
							humid = myDHT22.getHumidity(); delay(WDelay);Serial.print("Humidity "); Serial.print(humid);
							dewPoint = calculateDewpoint(tempC, humid); delay(WDelay); Serial.print("Dew Point: "); Serial.print(dewPoint);
                              
                            lcd.clear();
                            lcd.setCursor(4,0);
                            lcd.print("Temp: ");
                            lcd.print(tempC);
                            lcd.write(1);
                            lcd.print("C");
                              
							insertSQL(); //Inserting data intp SQL Database
                              
                              if (tempC >= 26){setBacklight(255,0,0);}
                              else if(tempC <= 23){setBacklight(0,0,255);}
                              else {setBacklight(0,255,0);}
                        
                              for (int positionCounter = 0; positionCounter < 16; positionCounter++) {
                                        
                                          lcd.setCursor(0,1);
                                          lcd.scrollDisplayLeft(); 
                                          lcd.print("Humidity: ");
                                          lcd.print(humid);
                                          lcd.print("%");  
                                       
                                           lcd.print(" Dew Point: ");
                                           lcd.print(dewPoint);
                                           lcd.println("d");
                                           delay(500);
                               } 
                  break;
        
              case DHT_ERROR_CHECKSUM:
                lcd.print("Error Cheksum");
                break;
              case DHT_BUS_HUNG:
                lcd.print("Bus Hung");
                break;
              case DHT_ERROR_NOT_PRESENT:
                lcd.print("Not Present");
                break;
              case DHT_ERROR_ACK_TOO_LONG:
                break;
              case DHT_ERROR_SYNC_TIMEOUT:
                break;
              case DHT_ERROR_DATA_TIMEOUT:
                break;
              case DHT_ERROR_TOOQUICK:
                break;
              }
}

void setBacklight(uint8_t r, uint8_t g, uint8_t b) {
        // normalize the red LED - its brighter than the rest!
        r = map(r, 0, 255, 0, 100);
        g = map(g, 0, 255, 0, 150);
        r = map(r, 0, 255, 0, 100);
        g = map(g, 0, 255, 0, 100);
        b = map(b, 0, 255, 0, 100);
        // common anode so invert!
        r = map(r, 0, 255, 255, 0);
        g = map(g, 0, 255, 255, 0);
        b = map(b, 0, 255, 255, 0);

        analogWrite(REDLITE, r);
        analogWrite(GREENLITE, g);
        analogWrite(BLUELITE, b);
}

float calculateDewpoint(float T, float RH){
    // approximate dewpoint using the formula from wikipedia's article on dewpoint
    float dp = 0.0;
    float gTRH = 0.0;
    float a = 17.271;
    float b = 237.7;
    gTRH = ((a*T)/(b+T))+log(RH/100);
    dp = (b*gTRH)/(a-gTRH);
    return dp;
}



