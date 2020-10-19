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
  int httpCode = ThingSpeak.setField(1, pData->wifiRssi);
  if(httpCode != 200) {
    Serial.println("Problem setting field 1");
    return -1;
  }

  httpCode = ThingSpeak.setField(2, pData->batteryVoltage);
  if(httpCode != 200) {
    Serial.println("Problem setting field 2");
    return -1;
  }

  httpCode = ThingSpeak.setField(3, pData->pirSensor);
  if(httpCode != 200) {
    Serial.println("Problem setting field 3");
    return -1;
  }

  httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (httpCode == 200) {
    Serial.println("Channel write successful.");
  } else {
    Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
    return -1;
  }
  return 0;
}
