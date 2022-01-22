/*  DigitalMassBalance uses a load cell to measure and report an object's mass.
      This firmware is designed to meet SMA SCP 0499 Level #2 for scale serial 
      communication. The command and response formats for serial communication 
      are documented in included files.
      
      The scale was designed by researchers ing Michigan Technological 
      University's MOST group <https://www.appropedia.org/Category:MOST>
      
      REVISIONS:
      1.0.0 : Initial release - function scale with serial reporting.
      2.0.0 : First release up to SMA standards. Work to do on data filtering.
      
    Copyright (C) 2020 Benjamin Hubbard
      ! Note that external libraries included with this software are subject to
        their own licenses, included within their respective folders.
        
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


// Headers including a variety of definitions for the scale. These are separated
// to help reduce the length of this script.
#include "src\Libraries.hpp"
#include "src\Pinouts.hpp"
#include "src\Config.hpp" 


const String REV = "2.0.1";


// Hardware objects (uses external libraries).
// Load cell amplifier.
HX711 loadcell;
// LCD Display. Pins defined in Pinouts.hpp.
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


// Variables used during data collection.
// Sensitivity is read from memory - this is here as a default.
double sensitivity = 1.0;
// Used as an offset from zero (ie for a container). Implemented in this script,
// while zero is implemented within the HX711 library.
double tareWeight = 0.0;
// Measured mass (can be assigned an averaged/filtered value or an instantaneous
// value.
double mass;
  

void setup() {
  // Initialization methods have self-explanatory names. Initializers as of REV
  // 1.0.0 report non-standard information over serial. Those at startup are the
  // only non-standard serial communications this scale produces.
  initSerial();
  initLoadCell();
  initLCD();
  getSensitivity();
  initQueue();
  pinMode(BTN_TARE, INPUT_PULLUP);
  Serial.print("\nUse <LF>X?<CR> to view serial commands\r");
}


void loop() {
  // Get the averaged, tared mass.
  mass = getMassAveraged();
  
  // Enforce report rate without hampering sample rate (sample rate depends on
  // how much processing is done during each iteration of loop().
  // TODO: add a command to change the report rate.
  if (millis() - lastRefresh > 1/REPORT_RATE * 1000) {
    // Reset the time. Last refresh is initialized in Config.hpp.
    lastRefresh = millis();
    
    // Listen for input over serial.
    // When using Arduino Serial Monitor, switch to 'Both NL & CR' in bottom 
    // right. When the scale first starts up, hit enter once to queue up a <LF>
    // character, otherwise the first command will not meet com standards and 
    // return a ?
    // When using Putty, use Ctrl+J for LF, followed by command, followed by 
    // Ctrl+M or simply Enter for CR.
    if (Serial.available() > 2) { // Minimum cmd length is 3 characters<LF>c<CR>
      doSerial();
    }
    
    if (isContinuousReport) {
      reportMassAveraged();
    }
    
    // Simple scale functionality. Note that placement of button listener 
    // requires an extended button press. Assuming a report rate of at least
    // 1 Hz, this should not be an issue.
    displayMass(mass);
    listenForButtonInput();
  }
}


//----Support methods----//
  //----Initialize----//
    void initSerial() {
      // Initialize the serial connection. BAUD is defined in Config.hpp to 
      // match the SMA standard.
      Serial.begin(BAUD);
      
      while(!Serial) {
        // Wait for serial to initialize.
      } // Serial initialized.
    }
    
    
    void initLoadCell() {
      // Set up the HX711 for use. Turns on the device, then verifies the 
      // calibration value.
      Serial.print("\nInitializing HX711...");
      
      // Turn on the HX711 power supply.
      pinMode(HX_VCC, OUTPUT);
      digitalWrite(HX_VCC, HIGH);
      
      // Give it time to power on.
      delay(500);
      
      // Initialize the HX711.
      loadcell.begin(HX_DT, HX_SCK);
      
      // Wait until it's ready.
      bool is_ready = false;
      int num_retries = 3;
      int wait_delay = 200;
      while (!is_ready) {
        // Give some indication that it's thinking.
        Serial.print("...");
        is_ready = loadcell.wait_ready_retry(num_retries, wait_delay);
      }
      
      // Give the HX711 a chance to finish initializing.
      delay(2000);
      
      // Zero the scale (set the offset on data returned by the HX711).
      zeroSilent();
      
      Serial.print("HX711 Initialized!\r\n\r");
    }
    
    
    void initLCD() {
      // Runs all the necessary startup for the LCD.
      if (!isLCD) {
        return;
      }
      
      Serial.print("\nInitializing LCD...");
      
      // Turn on the LCD power supply.
      pinMode(LCD_VCC, OUTPUT);
      digitalWrite(LCD_VCC, HIGH);
      
      // Give it time to power on.
      delay(500);
      
      // Initialize the display.
      lcd.begin(LCD_COLS, LCD_ROWS);
      // Clear the display.
      lcd.clear();
      // Home the cursor.
      lcd.home();
      // Hide the cursor.
      lcd.noCursor();
      // Ensure the display is on. (lcd.noDisplay() turns off the display).
      lcd.display();
      
      // Test the display.
      Serial.print("Testing the display...");
      // Print an 8 to each character in the display.
      for (int i = 0; i < LCD_ROWS * LCD_COLS; i++) {
        lcd.print("8");
        if (i == LCD_COLS - 1) {
          lcd.setCursor(0, 1);
        }
      }
      delay(200);
      lcd.clear();
      
      Serial.print("LCD initialized!\r\n\r");
    }
    
    // TODO: replace averaging queue with a low pass filter.
    void initQueue() {
      for (int i = 0; i < QUEUE_SIZE; i++) {
        hxQueue[i] = 0.00;
      }
    }
    
    
  //----Serial----//
    void doSerial() {
      // Give everything a chance to transmit over serial.
      delay(200);
      // Make sure the buffer is settled.
      Serial.flush();

      // Determine how many characters are waiting.
      int len = Serial.available();
      
      // Initialize an array to hold the command.
      int cmd[len];

      // Fill up cmd.
      receiveCommand(cmd, len);
      
      // Check for abort command.
      int escIdx = findInArray(cmd, ESC, 0, len);
      if (escIdx >= 0) {  // If there's an escape character, reset.
        softReset();
      }
      
      // Parse the command for the <LF> and <CR>. The command starts one char
      // beyond the LF, and ends with the CR.
      int startIdx = findInArray(cmd, LF, 0, len) + 1;
      int endIdx = findInArray(cmd, CR, startIdx, len);
      
      // Check for errors. Since we start after the LF, minimum index is 1. 
      // Note that findInArray returns -1 if it cannot find the character.
      if (startIdx < 1 || endIdx < 1) {
        Serial.println("\n?\r");
        return;
      }
      
      // Execute the command.
      doCommand(cmd, startIdx, endIdx);
    }

    // Note that arrays are pointers, so just pass in the array's variable.
    // TODO: make this accommodate commands that come character by character.
    //        This can be accomplished by clearing command in when receiving
    //        a LF, but requires preallocating potentially too much memory for a 
    //        command. Can be accomplished by peeking for a <CR>. Could have
    //        timing issues. Maybe watch the change in Serial.available().
    void receiveCommand(int *cmd, int len) {  
      // Read everything but the last character into the cmd array.
      // The last character could be an LF which would lead the next command.
      for (int i = 0; i < len-1; i++) {
        cmd[i] = Serial.read();
      }

      // If the next guy is <LF>, leave 'er alone (this accommodates Arduino
      // Serial Monitor behavior).
      // If not, add it to the cmd array (could be a CR from PuTTY, or a 
      // single character from terminal without local echo/line editing on.
      char last = Serial.peek();
      if (last != LF){
        // Chuck it at the end of the array.
        cmd[len-1] = Serial.read();
      }
    }
   
    
    int findInArray(int *array, int query, int startSearch, int endSearch) {
      // Finds a character in an integer array. Used to find <CR> and <LF>.
      // Account for incrementing i at top of loop.
      int i = startSearch - 1;
      int c;  // Character being checked.
      do {
        // Increment i.
        i++;
        // Read a character from the array.
        c = array[i];
        
        // Don't go looking where there is nothing to be found.
        if (i > endSearch) {
          return -1;
        }
      } while (c != query);
      
      return i;
    }
    
    
    void doCommand(int *cmd, int startIdx, int endIdx) {
      // By the SMA standard, the first character indicates the function. 
      // The scale should not accept a command if it doesn't match the syntax 
      // exactly. (eg <LF> wa <CR> should not execute the <LF> w <CR> command.)
      // To combat this, interpret commands by length.
      // First, cancel continuous reporting.
      isContinuousReport = 0;
      
      switch (endIdx - startIdx) {
        case 1:   // Single character commands.
          switch ((char) cmd[startIdx]) {
            case 'w': // Report weight.
            case 'W':
              reportMassAveraged();
              break;
              
            case 'z': // Zero request.
            case 'Z':
              zero();
              break;
              
            case 'd': // Run diagnostics.
            case 'D':
              // TODO: Implement actual diagnostics.
              Serial.print("\n    \r");
              break;
              
            case 'a': // About (first row).
            case 'A':
              aboutIdx = 0;
              Serial.print("\nSMA:2/1.0\r");
              break;
              
            case 'b': // aBout (scrolling).
            case 'B':
              switch (aboutIdx) {
                case 0:
                  Serial.print("\nMFG:Michigan Tech MOST\r");
                  break;
                case 1:
                  Serial.print("\nMOD:Digital Mass Balance\r");
                  break;
                case 2:
                  Serial.print("\nREV:" + REV + "\r");
                  break;
                case 3:
                  Serial.print("\nEND:\r");
                  break;
                default:
                  Serial.print("\n?\r");
                  break;
              }
              aboutIdx++;
              break;
              
            case 'r': // Continuous reporting request.
            case 'R':
              isContinuousReport = 1;
              break;
              
            case 't': // Tare request.
            case 'T':
              tare();
              break;
              
            case 'c': // Clear tare.
            case 'C':
              clearTare();
              break;
              
            case 'm': // Return the current tare weight.
            case 'M':
              reportTare();
              break;
              
            default:  // Unknown command.
              Serial.print("\n?\r");
              break;
          } // End of single character commands.
          break;
          
        case 2: // 2 character commands.
          // Command will be an X followed by a character.
          switch ((char) cmd[startIdx]) {
            case 'x': // Custom commands.
            case 'X':
              switch ((char) cmd[startIdx + 1]) {
                case 'c': // Calibrate request (using hard-coded standard mass).
                case 'C':
                  // Calibration occurs with no tare.
                  clearTareSilent();
                  calResponse();
                  calibrate();
                  break;
                
                case 'l': // Toggle LCD power.
                case 'L':
                  if (isLCD) {
                    isLCD = false;
                    lcd.noDisplay();
                    digitalWrite(LCD_VCC, LOW);
                  } else if (!isLCD) {
                    isLCD = true;
                    initLCD();
                  }
                  break;
                  
                case 'p': // Toggle output precision.
                case 'P':
                  precision += 1;
                  if (precision > 4) {
                    precision = 0;
                  }
                  break;
                  
                case '?': // Tell the user what commands are available.
                  Serial.print("\nSMA SCP 0499 Serial Protocol\r");
                  Serial.print("\nAll cmds: <LF>cmd<CR>\r");
                  Serial.print("\nw           : averaged Weight\r");
                  Serial.print("\nz           : Zero scale\r");
                  Serial.print("\nd           : run Diagnostics\r");
                  Serial.print("\na           : About, first row\r");
                  Serial.print("\nb           : aBout, scroll\r");
                  Serial.print("\nr           : continuous Report\r");
                  Serial.print("\nt           : Tare scale\r");
                  Serial.print("\nc           : Clear tare\r");
                  Serial.print("\nm           : report tare weight\r");
                  Serial.print("\nxc          : enter Calibration mode\r");
                  Serial.print("\nxl          : toggle Lcd power\r");
                  Serial.print("\nxp          : scroll output Precision\r");
                  Serial.print("\nx?          : list all commands\r");
                  Serial.print("\nxc######.###: Calibrate with mass (needs 10 digits)\r");
                  // Give time for everything to send.
                  Serial.flush();
                  break;  
                  
                default:  // Unrecognized custom command.
                  Serial.print("\n?\r");
                  break;
              } // End of custom commands.
              break;
              
              // TODO: Add power saving commands/methods.
            default:  // Unrecognized 2 character command.
              Serial.print("\n?\r");
              break;
          } // End of 2 character commands.          
          break;
          
        case 12:  // Command with numeric input.
          // Commands will be an x, character, then 10 character number (with
          // leading whitespace).
          switch ((char) cmd[startIdx]) {
            case 'x': // Custom command.
            case 'X':
              switch ((char) cmd[startIdx + 1]) {
                case 'c': // Calibrate request.
                case 'C': { // Use brackets to prevent fall-through warnings.
                  // The calibration weight is submitted with 10 characters of 
                  // the value, plus 3 characters of units.
                  // TODO: handle multiple options for units.
                  clearTareSilent();
                  
                  // Read the requested calibration mass.
                  String calStandard = "";
                  for (int i = 0; i < 10; i++) {
                    calStandard += String((char) cmd[startIdx + 2 + i]);
                  }
                  // BUG: there seems to be an overflow issue for large inputs.
                  // BUG: toDouble() only returns two decimal points of 
                  //      precision.
                  cal_standard_mass = calStandard.toDouble();
                  
                  calResponse();
                  calibrate();
                  break;
                }
                
                default:  // Unrecognized custom, numeric input command.
                  Serial.print("\n?\r");
              } // End of custom commands.
              break;
          } // End of 12 character commands.
          break;
          
        default:  // Unrecognized command length.
          Serial.print("\n?\r");
          break;
      } // End of command interpretation.
    }

  
  //----Other----//
    void zero() {
      // Uses the HX711 built in tare() command to set the zero (empty bed).
      loadcell.tare(HX_NUM_AVGS);
      clearTareSilent();
      
      // Report instantaneous mass (the averaging window won't have caught up to
      // the change yet).
      mass = getMass();
      String massStr = rightJustify(String(mass, precision), WT_WIDTH);
      
      // SMA formatted response.
      String response = "\n";           // <LF>
      response += "Z";                  // <s>
      response += String(range);        // <r>
      response += String(netOrGross()); // <n>
      response += " ";                  // <m>
      response += " ";                  // <f>
      response += massStr;              // <xxxxxx.xxx>
      response += units;                // <uuu>
      response += "\r";                 // <CR>
      Serial.print(response);
    }
    
    
    void zeroSilent() {
      // Zero without serial response. Used for button-press and calibrate.
      loadcell.tare(HX_NUM_AVGS);
      clearTareSilent();
    }
    
    void tare() {
      // Sets the tareWeight to the current measured weight 
      // (accounting for current tare).
      tareWeight += mass;
      // Report instantaneous mass.
      reportMass();
    }
    
    
    void tareSilent() {
      // Silently change the tare (no serial output).
      tareWeight += mass;
    }
    
    
    void clearTare() {
      // Reset the tare.
      tareWeight = 0.0;
      // Report instantaneous mass.
      reportMass();
    }
    
    
    void clearTareSilent() {
      // Silently reset the tare.
      tareWeight = 0.0;
    }
    
    
    void reportTare() {
      // Report the tare weight over serial (response to 'M').
      String tareStr = rightJustify(String(tareWeight, precision), WT_WIDTH);
      // SMA formatted response.
      String response = "\n";     // <LF>
      response += " ";            // <s>
      response += String(range);  // <r>
      response += "T";            // <n>
      response += " ";            // <m>
      response += " ";            // <f>
      response += tareStr;        // <xxxxxx.xxx>
      response += units;          // <uuu>
      response += "\r";           // <CR>
      Serial.print(response);
    }
    
    
    String rightJustify(String str, int width) {
      // Sets a string right justified within a window. Used for formatting
      // numbers to SMA specification on serial output.
      int numWhtSpc = width - str.length();
      for (int i = 0; i < numWhtSpc; i++) {
        str = " " + str;
      }
      
      return str;
    }
    
    
    String netOrGross() {
      // Net/Gross status. Net if tared, Gross if tare = 0.
      if (tareWeight == 0) {
        return "G";
      } else {
        return "N";
      }
    }
    
    
    void reportMass() {
      // Reports the instantaneous mass over serial. Used for 'T', 'Z', 'XC'.
      mass = getMass();
      String massStr = rightJustify(String(mass, precision), WT_WIDTH);
      
      // SMA formatted response.
      String response = "\n";           // <LF>
      response += " ";                  // <s>
      response += String(range);        // <r>
      response += String(netOrGross()); // <n>
      response += " ";                  // <m>
      response += " ";                  // <f>
      response += massStr;              // <xxxxxx.xxx>
      response += units;                // <uuu>
      response += "\r";                 // <CR>
      Serial.print(response);
    }
    
    void reportMassAveraged() {
      // Reports averaged/filtered mass over serial. Used for 'W' and 'R'
      mass = getMassAveraged();
      String massStr = rightJustify(String(mass, precision), WT_WIDTH);
      
      // SMA formatted response.
      String response = "\n";           // <LF>
      response += " ";                  // <s>
      response += String(range);        // <r>
      response += String(netOrGross()); // <n>
      response += " ";                  // <m>
      response += " ";                  // <f>
      response += massStr;              // <xxxxxx.xxx>
      response += units;                // <uuu>
      response += "\r";                 // <CR>
      Serial.print(response);
    }
    
    
    void clearDisplay() {
      // Remove all information from the LCD.
      lcd.clear();
    }
    
    
    void printToDisplay(String output, int row, int col) {
      // Sends a formatted string to the LCD, starting at the requested 
      // location.
      lcd.setCursor(col, row);
      lcd.print(output);
    }
    
    
    // TODO: Check for overload.
    double getHxReadout() {
      // Read the raw (zeroed) value from the loadcell amplifier.
      return loadcell.get_value(HX_NUM_AVGS);
    }
    
    
    double getHxReadoutAveraged() {
      // Uses an array to read an average reading from the HX711 (not scaled for
      // loadcell sensitivity).
      // Shift the queue.
      for (int i = QUEUE_SIZE - 1; i > 0; i--) {
        hxQueue[i] = hxQueue[i-1];
      }
      
      // Place the current mass (24-bit unscaled number) in the queue.
      hxQueue[0] = getHxReadout();
      
      // Return the average value from the queue.
      double sum = 0;
      for (int i = 0; i < QUEUE_SIZE; i++) {
        sum += hxQueue[i];
      }
      
      return sum / QUEUE_SIZE;
    }
    
      
    double getMass() {
      // Reads instantaneous, tared mass.
      return getHxReadout() / sensitivity - tareWeight;
    }
    
    
    double getMassAveraged() {
      // Reads averaged, tared mass.
      return getHxReadoutAveraged() / sensitivity - tareWeight;
    }
    
    
    void displayMass(double mass) {
      // Shows the mass (whether it is instantaneous or averaged) on the LCD.
      clearDisplay();
      String massStr = rightJustify(String(mass, precision), WT_WIDTH);
      printToDisplay(massStr + " " + units, 0, 0);
    }
    
    
    void listenForButtonInput() {
      // Checks for a press of the tare button. Allows zero or calibration.
      switch (digitalRead(BTN_TARE)) {
        case 0: { // Use brackets to prevent fall-through warnings.
          // Prepare to measure how long the button is held.
          unsigned long time_of_press = millis();
          // Give the user an indication of detection on the display.
          printToDisplay(".", LCD_ROWS - 1, LCD_COLS - 1);
          
          while (digitalRead(BTN_TARE) == 0) {
            // Wait for button release.
          } // Button released.
          
          // Clear the '.' indicator once the button is released.
          printToDisplay(" ", LCD_ROWS - 1, LCD_COLS - 1);
          
          // Check when the button was released.
          unsigned long time_of_release = millis();
          
          // Behavior is determined by length of button press.
          if (time_of_release - time_of_press < CAL_WAIT) {
            // Use zero to simulate tare because there is no way to clear tare 
            // with the button.
            zeroSilent();
          } else {
            // Calibrate if the button was held long enough.
            calibrate();
          }
          break; // End of button pressed response.
        }
        default: 
          // Buttons are active LOW; do nothing if button isn't pressed.
          break;
      } // End of button-press check.
    }
    
    
    void calResponse() {
      // SMA formatted response for calibration.
      String calStr = String(cal_standard_mass, precision);
      calStr = rightJustify(calStr, WT_WIDTH);
      
      String response = "\n";           // <LF>
      response += "C";                  // <s>
      response += String(range);        // <r>
      response += String(netOrGross()); // <n>
      response += " ";                  // <m>
      response += " ";                  // <f>
      response += calStr;               // <xxxxxx.xxx>
      response += units;                // <uuu>
      response += "\r";                 // <CR>
      Serial.print(response);
    }
    
    
    void calibrate() {
      // Calibration sequence.
      // Tell the user what mass to use.
      printToDisplay("Cal w/ " + String(cal_standard_mass, precision) + 
        " " + units, 0, 0);
        
      // Ensure the scale is zeroed.
      zeroSilent();
      
      // Wait for the mass to get added (no use averaging with no weight on the
      // scale).
      printToDisplay("Add mass...", 1, 0);
      while (getHxReadout() < CAL_THRESHOLD) {
        // Wait for obvious addition of mass.
        // Allow user interrupt - cancel if new command received or button
        // pushed.
        if (Serial.available() > 2 || digitalRead(BTN_TARE) == 0) {
          return;
        }
      } // Mass apparently added.
      
      // Allow the loadcell to settle.
      delay(1000);
      
      // Clear the averaging queue.
      initQueue();
      double hxReadout;
      
      // Fill the averaging queue.
      for (int i = QUEUE_SIZE; i>0; i--) {
        // Add extra space to account for change in number of digits displayed.
        printToDisplay("Avg rem: " + String(i) + " ", 1, 0);
        hxReadout = getHxReadoutAveraged();
        delay(1000);
      }
      
      // Determine the new sensitivity.
      sensitivity = hxReadout / cal_standard_mass;
      
      // Save the sensitivity to hard memory for next time.
      setSensitivity();
      
      // Report an instantaneous mass so the user can see if the calibration was
      // successful.
      reportMass();
    }
    
    
    void getSensitivity() {
      // Fetch the stored sensitivity value from memory.
      Serial.print("\nReading sensitivity from memory...");
      
      char cal_check;
      EEPROM.get(CAL_SIGNATURE_ADDR, cal_check);
      
      if (cal_check != CAL_SIGNATURE) {
        Serial.print("No sensitivity stored in memory.\r");
      } else {
        EEPROM.get(CAL_VALUE_ADDR, sensitivity);
      }
      
      reportSensitivity();
    }
    
    
    void setSensitivity() {
      // Send the current sensitivity to memory silently.
      EEPROM.put(CAL_SIGNATURE_ADDR, CAL_SIGNATURE);
      EEPROM.put(CAL_VALUE_ADDR, sensitivity);
    }
    
    
    void reportSensitivity() {
      // No longer used.
      Serial.print("\nSensitivity: " + String(sensitivity, precision) + 
        " div/" + units + "\r\n\r");
    }
    
    
    void softReset() {
      // Reset the software. This seems to jump back to setup(), but not 
      // completely reset the Arduino.
      asm volatile (" jmp 0");
    }