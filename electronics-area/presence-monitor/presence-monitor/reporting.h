#pragma once

typedef struct{
  bool pirSensor;
  float batteryVoltage;
  long wifiRssi;
  long connectFailures;
  long reportingFailures;
} ReportData;
