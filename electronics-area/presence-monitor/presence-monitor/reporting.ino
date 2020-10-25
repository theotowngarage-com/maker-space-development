#include "ThingSpeak.h"
#include "secretss.h"

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
WiFiClient  client;

bool beginReporting() {
    ThingSpeak.begin(client);
}

int sendReport(ReportData *pData) {
    // Write value to Field 1 of a ThingSpeak Channel
  ThingSpeak.setField(1, pData->wifiRssi);
  ThingSpeak.setField(2, pData->batteryVoltage);
  ThingSpeak.setField(3, pData->pirSensor);
  ThingSpeak.setField(4, pData->connectFailures);
  ThingSpeak.setField(5, pData->reportingFailures);
  ThingSpeak.setField(6, pData->temperature);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (httpCode == 200) {
    Serial.println("Channel write successful.");
  } else {
    Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
    return -1;
  }
  return 0;
}
