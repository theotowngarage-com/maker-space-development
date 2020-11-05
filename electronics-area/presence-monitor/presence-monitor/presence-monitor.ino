/*
 * Write data to ThingSpeak channel
 * Channels used:
 * 1 - WiFi RSSI
 * 2 - battery voltage (2S LiPo battery connected through voltage divider)
 * 3 - PIR motion sensor state
 * 4 - Number of WiFi connection failures
 * 5 - Number of network errors while sending data
 * 6 - Temperature
*/

#include <ESP8266WiFi.h>
#include "secretss.h"
#include "reporting.h"
#include <coredecls.h>         // crc32()
#include <PolledTimeout.h>
#include <include/WiFiState.h> // WiFiState structure details
#include <DallasTemperature.h>

//#define DEBUG  // prints WiFi connection info to serial, uncomment if you want WiFi messages
#ifdef DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_PRINT(x)  Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

#define PIR_INPUT 13 // D7
#define ONE_WIRE_BUS 12 // D6

const float analogToVoltage = 110.5f;
const float batteryRunningAvgCoeff = 0.0625;
const uint32_t noActivityIntervalmSec = 30 * 60e3;
const uint32_t pirActiveIntervalmSec = 20e3;
const uint32_t sleepTimemSec = 3000;
const int temperatureResolutionBits = 9; // 9-12: higher resolution conversion uses more time

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password

IPAddress staticIP(0, 0, 0, 0); // parameters below are for your static IP address, if used
IPAddress gateway(0, 0, 0, 0);
IPAddress subnet(0, 0, 0, 0);
IPAddress dns1(0, 0, 0, 0);
IPAddress dns2(0, 0, 0, 0);
const uint32_t timeout = 30E3;  // 30 second timeout on the WiFi connection

#define FLAG_LAST_PIR_STATE (1 << 0)
#define FLAG_PIR_LAST_SEND (1 << 1)
#define FLAG_HAS_WIFI (1 << 3)
// This structure is stored in RTC memory to save the WiFi state and reset count (number of Deep Sleeps),
// and it reconnects twice as fast as the first connection; it's used several places in this demo
struct nv_s {
  WiFiState wss; // core's WiFi save state

  uint32_t crc32;
  struct {
    uint32_t rstCount;  // stores the Deep Sleep reset count
    // you can add anything else here that you want to save, must be 4-byte aligned
    uint32_t flags;
    uint32_t lastResetTimemSec;
    uint32_t lastReportmSec;
    int32_t connectFailures;
    int32_t reportingFailures;
    float batteryVoltage;
  } rtcData;
};

static nv_s* nv = (nv_s*)RTC_USER_MEM; // user RTC RAM area

esp8266::polledTimeout::oneShotMs wifiTimeout(timeout);  // 30 second timeout on WiFi connection

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
uint8_t tempDeviceAddress;

bool pirActivated = false;

void wakeupCallback() {  // unlike ISRs, you can do a print() from a callback function
  Serial.println(F("Woke from Light Sleep - this is the callback"));
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIR_INPUT, INPUT);
  
  Serial.begin(115200);
  Serial.println();
  DEBUG_PRINT(F("Core Version: "));
  DEBUG_PRINT(ESP.getCoreVersion());
  DEBUG_PRINT(F(", SDK Version: "));
  DEBUG_PRINTLN(ESP.getSdkVersion());

  String resetCause = ESP.getResetReason();
  DEBUG_PRINTLN(resetCause);

  // Read data from RTC memory, if any
  uint32_t crcOfData = crc32((uint8_t*) &nv->rtcData, sizeof(nv->rtcData));
  if ((crcOfData == nv->crc32) && (resetCause == "Deep-Sleep Wake")) {
    DEBUG_PRINTLN(F("RTC data valid"));
    if(nv->rtcData.flags & FLAG_HAS_WIFI) {
      DEBUG_PRINTLN(F("WIFI module enabled"));
    } else {
      DEBUG_PRINTLN(F("WIFI module disabled"));
    }
    nv->rtcData.rstCount++;
  } else {
    memset(&nv->rtcData, 0, sizeof(nv->rtcData));
  }
  updateRTCcrc();

  // start reporting library
  beginReporting();
}

