/*
    Adam Dykhouse
    5/23/2017
    Pennsylvania State University
    Dept. of Physics

    HOLLOW CATHODE LAMP (HCL) CONTROL
      This sketch allows the Arduino Nano to recieve and interpret a fixed length command (8 ASCII bytes) over USB.
      Once interpreted, the commands are used to control the operation of 5 seperate HCL power supplies, each their own channel.
      The programmable control voltage set by the user, is transferred from the Arduino to a DAC via SPI. Furthermore, when
      housekeeping mode is enabled, the Arduino will use interrupts to periodically send measurements for Vdd, board temperature,
      and program voltage over USB to the host computer for monitoring purposes.
    
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

    Arduino Nano Pins used:
        Enable:
          * D2 = HKN
          * D3 = EN0
          * D4 = EN1
          * D5 = EN2
          * D6 = EN3
          * D7 = EN4
          * D8 = VDD_EN

        SPI:
          * D10 = SSN
          * D11 = MOSI
          * D13 = SCK

        Analog Voltage Signals:
          * A7 = VCTRL0
          * A6 = VCTRL1
          * A5 = VCTRL2
          * A4 = VCTRL3
          * A3 = VCTRL4
          * A2 = TEMP
          * A1 = VDD_MON
*/


#include <SPI.h>                      // include the SPI library:

// Digital pin initializations
const int HKN = 2;                    // House keeping enable
const int EN0 = 3;                    // Channel 0 enable
const int EN1 = 4;                    // Channel 1 enable
const int EN2 = 5;                    // Channel 2 enable
const int EN3 = 6;                    // Channel 3 enable
const int EN4 = 7;                    // Channel 4 enable
const int VDD_EN = 8;                 // 24V enable
const int SSN = 10;                   // active low slave select signal
const int MOSI = 11;                  // Master out slave in data line
const int SCK = 13;                   // System clock signal

// Analog pint initializations
const int VCTRL0 = A7;                // Channel 0 control voltage
const int VCTRL1 = A6;                // Channel 1 control voltage
const int VCTRL2 = A5;                // Channel 2 control voltage
const int VCTRL3 = A4;                // Channel 3 control voltage
const int VCTRL4 = A3;                // Channel 4 control voltage
const int TEMP = A2;                  // Temperature monitoring pin for housekeeping
const int VDD_MON = A1;               // 24V monitoring for power supply

char buff;                            // Current character from user input
char act = 0;                         // Character corresponding to one of the valid command actions 'p','e', or 'd'
char adr = 0;                         // Character corresponding to one of the valid channel numbers '0','1','2','3', or '4'
char data3 = 0;                       // Character corresponding to the most significant hex digit of the data value
char data2 = 0;                       // Character corresponding to the next most significant hex digit of the data value
char data1 = 0;                       // Character corresponding to the next most significant hex digit of the data value
char data0 = 0;                       // Character corresponding to the least significant hex digit of the data value
int fsm = 0;                          // State variable


void setup() 
{
  SPI.begin();                        // Initialize SPI - sets SCK, MOSI, and SSN to outputs. Pulls SCK and MOSI low, and SS high.
  Serial.begin(9600);                 // Initialize serial port with 9600 Baud Rate
}

void loop() 
{
  if(Serial.available() > 0)         // If the buffer contains a byte,   
    buff = Serial.read();            //   save that byte to buff
  
  switch(fsm)
  {
    case 0:    // Check first byte for p, e, or d
      if(buff == 'p' || buff == 'e' || buff == 'd')
      {
        act = buff;                 // If so, save that command character to act
        fsm = 1;                    // Check next byte
      }
      else                          
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 1:    // Check second byte for space character                        
      if(buff == ' ')
        fsm = 2;                    // If so, proceed to check third byte for channel number
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 2:    // Check third byte for channel number
      if(buff == '0' || buff == '1' || buff == '2' || buff == '3' || buff == '4')
      {
        adr = buff;                 // If so, save that channel number to adr
        fsm = 3;                    // Then check next byte
      }
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 3:    // Check fourth byte for space character                         
      if(buff == ' ')
        fsm = 4;                    // If so, proceed to check third byte for channel number
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 4:    // Check fifth byte for valid character between '0'-'f'
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data3 = buff;               // If so, save that character to data3
        fsm = 5;                    // check next byte
      }
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 5:    // Check sixth byte for valid character between '0'-'f'
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data2 = buff;               // If so, save that character to data2
        fsm = 6;                    // check next byte
      }
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 6:    // Check seventh byte for valid character between '0'-'f'
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data1 = buff;               // If so, save that character to data1
        fsm = 7;                    // check next byte
      }
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 7:    // Check eighth byte for valid character between '0'-'f'
      if((buff > '/' && buff < ':')||(buff > '`' && buff < 'g'))
      {
        data0 = buff;               // If so, save that character to data0
        fsm = 8;                    // check next byte
      }
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 8:    // Check ninth byte for either NL or CR
      if(buff == '\n' || buff == '\r')
        fsm = 9;                    // Valid command has been found, proceed to execute
      else
        fsm = 0;                    // Otherwise, invalid command, re-check first byte for valid character
      break;

    case 9:    // Syntactically correct command, proceed to interpret and execute
      if(act == 'p' && adr == '0' && data3 == '0' && data2 == '0' && data1 == '0')
      {
        if(data0 == '0')
        {
          digitalWrite(VDD_EN,LOW);
          fsm = 0;
        }
        else if(data0 == '1')
        {
          digitalWrite(VDD_EN,HIGH);
          fsm = 0;
        }
      }
      else if(act == 'e')
      {
        
      }
      
    default: 
      fsm = 0;
    break;
  }
}




// PRE: channel = 0x30-0x34, value = 0x0000-0xffff, SSN = HIGH
// Protocol for sending data from Arduino to DAC over SPI, Used only for 'd' commands
void WriteDAC(int channel, int valueMSB, int valueLSB)     
{   
  digitalWrite(SSN, LOW);           // 1. Drive SSN low to signal DAC transmission start
  SPI.transfer(channel);            // 2. Send in the channel number via SPI
  SPI.transfer(valueMSB);           // 3. Send in the most significant byte of value via SPI
  SPI.transfer(valueLSB);           // 4. Send in the least significant byte of value via SPI
  digitalWrite(SSN, HIGH);          // 5. Drive SSN high to signal DAC transmission stop
}
