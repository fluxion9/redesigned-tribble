#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#define chipSelect 10
#define ONE_WIRE_BUS 2
#define cin A4
#define vin A1
#define luxin A5
#define flowIn A3
#define boiler 9
unsigned long dumpInterval = 66000;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
unsigned long lastTime = 0;
String dataString, localTime;
float temp1, temp2, temp3, temp4;
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
float measureIrradians()
{
  float voltage = analogRead(luxin);
  voltage = (voltage * 5.0) / 1023.0; //volts
  float illuminance = 2500 / voltage;
  illuminance -= 500;
  illuminance /= 10; //lux
  float irradiance = illuminance / 120.0;
  return irradiance;
}

float measureFlowRate()
{
  unsigned long h, l, t;
  unsigned long tprev = millis();
  float f, freq = 0, count = 0, flow = 0;
  while(millis() - tprev < 2000)
  {
    h = pulseIn(flowIn, 1);
    l = pulseIn(flowIn, 0);
    t = h + l;
    f = 1000000UL / t;
    freq += f;
    count++;
  }
  freq /= count;
  flow = freq / 7.5; // L/Min
  if(flow > 30 || flow < 0)
  {
    return 0;
  }
  else 
  {
    return flow;
  }
}
void measureTemperatures()
{
  sensors.requestTemperatures();
  float temp1C = sensors.getTempCByIndex(0);
  float temp2C = sensors.getTempCByIndex(1);
  float temp3C = sensors.getTempCByIndex(2);
  float temp4C = sensors.getTempCByIndex(3);
  if (temp1C != DEVICE_DISCONNECTED_C || temp2C != DEVICE_DISCONNECTED_C || temp3C != DEVICE_DISCONNECTED_C || temp4C != DEVICE_DISCONNECTED_C)
  {
    temp1 = temp1C;
    temp2 = temp2C;
    temp3 = temp3C;
    temp4 = temp4C;
  }
  else
  {
    temp1 = 0;
    temp2 = 0;
    temp3 = 0;
    temp4 = 0;
  }
}
void setup() {
  Serial.begin(9600);
  pinMode(boiler, 1);
  pinMode(flowIn, 0);
  while (!SD.begin(chipSelect)) {
    Serial.println("failed to mount sd. ");
    delay(3000);
  }
  Serial.println("SD card mounted successfully. ");
  sensors.begin();
  dataString = "Temp1 (*C),Temp2 (*C),Temp3 (*C),Temp4 (*C),Voltage (V),Current (I),Power (W),Irradiance (W/M^2),Flow Rate (L/Min),Time Stamp (DD:HH:MM:SS)";
  Serial.println(dataString);
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }
  else {
    Serial.println("Bad SD card. ");
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
    float irrad = measureIrradians();
    float rate = measureFlowRate();
    dataString += String(temp1, 1) + ",";
    dataString += String(temp2, 1) + ",";
    dataString += String(temp3, 1) + ",";
    dataString += String(temp4, 1) + ",";
    dataString += String(v, 0) + ",";
    dataString += String(i, 0) + ",";
    dataString += String(p, 0) + ",";
    dataString += String(irrad) + ",";
    dataString += String(rate) + ",";
    epochToLocal(millis() / 1000);
    dataString += localTime;
    Serial.println(dataString);
    File dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
    }
    else {
      Serial.println("Bad SD card. ");
    }
    lastTime = millis();
  }
  float avgTemp = (temp1 + temp2) / 2.0;
  if(avgTemp >= 70.0)
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
