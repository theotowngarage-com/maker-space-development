/* Config
    Defines various settings for DigitalMassBalance.
*/


#pragma once


//-----HX711----//
  // Number of averages while measuring. *Set to 1 because the library's averaging
  // is too slow and interferes with other interactions.
  const int HX_NUM_AVGS = 1;
  
  // Averaging window (applies only to calibrated value, not raw).
  const int QUEUE_SIZE = 10;
  double hxQueue[QUEUE_SIZE];


//----LCD----//
  // Is there an LCD?
  bool isLCD = true;

  // Display size.
  const int LCD_ROWS = 2;
  const int LCD_COLS = 16;


//----Calibration/Sensitivity----//
  // Button hold length to enter/exit calibration mode (ms).
  const int CAL_WAIT = 3000;
  
  // Signature to store in the memory when calibrating.
  const char CAL_SIGNATURE = 'C';

  // Address of calibration signature.
  const int CAL_SIGNATURE_ADDR = 0;

  // Address of the stored calibration value.
  const int CAL_VALUE_ADDR = CAL_SIGNATURE_ADDR + sizeof(char);

  // Mass used to calibrate (default: 1 US cup of water);
  double cal_standard_mass = 235.9;

  // Mass units.
  String units = "  g";
  
  const double CAL_THRESHOLD = 20000;
  
 
//----Serial----//
  // Communication rate.
  const int BAUD = 9600;
  const int REPORT_RATE = 1; // Hz
  unsigned long lastRefresh = 1;  // milliseconds
  
  // Non-printable ASCII characters.
  const int LF = 0x0A;
  const int CR = 0x0D;
  const int ESC= 0x1B;
  const int SPACE = 0x20;
  // Continuous output to serial.
  bool isContinuousReport = 0;
  
  // Response characteristics.
  // Response block width for a weight report.
  const int WT_WIDTH = 10;
  // Number of digits after the decimal.
  int precision = 3;
  // About index.
  int aboutIdx = 4;
  // Response characters.
  int range = 1;  // This scale is single-range.
  