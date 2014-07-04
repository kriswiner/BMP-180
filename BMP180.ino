/* BMP-180 Basic Example Code
 by: Kris Winer
 date: July 1, 2014
 license: Beerware - Use this code however you'd like. If you 
 find it useful you can buy me a beer some time.
 
 Demonstrate basic functionality of the BMP180 pressure sensor. 
 Sketch runs on the 3.3 V 8 MHz Pro Mini and the Teensy 3.1.
 
 SDA and SCL should have external pull-up resistors (to 3.3V).
 10k resistors are on the BMP-180 breakout board.
 
 Hardware setup:
 BMP180 Breakout --------- Arduino
 VDD --------------------- 3.3V
VDDIO -------------------- 3.3V
 SDA ----------------------- A4
 SCL ----------------------- A5
 GND ---------------------- GND
 
 Note: The BMP-180 is an I2C sensor and uses the Arduino Wire library. 
 Because the sensor is not 5V tolerant, we are using a 3.3 V 8 MHz Pro Mini or a 3.3 V Teensy 3.1.
 We have disabled the internal pull-ups used by the Wire library in the Wire.h/twi.c utility file.
 We are also using the 400 kHz fast I2C mode by setting the TWI_FREQ  to 400000L /twi.h utility file.
 */
#include "Wire.h" 
#include "SPI.h" 
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Using NOKIA 5110 monochrome 84 x 48 pixel display
// pin 9 - Serial clock out (SCLK)
// pin 8 - Serial data out (DIN)
// pin 7 - Data/Command select (D/C)
// pin 5 - LCD chip select (CS)
// pin 6 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(9, 8, 7, 5, 6);

#define BMP180_ADDRESS          0x77  // I2C address of BMP180
#define BMP180_WHO_AM_I         0xD0  // WHO_AM_I id of BMP180, should return 0x55
#define BMP180_RESET            0xE0
#define BMP180_CONTROL          0xF4
#define BMP180_OUT_MSB          0xF6
#define BMP180_OUT_LSB          0xF7
#define BMP180_OUT_XLSB         0xF8

#define HT16K33_ADDRESS         0x70
#define HT16K33_ON              0x21  // Commands
#define HT16K33_STANDBY         0x20
#define HT16K33_DISPLAYON       0x81
#define HT16K33_DISPLAYOFF      0x80
#define HT16K33_BLINKON         0x85 // Blink is off (00), 2 Hz (01), 1 Hz (10), or 0.5 Hz (11) for bits (21) 
#define HT16K33_BLINKOFF        0x81
#define HT16K33_DIM             0xE0 | 0x08  // Set brihtness from 1/16 (0x00) to 16/16 (0xFF)

// Arrangement for display 1 (4 digit bubble display)
// 
//               a = A0
//             _________
//            |         |
//   f = A2   |  g = A4 | b = A1
//            |_________|
//            |         |
//   e = A5   |         | c = A6
//            |_________|
//               d = A3        DP = A7


static const byte numberTable[] =
{
  0x6F, // 0 = 0
  0x42, // 1 = 1, etc
  0x3B, // 2
  0x5B, // 3
  0x56, // 4
  0x5D, // 5
  0x7D, // 6
  0x43, // 7
  0x7F, // 8
  0x57, // 9
  0x80, // decimal point
  0x00, // blank
  0x10, // minus sign
};

#define display1 1
#define display2 2
#define display3 3
#define display4 4

enum OSS {  // BMP-085 sampling rate
  OSS_0 = 0,  // 4.5 ms conversion time
  OSS_1,      // 7.5
  OSS_2,      // 13.5
  OSS_3       // 25.5
};

// Specify sensor parameters
uint8_t OSS = OSS_3;           // maximum pressure resolution

// These are constants used to calulate the temperature and pressure from the BMP-085 sensor
int16_t ac1, ac2, ac3, b1, b2, mb, mc, md, b5;  
uint16_t ac4, ac5, ac6;
float temperature, pressure, temppress, altitude;
uint32_t delt_t, count, tempcount;

// Pin definitions
int intPin = 12;  // These can be changed, 2 and 3 are the Arduinos ext int pins
int ledPin = 13;  // These can be changed, 2 and 3 are the Arduinos ext int pins

