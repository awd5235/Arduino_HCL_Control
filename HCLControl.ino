 /*
    Adam Dykhouse
    5/23/2017
    Pennsylvania State University
    Dept. of Physics

    HOLLOW CATHODE LAMP (HCL) CONTROL
      This sketch allows the Arduino Nano to recieve and interpret a fixed length command (8 ASCII bytes) over USB.
      Once interpreted, the commands are used to control the operation of 5 seperate HCL power supplies, each their own channel.
      The programmable control voltage set by the user, is transferred from the Arduino to a DAC via SPI.
    
    Each HCL power supply has 5 lines:
        * Vdd:       +24V comes directly from power supply turn on/off with Arduino pin 11 (Serves as master on/off for ALL HCL supplies).
        * Vdd_rtn:   Reference voltage for Vdd.
        * EN0:       +24V enable comes directly from Arduino pins 6-10 (D3-D7) used to enable Vdd for any SINGLE HCL supply independently of the others.
        * VCTRL0:    Desired control voltage input by user (over USB), and sent over SPI (MOSI) to the DAC, which is then amplified and sent to the HCL power supply.
        * VCTRL_rtn: Reference voltage for VCTRL

    Command Protocol:
        * "p 0 0000" = Disconnect 24V supply (All channels)
        * "p 0 0001" = Connect 24V supply (All channels)
        * "e n 0000" = Disable 24V supply for single channel n
        * "e n 0001" = Enable 24V supply for single channel n
        * "d n xxxx" = set DAC channel n to 16-bit value xxxx over SPI

    Housekeeping Protocol:
        * "v 0 xxxx" = Displays power supply voltage xxxx, usually around 0x01BF for intended +24V operation
        * "t 0 xxxx" = Displays temperature reading xxxx
        * "a n xxxx" = Displays analog control voltage xxxx for channel n
*/


#include <SPI.h>                      // include the SPI library:

// Digital pin initializations
const int EN0 = 3;                    // Channel 0 enable
const int EN1 = 4;                    // Channel 1 enable
const int EN2 = 5;                    // Channel 2 enable
const int EN3 = 6;                    // Channel 3 enable
const int EN4 = 7;                    // Channel 4 enable
const int VDD_EN = 8;                 // 24V enable

// SPI pin
const int SSN = 10;                   // active low slave select signal for SPI

// Analog pint initializations
const int VCTRL0 = A7;                // Channel 0 control voltage
const int VCTRL1 = A6;                // Channel 1 control voltage
const int VCTRL2 = A5;                // Channel 2 control voltage
const int VCTRL3 = A4;                // Channel 3 control voltage
const int VCTRL4 = A3;                // Channel 4 control voltage
const int TEMP = A2;                  // Temperature monitoring pin for housekeeping
const int VDD_MON = A1;               // 24V monitoring for power supply

// Local Variables
char buff;                            // User input character buffer
char act = 0;                         // Character corresponding to one of the valid command actions 'p','e', or 'd'
char adr = 0;                         // Character corresponding to one of the valid channel numbers '0','1','2','3', or '4'
char data3 = 0;                       // Character corresponding to the most significant hex digit of the data value
char data2 = 0;                       // Character corresponding to the next most significant hex digit of the data value
char data1 = 0;                       // Character corresponding to the next most significant hex digit of the data value
char data0 = 0;                       // Character corresponding to the least significant hex digit of the data value
int fsm = 0;                          // State variable
word dacVal = 65535;                  // 16-bit value to set dac, initialized to 0xffff
int dacChan = 0;                      // 4-bit dac command + 4-bit channel number



void setup() 
{
  SPI.begin();                        // Initialize SPI - sets SCK, MOSI, and SSN to outputs. Pulls SCK and MOSI low, and SS high.
  Serial.begin(9600);                 // Initialize serial port with 9600 Baud Rate

  pinMode(EN0,OUTPUT);                // Initialize enable for channel 0
  digitalWrite(EN0,LOW);              // Set low
  
  pinMode(EN1,OUTPUT);                // Initialize enable for channel 1
  digitalWrite(EN1,LOW);              // Set low
  
  pinMode(EN2,OUTPUT);                // Initialize enable for channel 2
  digitalWrite(EN2,LOW);              // Set low
  
  pinMode(EN3,OUTPUT);                // Initialize enable for channel 3
  digitalWrite(EN3,LOW);              // Set low
  
  pinMode(EN4,OUTPUT);                // Initialize enable for channel 4  
  digitalWrite(EN4,LOW);              // Set low
  
  pinMode(VDD_EN,OUTPUT);             // Initialize 24V power supply enable
  digitalWrite(VDD_EN,LOW);           // Set low
  
  pinMode(SSN,OUTPUT);                // Initialize slave select line
  digitalWrite(SSN,HIGH);             // Set high
}

