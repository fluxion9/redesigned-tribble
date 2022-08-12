#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>

#define chipSelect 8
#define ONE_WIRE_BUS 9
#define cin A3
#define vin A5

unsigned long dumpInterval = 68000;
#define boiler 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
unsigned long lastTime = 0;
String dataString, localTime;
float temp1;
void epochToLocal(unsigned long unixEpoch)
{
  long second = unixEpoch % 60;
  long minute = (unixEpoch / 60) % 60;
  long hour = (unixEpoch / 3600) % 24;
  long day = (unixEpoch / 86400) % 365;
  localTime = "";
  localTime += String(day) + ':';
  localTime += String(hour) + ':';
  localTime += String(minute) + ':';
  localTime += String(second);
}

void measureTemperatures()
{
  sensors.requestTemperatures();
  float temp1C = sensors.getTempCByIndex(0);
  if (temp1C != DEVICE_DISCONNECTED_C)
  {
    temp1 = temp1C;
  }
  else
  {
    temp1 = 0;
  }
}
void setup() {
  pinMode(boiler, 1);
  if (!SD.begin(chipSelect)) {
    while (1);
  }
  sensors.begin();
  dataString = "Temp1 (*C),Voltage (V),Current (I),Power (W),Time Stamp";
  File dataFile = SD.open("datalog2.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }
  else {
    ;
  }
}

void loop() {
  if (millis() - lastTime >= dumpInterval)
  {
    dataString = "";
    float v = measureVoltageAC(vin);
    float i = measureCurrentAC();
    float p = v * i;
    measureTemperatures();
    dataString += String(temp1, 1) + ",";
    dataString += String(v, 0) + ",";
    dataString += String(i, 0) + ",";
    dataString += String(p, 0) + ",";
    epochToLocal(millis() / 1000);
    dataString += localTime;
    File dataFile = SD.open("datalog2.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
    }
    else {
      ;
    }
    lastTime = millis();
  }
  float avgTemp = (temp1 + temp1) / 2.0;
  if(avgTemp >= 80.0)
  {
    digitalWrite(boiler, 0);
  }
  else {
    digitalWrite(boiler, 1);
  }
  dataString = "";
}
float measureVoltageAC(int pin)
{
  float val = 0;
  float maxpk = 0, RMS = 0;
  unsigned long Time = millis(), sampleTime = 2000;
  while (millis() - Time <= sampleTime)
  {
    for (int i = 0; i < 300; ++i)
    {
      val += analogRead(pin);
      val /= 2;
    }
    if (val <= 0)
    {
      maxpk = 0;
    }
    else
    {
      if (val > maxpk)
      {
        maxpk = val;
      }
    }
  }
  maxpk = (maxpk * 505.0) / 1023.0;
  RMS = maxpk * 0.707;
  return RMS;
}
double measureCurrentAC()
{
  double Voltage = getVPP();
  double VRMS = (Voltage / 2.0) * 0.707; //root 2 is 0.707
  double AmpsRMS = (VRMS * 1000) / 100;
  return AmpsRMS;
}
float getVPP()
{
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here

  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) //sample for 1 Sec
  {
    readValue = analogRead(cin);
    // see if you have a new maxValue
    if (readValue > maxValue)
    {
      /*record the maximum sensor value*/
      maxValue = readValue;
    }
    if (readValue < minValue)
    {
      /*record the minimum sensor value*/
      minValue = readValue;
    }
  }

  // Subtract min from max
  result = ((maxValue - minValue) * 5.0) / 1024.0;
  return result;
}