void setup()
{
  Wire.begin();
//  TWBR = 12;  // 400 kbit/sec I2C speed
  Serial.begin(38400);
 
  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(intPin, INPUT);
  digitalWrite(intPin, LOW);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  initHT16K33();          // initialize bubble display
  clearDsplay(display1);  // clear bubble display1
  clearDsplay(display2);  // clear bubble display2
  clearDsplay(display3);  // clear bubble display1
  clearDsplay(display4);  // clear bubble display2
  
  display.begin(); // Initialize the display
  display.setContrast(55); // Set the contrast
  display.setRotation(0); //  0 or 2) width = width, 1 or 3) width = height, swapped etc.
  
// Start device display with ID of sensor
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10,0); display.print("GY-80");
  display.setTextSize(1);
  display.setCursor(0, 20); display.print("10 DOF 10-bit");
  display.setCursor(0, 30); display.print("sensor fusion");
  display.setCursor(20,40); display.print("AHRS");
  display.display();
  delay(1000);

// Set up for data display
  display.setTextSize(1); // Set text size to normal, 2 is twice normal etc.
  display.setTextColor(BLACK); // Set pixel color; 1 on the monochrome screen
  
  // Read the WHO_AM_I register of the BMP-180, this is a good test of communication
  uint8_t c = readByte(BMP180_ADDRESS, BMP180_WHO_AM_I);  // Read WHO_AM_I register A for BMP-180
  display.clearDisplay();   // clears the screen and buffer
  display.setCursor(20,0);  display.print("BMP180");
  display.setCursor(0,10);  display.print("I READ");
  display.setCursor( 0,20); display.print(c, HEX);  
  display.setCursor(0,30);  display.print("I Should Be");
  display.setCursor(0,40);  display.print(0x55, HEX);
  display.display();
  delay(1000); 

  if (c = 0x55) // WHO_AM_I must be 0x55 (BMP-180 id) to proceed
  {  
    Serial.println("BMP180 is online...");  
 // Initialize devices for active mode read of pressure and temperature
    BMP180Calibration();
    Serial.println("BMP-180 calibration completed....");
  }
  else
  {
  display.clearDisplay();
  display.setCursor(0, 20); display.print("No Connection"); 
  display.setCursor(20, 40); display.print("to BMP-180!");
  display.display();
  while(1) ; // Loop forever if communication doesn't happen
  }
}

void loop()
{  
    // Average over the display duty cycle to get the best pressure and altitude resolution
    temperature = (float)BMP180GetTemperature()/10.;  // Get temperature from BMP-180 in degrees C
    temppress += (float)BMP180GetPressure();          // Get pressure from BMP-180 in Pa
    tempcount++;

    // Serial print and/or display at 0.5 s rate independent of data rates
    delt_t = millis() - count;
    if (delt_t > 500) { // update LCD once per half-second independent of read rate

    digitalWrite(ledPin, !digitalRead(ledPin));
   
    pressure = temppress/tempcount;  // use average pressure for reading to get ultra-high resolution
    temperature = temperature*9./5. + 32.;                          // convert to Fahrenheit
    altitude = 44330.*( 1. - pow((pressure/101325.0), (1./5.255))); // Calculate altitude in meters
   
    writeFloat(display1, temperature, 1);             // display temperature in degrees Fahrenheit to bubble display
    writeFloat(display2, pressure/1000, 2);           // display pressure in mPa to bubble display
    writeFloat(display3, altitude, 1);                // display altitude in meters to bubble display
    writeFloat(display4, altitude*3.281, 1);          // display altitude in feet to bubble display

    Serial.print("Temperature is "); Serial.print(temperature, 2); Serial.println(" C");
    Serial.print("Pressure is "); Serial.print(pressure, 2); Serial.println(" Pa");
    Serial.print("Altitude is "); Serial.print(altitude, 2); Serial.println(" m");
    Serial.print("Altitude is "); Serial.print(altitude*3.281, 2); Serial.println(" ft");
    Serial.println("");
 //   Serial.print(temperature); Serial.print(","); Serial.print(pressure); Serial.print(","); Serial.println(altitude);        

    count = millis();  
    temppress = 0;
    tempcount = 0;
    }
}

//===================================================================================================================
//====== Set of useful function to access pressure and temperature data
//===================================================================================================================
        
