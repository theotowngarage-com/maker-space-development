/* Pinouts
    Defines the pins used on the Arduino microcontroller for DigitalMassBalance.
*/


#pragma once


//----HX711----//
  // Data pin.
  const int HX_DT  = 2;

  // Clock pin.
  const int HX_SCK = 3;

  // Vcc pin - HX711 requires 3-5V @ 1.5 mA, Nano supplies 5V @ 20 mA.
  const int HX_VCC = 4;


//----LCD----//
  // Reset pin.
  const int LCD_RS = A0;

  // Enable pin.
  const int LCD_EN = A1;

  // Data pins.
  const int LCD_D4 = A2;
  const int LCD_D5 = A3;
  const int LCD_D6 = A4;
  const int LCD_D7 = A5;

  // Power supply. allaboutcircuits.com suggests an LCD requires 5V @ 3mA.
  const int LCD_VCC = 5;


//----Input pins----//
  // Calibrate.
  const int BTN_CAL = 7;
  
  // Tare.
  const int BTN_TARE = 8;