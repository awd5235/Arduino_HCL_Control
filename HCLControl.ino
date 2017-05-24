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

const int SSN = 10;                   // Digital pin 10 is active low slave select signal

void setup() {
  SPI.begin();                        // Initialize SPI - sets SCK, MOSI, and SSN to outputs. Pulls SCK and MOSI low, and SS high.
}


void loop() {
  // put your main code here, to run repeatedly:

}

// PRE: channel = 0x30-0x34, value = 0x0000-0xffff
void WriteDAC(int channel, int value) {   // Protocol for sending data from Arduino to DAC over SPI
  digitalWrite(SSN, LOW);                 // 1. Drive SSN low to signal DAC transmission start
  SPI.transfer(channel);                  // 2. Send in the channel number via SPI
  SPI.transfer(value);                    // 3. Send in the value via SPI
  digitalWrite(SSN, HIGH);                // 4. Drive SSN high to signal DAC transmission stop
}