// Stores all of the BMP180's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
// These BMP-180 functions were adapted from Jim Lindblom of SparkFun Electronics
void BMP180Calibration()
{
  ac1 = readByte(BMP180_ADDRESS, 0xAA) << 8 | readByte(BMP180_ADDRESS, 0xAB);
  ac2 = readByte(BMP180_ADDRESS, 0xAC) << 8 | readByte(BMP180_ADDRESS, 0xAD);
  ac3 = readByte(BMP180_ADDRESS, 0xAE) << 8 | readByte(BMP180_ADDRESS, 0xAF);
  ac4 = readByte(BMP180_ADDRESS, 0xB0) << 8 | readByte(BMP180_ADDRESS, 0xB1);
  ac5 = readByte(BMP180_ADDRESS, 0xB2) << 8 | readByte(BMP180_ADDRESS, 0xB3);
  ac6 = readByte(BMP180_ADDRESS, 0xB4) << 8 | readByte(BMP180_ADDRESS, 0xB5);
  b1  = readByte(BMP180_ADDRESS, 0xB6) << 8 | readByte(BMP180_ADDRESS, 0xB7);
  b2  = readByte(BMP180_ADDRESS, 0xB8) << 8 | readByte(BMP180_ADDRESS, 0xB9);
  mb  = readByte(BMP180_ADDRESS, 0xBA) << 8 | readByte(BMP180_ADDRESS, 0xBB);
  mc  = readByte(BMP180_ADDRESS, 0xBC) << 8 | readByte(BMP180_ADDRESS, 0xBD);
  md  = readByte(BMP180_ADDRESS, 0xBE) << 8 | readByte(BMP180_ADDRESS, 0xBF);
}

  // Temperature returned will be in units of 0.1 deg C
  int16_t BMP180GetTemperature()
  {
  int16_t ut = 0;
  writeByte(BMP180_ADDRESS, BMP180_CONTROL, 0x2E); // start temperature measurement
  delay(5);
  uint8_t rawData[2] = {0, 0};
  readBytes(BMP180_ADDRESS, 0xF6, 2, &rawData[0]); // read raw temperature measurement
  ut = (int16_t)(((int16_t) rawData[0] << 8) | rawData[1]);
 
 long x1, x2;
  
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return  ((b5 + 8)>>4);  
}

// Calculate pressure read calibration values  
// b5 is also required so BMP180GetTemperature() must be called first.
// Value returned will be pressure in units of Pa.
long BMP180GetPressure()
{
  long up = 0;
  writeByte(BMP180_ADDRESS, BMP180_CONTROL, 0x34 | OSS << 6); // Configure pressure measurement for highest resolution
  delay(5 + 7*OSS); // delay 5 ms at lowest resolution, 26 ms at highest
  uint8_t rawData[3] = {0, 0, 0};
  readBytes(BMP180_ADDRESS, BMP180_OUT_MSB, 3, &rawData[0]); // read raw pressure measurement of 19 bits
  up = (((long) rawData[0] << 16) | ((long)rawData[1] << 8) | rawData[2]) >> (8 - OSS);

  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;
  
  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;
  
  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
  
  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
    
  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  
  return p;
}      

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++ Useful Display Functions++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 // Wire.h read and write protocols
 void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
	Wire.beginTransmission(address);  // Initialize the Tx buffer
	Wire.write(subAddress);           // Put slave register address in Tx buffer
	Wire.write(data);                 // Put data in Tx buffer
	Wire.endTransmission();           // Send the Tx buffer
}

  uint8_t readByte(uint8_t address, uint8_t subAddress)
{
	uint8_t data; // `data` will store the register data	 
	Wire.beginTransmission(address);         // Initialize the Tx buffer
	Wire.write(subAddress);	                 // Put slave register address in Tx buffer
	Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
	Wire.requestFrom(address, (uint8_t) 1);  // Read one byte from slave register address 
	data = Wire.read();                      // Fill Rx buffer with result
	return data;                             // Return data read from slave register
}

 void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{  
	Wire.beginTransmission(address);   // Initialize the Tx buffer
	Wire.write(subAddress);            // Put slave register address in Tx buffer
	Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
	uint8_t i = 0;
        Wire.requestFrom(address, count);  // Read bytes from slave register address 
	while (Wire.available()) {
        dest[i++] = Wire.read(); }         // Put read results in the Rx buffer
}