void loop() {
  uint32_t nextSleepTimemSec = sleepTimemSec;
  // ------- data needed to determine if message needs to be sent ---------
  // monitoring battery voltage - measurements are unstable, use weighted moving average
  float rawVoltage = analogRead(A0) / analogToVoltage;
  if(nv->rtcData.batteryVoltage == 0) {
    nv->rtcData.batteryVoltage = rawVoltage;
  } else {
    nv->rtcData.batteryVoltage *= 1.0 - batteryRunningAvgCoeff;
    nv->rtcData.batteryVoltage += rawVoltage * batteryRunningAvgCoeff;
  }
  DEBUG_PRINT(F("Vin="));
  DEBUG_PRINTLN(rawVoltage);
  DEBUG_PRINT(F("Vin(avg)="));
  DEBUG_PRINTLN(nv->rtcData.batteryVoltage);
  pirActivated = digitalRead(PIR_INPUT);
  DEBUG_PRINT(F("PIR input: "));
  DEBUG_PRINTLN(pirActivated);
  if(pirActivated) {
    nv->rtcData.flags |= FLAG_LAST_PIR_STATE;
    updateRTCcrc();
  }

  // ------- decide if message needs to be sent now ----------
  // Throttle data send to specified intervals.
  // Send data more frequenly if PIR was activated in recent wakeups
  // Back off for longer interval every 8th wifi connection failure saving battery
  bool sendData = false;
  bool wifiErrorLimitExceeded = nv->rtcData.connectFailures + nv->rtcData.reportingFailures >= 8;
  if((nv->rtcData.flags & FLAG_LAST_PIR_STATE || nv->rtcData.flags & FLAG_PIR_LAST_SEND)
      && !wifiErrorLimitExceeded) {
    sendData = hasIntervalElapsed(pirActiveIntervalmSec);
  } else {
    sendData = hasIntervalElapsed(noActivityIntervalmSec);
  }

  // ----------- apply error condition filters ---------------
  // disable transmissions, and increase deep sleep duration 
  // when battery voltage is very low
  if(nv->rtcData.batteryVoltage < 6.2) {
    Serial.println("WARNING: Low battery");
    // extreme power saving when battery low detected
    nextSleepTimemSec *= 10;
    sendData = false;
  }

  // increase deep sleep duration if wifi error limits is exceeded
  if(wifiErrorLimitExceeded) {
    nextSleepTimemSec *= 10;
  }

  // -------- Actual message transmission block ------------
  if(sendData && nv->rtcData.flags & FLAG_HAS_WIFI) {
    ReportData data;
    data.temperature = getTemperature();
    data.pirSensor = nv->rtcData.flags & FLAG_LAST_PIR_STATE;
    data.batteryVoltage = nv->rtcData.batteryVoltage;
    data.connectFailures = nv->rtcData.connectFailures;
    data.reportingFailures = nv->rtcData.reportingFailures;
    
    if(initWiFi()) {
      // Measure Signal Strength (RSSI) of Wi-Fi connection
      data.wifiRssi = WiFi.RSSI();
    
      if(sendReport(&data) == 0) {
        // update nvram data after success
        if(nv->rtcData.flags & FLAG_LAST_PIR_STATE) {
          nv->rtcData.flags |= FLAG_PIR_LAST_SEND;
        } else {
          nv->rtcData.flags &= ~FLAG_PIR_LAST_SEND;
        }
        nv->rtcData.flags &= ~FLAG_LAST_PIR_STATE; 
        nv->rtcData.lastReportmSec = nv->rtcData.lastResetTimemSec + millis(); // crc will be updated before sleep
        // confirm correct data send for deep sleep
        sendData = false;
        nv->rtcData.reportingFailures = 0;
        nv->rtcData.connectFailures = 0;
      } else {
        nv->rtcData.reportingFailures++;
      }
    } else {
      nv->rtcData.connectFailures++;
    }
    // Save current wifi state to nvram
    if(!WiFi.mode(WIFI_SHUTDOWN, &nv->wss)) {
      Serial.println(F("ERROR: Failed to go to WIFI_SHUTDOWN"));
    }
  }

  // ------------- deep sleep ------------
  // after deep sleep mode is activated, chip is waking up only by reset
  if(sendData) {
    // quickly wakeup with RF enalbed when data send was requested, but RF was disabled
    nv->rtcData.flags |= FLAG_HAS_WIFI;
    // keep track of time elapsed between resets - update immediately before entering sleep
    // time is adjusted for time spent in sleep
    nv->rtcData.lastResetTimemSec += millis() + 50;
    updateRTCcrc();
    ESP.deepSleep(50, WAKE_RF_DEFAULT);
  } else {
    nv->rtcData.flags &= ~FLAG_HAS_WIFI;
    // keep track of time elapsed between resets - update immediately before entering sleep
    // time is adjusted for time spent in sleep
    nv->rtcData.lastResetTimemSec += millis() + nextSleepTimemSec;
    updateRTCcrc();
    ESP.deepSleep(nextSleepTimemSec * 1000, WAKE_RF_DISABLED);
  }
  while(true){
    yield();
  }
}