void loop() 
{
  if(fsm != 9)
  {
    while(Serial.available() == 0);    // Do nothing while buffer is NULL, 
    buff = Serial.read();              // Save input character to buff
  }
  
  switch(fsm)
  {
    case 0:    // Check first byte for p, e, or d
    {
      if(buff == 'p' || buff == 'e' || buff == 'd' || buff == 'v' || buff == 't' || buff == 'a')
      {
        act = buff;                   // If so, save that command character to act
        fsm = 1;                      // Check next byte
        // Serial.print(buff);
      }
      else
      {                          
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 1:    // Check second byte for space character 
    {                       
      if(buff == ' ')
      {
        fsm = 2;                      // If so, proceed to check third byte for channel number
        // Serial.print(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 2:    // Check third byte for channel number
    {
      if(buff == '0' || buff == '1' || buff == '2' || buff == '3' || buff == '4')
      {
        adr = buff;                   // If so, save that channel number to adr
        fsm = 3;                      // Then check next byte
        // Serial.print(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 3:    // Check fourth byte for space character
    {                         
      if(buff == ' ')
      {
        fsm = 4;                      // If so, proceed to check third byte for channel number
        // Serial.print(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 4:    // Check fifth byte for valid character between '0'-'f'
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data3 = buff;                 // If so, save that character to data3
        fsm = 5;                      // check next byte
        // Serial.print(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    break;


    case 5:    // Check sixth byte for valid character between '0'-'f'
    {
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data2 = buff;                 // If so, save that character to data2
        fsm = 6;                      // check next byte
        // Serial.print(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 6:    // Check seventh byte for valid character between '0'-'f'
    {
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data1 = buff;                 // If so, save that character to data1
        fsm = 7;                      // check next byte
        // Serial.print(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 7:    // Check eighth byte for valid character between '0'-'f'
    {
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data0 = buff;                 // If so, save that character to data0
        fsm = 8;                      // check next byte
        // Serial.println(buff);
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 8:    // Check ninth byte for either NL or CR
    {
      if(buff == '\n' || buff == '\r')
      {
        fsm = 9;                      // Valid command has been found, proceed to execute
      }
      else
      {
        fsm = 0;                      // Otherwise, invalid command, re-check first byte for valid character
        // Serial.println("Invalid Command, please try again");
      }
    }
    break;


    case 9:    // Syntactically correct command, proceed to interpret and execute.
    {
      if(act == 'p' && adr == '0' && data3 == '0' && data2 == '0' && data1 == '0') // 'p' command execution
      {
        if(data0 == '0')
        {
          digitalWrite(VDD_EN,LOW);             // Disconnect 24V supply to ALL channels
          // Serial.println("Disconnect 24V supply to ALL channels");
        }
        
        else if(data0 == '1')
        {
          digitalWrite(VDD_EN,HIGH);            // Connect 24V supply to ALL channels
          // Serial.println("Connect 24V supply to ALL channels");
        }

        // else
        //  Serial.println("Invalid Command!");   // Occurs if data0 is not 0 or 1
      }
      else if(act == 'e' && data3 == '0' && data2 == '0' && data1 == '0')          // 'e' command execution
      {
        if(adr == '0' && data0 == '0')
        {
          digitalWrite(EN0,LOW);                // Disable channel 0 supply
          // Serial.println("Disable channel 0 supply");
        }
        
        else if(adr == '0' && data0 == '1')
        {
          digitalWrite(EN0,HIGH);               // Enable channel 0 supply
          // Serial.println("Enable channel 0 supply");
        }
        
        else if(adr == '1' && data0 == '0')
        {
          digitalWrite(EN1,LOW);                // Disable channel 1 supply
          // Serial.println("Disable channel 1 supply");
        }
        
        else if(adr == '1' && data0 == '1')
        {
          digitalWrite(EN1,HIGH);               // Enable channel 1 supply
          // Serial.println("Enable channel 1 supply");
        }
        
        else if(adr == '2' && data0 == '0')
        {
          digitalWrite(EN2,LOW);                // Disable channel 2 supply
          // Serial.println("Disable channel 2 supply");
        }
        
        else if(adr == '2' && data0 == '1')
        {
          digitalWrite(EN2,HIGH);               // Enable channel 2 supply
          // Serial.println("Enable channel 2 supply");
        }
        
        else if(adr == '3' && data0 == '0')
        {
          digitalWrite(EN3,LOW);                // Disable channel 3 supply
          // Serial.println("Disable channel 3 supply");
        }
        
        else if(adr == '3' && data0 == '1')
        {
          digitalWrite(EN3,HIGH);               // Enable channel 3 supply
          // Serial.println("Enable channel 3 supply");
        }
          
        else if(adr == '4' && data0 == '0')
        {
          digitalWrite(EN4,LOW);                // Disable channel 4 supply
          // Serial.println("Disable channel 4 supply");
        }
          
        else if(adr == '4' && data0 == '1')
        {
          digitalWrite(EN4,HIGH);               // Enable channel 4 supply
          // Serial.println("Enable channel 4 supply");
        }

        // else
        //  Serial.println("Invalid command!");   // Occurs if adr is not '0'-'4' or if data0 is not '0' or '1'
      }
      else if(act == 'd')                                   // 'd' command execution
      {
        dacChan = char2num('0','0','3',adr);                // Determine upper 8-bit to send DAC (4-bit command + 4-bit channel)
        dacVal = char2num(data3, data2, data1, data0);      // Convert 4 data chars to single 16-bit number
        WriteDAC(dacChan,dacVal);                           // Write value to DAC at specified address
        // Serial.print(dacVal);
        // Serial.print(" written to DAC channel ");
        // Serial.println(dacChan);
      }
      else if(act == 'v' && adr == '0' && data3 == '0' && data2 == '0' && data1 == '0' && data0 == '0')   // Read Supply monitor voltage
      {
        Serial.print("Vdd = ");
        Serial.println(analogRead(VDD_MON));
      }
      else if(act == 't' && adr == '0' && data3 == '0' && data2 == '0' && data1 == '0' && data0 == '0')   // Read Board Temperature
      {
        Serial.print("Board Temp = ");
        Serial.println(analogRead(TEMP));
      }
      else if(act == 'a' && data3 == '0' && data2 == '0' && data1 == '0' && data0 == '0')   // Read Control Voltage
      {
        if(adr == '0')      // Channel 0 control voltage
        {
          Serial.print("Control Voltage 0 = ");
          Serial.println(analogRead(VCTRL0));
        } 
         
        else if(adr == '1') // Channel 1 control voltage
        {
          Serial.print("Control Voltage 1 = ");
          Serial.println(analogRead(VCTRL1));
        } 
         
        else if(adr == '2') // Channel 2 control voltage
        {
          Serial.print("Control Voltage 2 = ");
          Serial.println(analogRead(VCTRL2));
        }
          
        else if(adr == '3') // Channel 3 control voltage
        {
          Serial.print("Control Voltage 3 = ");
          Serial.println(analogRead(VCTRL3));
        }
          
        else if(adr == '4') // Channel 4 control voltage
        {
          Serial.print("Control Voltage 4 = ");
          Serial.println(analogRead(VCTRL4));
        }

        // else                // Occurs of address is not '0'-'4'
        //   Serial.println("Invalid command!"); 
      }
      // else
      //   Serial.println("Invalid command!");     // If all else fails, throw error
      fsm = 0;              // Restart command parsing
    }
    break;

      
    default: 
      fsm = 0;              // Restart command parsing
    break;
  }
}


// Given valid characters data3-0 return a single 16-bit number corresponding to their value
word char2num(char data3, char data2, char data1, char data0)
{
  word num = 65535;            // Initialize to 0xffff

  if(data0 > '9')              // Check LSB is a letter 'a'-'f'
    num = data0 - 87;          // If so, subtract 87 to convert to number 10-15
  else                         // Otherwise, it is a digit '0'-'9'
    num = data0 - 48;          // Subtract 48 to convert to number 0-9

  if(data1 > '9')              // Check next most LSB is a letter 'a'-'f'
    num += (data1 - 87)*16;    // If so, subtract 87 to convert to number 10-15
  else                         // Otherwise, it is a digit '0'-'9'
    num += (data1 - 48)*16;    // Subtract 48 to convert to number 0-9

  if(data2 > '9')              // Check next most LSB is a letter 'a'-'f'
    num += (data2 - 87)*256;   // If so, subtract 87 to convert to number 10-15
  else                         // Otherwise, it is a digit '0'-'9'
    num += (data2 - 48)*256;   // Subtract 48 to convert to number 0-9

  if(data3 > '9')              // Check MSB is a letter 'a'-'f'
    num += (data3 - 87)*4096;  // If so, subtract 87 to convert to number 10-15
  else                         // Otherwise, it is a digit '0'-'9'
    num += (data3 - 48)*4096;  // Subtract 48 to convert to number 0-9
  
  return num;
}

// Protocol for sending data from Arduino to DAC over SPI, Used only for 'd' commands
void WriteDAC(int dacChan, word dacVal)     
{
  SPI.beginTransaction(SPISettings (10000,MSBFIRST,SPI_MODE0)); // 1. Specify transaction settings: 10kHz clock, send MSB first, active low clock sampled on rising edge
  digitalWrite(SSN, LOW);                                       // 2. Drive SSN low to signal DAC transmission start
  SPI.transfer(dacChan);                                        // 3. Send in the command + channel number via SPI
  SPI.transfer16(dacVal);                                       // 4. Send 16-bit value to set DAC
  digitalWrite(SSN, HIGH);                                      // 5. Drive SSN high to signal DAC transmission stop
  SPI.endTransaction();                                         // 6. End transaction
}