bool hasIntervalElapsed(uint32_t intervalmSec) {
  DEBUG_PRINT(F("Last send time [ms]: "));
  DEBUG_PRINTLN(nv->rtcData.lastReportmSec);
  DEBUG_PRINT(F("Current time [ms]: "));
  DEBUG_PRINTLN(nv->rtcData.lastResetTimemSec + millis());
  DEBUG_PRINT(F("Interval [ms]: "));
  DEBUG_PRINTLN(intervalmSec);
  return nv->rtcData.lastResetTimemSec + millis() - nv->rtcData.lastReportmSec > intervalmSec;
}

float getTemperature() {
    sensors.begin();
    DEBUG_PRINT(F("Temperature sensors: "));
    DEBUG_PRINTLN(sensors.getDS18Count());
    if(sensors.getDS18Count() > 0) {
      sensors.getAddress(&tempDeviceAddress, 0);
      sensors.setResolution(&tempDeviceAddress, temperatureResolutionBits);
      sensors.setWaitForConversion(false);
      sensors.requestTemperatures();
      delay(750 / (1 << (12 - temperatureResolutionBits)));
      DEBUG_PRINT(F("Temperature: "));
      DEBUG_PRINTLN(sensors.getTempCByIndex(0));
      return sensors.getTempCByIndex(0);
    }
    return -100.0;
}

void updateRTCcrc() {
  nv->crc32 = crc32((uint8_t*) &nv->rtcData, sizeof(nv->rtcData));
}

bool initWiFi() {
  bool result = false;
  digitalWrite(LED_BUILTIN, LOW);  // give a visual indication that we're alive but busy with WiFi
  uint32_t wifiBegin = millis();  // how long does it take to connect
  Serial.println(F("INFO: resuming WiFi"));
  if (!WiFi.mode(WIFI_RESUME, &nv->wss)) {  // couldn't resume, or no valid saved WiFi state yet
    /* Explicitly set the ESP8266 as a WiFi-client (STAtion mode), otherwise by default it
      would try to act as both a client and an access-point and could cause network issues
      with other WiFi devices on your network. */
    WiFi.persistent(false);  // don't store the connection each time to save wear on the flash
    WiFi.mode(WIFI_STA);
    WiFi.setOutputPower(10);  // reduce RF output power, increase if it won't connect
    WiFi.config(staticIP, gateway, subnet);  // if using static IP, enter parameters at the top
    WiFi.begin(ssid, pass);
    Serial.print(F("INFO: connecting to WiFi "));
    Serial.println(ssid);
    DEBUG_PRINT(F("my MAC: "));
    DEBUG_PRINTLN(WiFi.macAddress());
  }
  digitalWrite(LED_BUILTIN, HIGH);
  wifiTimeout.reset(timeout);
  while (((!WiFi.localIP()) || (WiFi.status() != WL_CONNECTED)) && (!wifiTimeout)) {
    yield();
  }
  if ((WiFi.status() == WL_CONNECTED) && WiFi.localIP()) {
    DEBUG_PRINTLN(F("WiFi connected"));
    Serial.print(F("INFO: WiFi connect time = "));
    float reConn = (millis() - wifiBegin);
    Serial.printf("%1.2f seconds\n", reConn / 1000);
    DEBUG_PRINT(F("INFO: WiFi Gateway IP: "));
    DEBUG_PRINTLN(WiFi.gatewayIP());
    DEBUG_PRINT(F("INFO: my IP address: "));
    DEBUG_PRINTLN(WiFi.localIP());
    result = true;
  } else {
    Serial.println(F("ERROR: WiFi timed out and didn't connect"));
  }
  WiFi.setAutoReconnect(true);
  return result;
}
