#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <HX711.h>
#include "filters.h"
#include <ESP32Servo.h>
using std::numeric_limits;

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define calibrationSamplesCount 400
#define pressureSamplesCountToAverage 30 // limited by typeof(pressureCheckCount) below

#define motorIterationCount 30
#define motorIterationTime 2000 // ms

#define RXD2 16
#define TXD2 17

#define scaleDT 18
#define scaleSCK 5

#define motorDT 19
#define motorFreq 50 // Hz
#define motorReslution 16 // bits
#define motorPWMchannel 0

const char* ssid = "ASUS_X00TDB";
const char* password = "9116927499";

float pressureK = 0.4;
float scaleK = 0.2;

AsyncWebServer server(80);

Adafruit_ADS1115 adXL(0x48);
Adafruit_ADS1115 adL(0x49);
Adafruit_ADS1115 adM(0x4B);
Adafruit_ADS1115 adS(0x4A);

HX711 scale;

filters XLs;
filters XLd;
filters Ls;
filters Ld;
filters Ms;
filters Md;
filters Ss;
filters Sd;

Servo motor;

int16_t pressure[8][3];
double filteredPressure[8];
bool readingFlag = false;
double averageCalibration[8];
uint16_t calibrationSamples = 0;
bool calibrationFlag = true;
double pressureSamplesArrayToAverage[8][pressureSamplesCountToAverage];
double sumForAveragePressure[8];
uint8_t pressureCheckCount = 0;
bool averageSucess = false;
String logInfo = "";
bool motorON = false;
uint8_t motorIteration = 0;
bool calibrationLog = false;

unsigned long wifiConnectLogTime;

unsigned long lastChecking;
unsigned long readingTime;
unsigned long lastMotorUpdating;
unsigned long lastMotorPrint;
void check_pressure();
String form_pressure();
//void ring();
void calibration();
void check_port();
void motor_control(uint8_t steps);
void update_motor();

template <typename T>
void shift(T* bufToShift, uint16_t n); // (массив, размер)

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("I'm ready!");
  delay(10);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(10);
  Serial2.setTimeout(100);
  delay(2000);

  // ledcSetup(motorPWMchannel, motorFreq, motorReslution);
  // ledcAttachPin(motorDT, motorPWMchannel);

  motor.attach(motorDT);

  // Serial.println("Motor setting...");
  // motor_control(255);
  // Serial.println("full");
  // delay(8000);
  // motor_control(1);
  // Serial.println("zero");
  // delay(8000);
  // Serial.println("Motor ready?");

  if(numeric_limits<typeof(pressureCheckCount)>::max() <= pressureCheckCount - 1) Serial.println("WARNING! Supremum of pressureCheckCount type is less than pressureCheckCount value!");

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }else{
    Serial.println("SPIFFS ready");
  }
  
  WiFi.begin(ssid, password);
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js");
  });
  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.svg");
  });

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
    if(readingFlag){
      readingTime = millis();
      while(readingFlag){
        if(millis() - readingTime > 20) break;
      }
    }
    if(!readingFlag){
      String answer = "";
      Serial.print("ARGS:: ");
      Serial.println(request->args());
      if(request->getParam("getPressure", true)->value() == "1"){
        //Serial.println("Sending pressure...");
        Serial.println("getPressure()");
        answer += (form_pressure() + "\n");
        Serial.println("getPressure() next step");
        // request->send(200, "text/plain", form_pressure());
        // request->args();
      }
      if(request->getParam("calibration", true)->value() == "1"){
        Serial.println("calibration() param");
        calibrationFlag = true;
        // request->send(200, "text/plain", "OK");
        // request->args();
        answer += "OK\n";
      }
      if(request->getParam("getMagnusParams", true)->value() == "1"){
        answer += "";
        Serial.println("getMagnus param");
      }
      if(request->getParam("motorStart", true)->value() == "1"){
        motorON = true;
        answer += "Motor started.\n";
        Serial.println("MotorStartedParam param");
      }
      if(request->getParam("motorStop", true)->value() == "1"){
        motorON = false;
        answer += "Motor stopped.\n";
        Serial.println("calibration() param");
      }
      request->send(200, "text/plain", answer);
      // Serial.println(request->args());
    }
  });

  server.begin();

  adXL.setGain(GAIN_SIXTEEN);
  adL.setGain(GAIN_SIXTEEN);
  adM.setGain(GAIN_SIXTEEN);
  adS.setGain(GAIN_SIXTEEN);
  adXL.begin();
  adL.begin();
  adM.begin();
  adS.begin();

  scale.begin(scaleDT, scaleSCK, 128); // DT, SCK, gain
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && millis() - wifiConnectLogTime > 1000){
    Serial.println("Connecting to WiFi..");
    //Serial.println(scale.read_average(40));
    wifiConnectLogTime = millis();
  }else if(millis() - wifiConnectLogTime > 2000){
    Serial.println(WiFi.localIP());
    //Serial.println(scale.read_average(40));
    wifiConnectLogTime = millis();
  }
  check_pressure();
  check_port();
  update_motor();
}

void check_pressure(){
  if(millis() - lastChecking >= 24){
    //Serial.println("Checking pressure...");
    //ring();
    for(byte i = 0; i < 8; i++){
      shift(pressure[i], 3);
    }
    pressure[0][2] = adXL.readADC_Differential_0_1();
    pressure[1][2] = adXL.readADC_Differential_2_3();
    pressure[2][2] = adL.readADC_Differential_0_1();
    pressure[3][2] = adL.readADC_Differential_2_3();
    pressure[4][2] = adM.readADC_Differential_0_1();
    pressure[5][2] = adM.readADC_Differential_2_3();
    pressure[6][2] = adS.readADC_Differential_0_1();
    pressure[7][2] = adS.readADC_Differential_2_3();

    readingFlag = true;
    filteredPressure[0] = XLs.expRunningAverage(XLs.median(pressure[0]), pressureK, filteredPressure[0]);
    filteredPressure[1] = XLd.expRunningAverage(XLd.median(pressure[1]), pressureK, filteredPressure[1]);
    filteredPressure[2] = Ls.expRunningAverage(Ls.median(pressure[2]), pressureK, filteredPressure[2]);
    filteredPressure[3] = Ld.expRunningAverage(Ld.median(pressure[3]), pressureK, filteredPressure[3]);
    filteredPressure[4] = Ms.expRunningAverage(Ms.median(pressure[4]), pressureK, filteredPressure[4]);
    filteredPressure[5] = Md.expRunningAverage(Md.median(pressure[5]), pressureK, filteredPressure[5]);
    filteredPressure[6] = Ss.expRunningAverage(Ss.median(pressure[6]), pressureK, filteredPressure[6]);
    filteredPressure[7] = Sd.expRunningAverage(Sd.median(pressure[7]), pressureK, filteredPressure[7]);
    readingFlag = false;

    if(pressureCheckCount < pressureSamplesCountToAverage){
      for(byte i = 0; i < 8; i++){
        shift(pressureSamplesArrayToAverage[i], pressureSamplesCountToAverage);
        pressureSamplesArrayToAverage[i][pressureSamplesCountToAverage - 1] = filteredPressure[i];
        sumForAveragePressure[i] += filteredPressure[i];
      }
    }else{
      //Serial.println("Average SUCESS!");
      averageSucess = true;
      //Serial.print(pressureSamplesArrayToAverage[0][0]); Serial.print(" "); Serial.println(sumForAveragePressure[0]);
      for(byte i = 0; i < 8; i++){
        sumForAveragePressure[i] -= pressureSamplesArrayToAverage[i][0];
        shift(pressureSamplesArrayToAverage[i], pressureSamplesCountToAverage);
        sumForAveragePressure[i] += filteredPressure[i];
        pressureSamplesArrayToAverage[i][pressureSamplesCountToAverage - 1] = filteredPressure[i];
      }
      //Serial.print(filteredPressure[0]); Serial.print(" "); Serial.println(sumForAveragePressure[0]);
    }
    // for(byte i = 0; i < pressureSamplesCountToAverage; i++){
    //   Serial.print(pressureSamplesArrayToAverage[0][i]);
    //   Serial.print(" ");
    // }
    // Serial.println();
    
    // for(byte i = 0; i < 8; i++){
    //   Serial.print(pressure[i][2]);
    //   Serial.print(" ");
    // }
    // Serial.println();
    // for(byte i = 0; i < 8; i++){
    //   Serial.print(filteredPressure[i]);
    //   Serial.print(" ");
    // }
    // Serial.println();
    // Serial.println("---");

    //Serial.print(fil1.getId()); Serial.print(": "); Serial.print(res); Serial.print(" "); Serial.print(fil1.median(res)); Serial.print(" "); Serial.print((int32_t)fil1.expRunningAverage(res, 0.5)); Serial.print(" "); Serial.println((int32_t)fil1.all(res, 0.5));
    //Serial.println("---");
    if(averageSucess && millis() > 2000) calibration();
    if(numeric_limits<typeof(pressureCheckCount)>::max() > pressureCheckCount + 1) pressureCheckCount++;
    lastChecking = millis();
    //Serial.println(pressureCheckCount);
  }
}

String form_pressure(){
  String answer = "";
  for(int i = 0; i < 8; i++){
    // Serial.print(sumForAveragePressure[i]);
    // Serial.print(" ");
    // if(i < 7) answer += (String((sumForAveragePressure[i] / (double)pressureSamplesCountToAverage) + (averageCalibration[1] - averageCalibration[i])) + "@");
    // else answer += String((sumForAveragePressure[i] / (double)pressureSamplesCountToAverage) + (averageCalibration[1] - averageCalibration[i]));
    if(i < 7) answer += (String((sumForAveragePressure[i] / (double)pressureSamplesCountToAverage) - averageCalibration[i]) + "@");
    else answer += String((sumForAveragePressure[i] / (double)pressureSamplesCountToAverage) - averageCalibration[i]);
  }
  if(logInfo != ""){
    answer += ("@" + logInfo);
    logInfo = "";
  }
  Serial.println(answer);
  return answer;
}

// void ring(){
//   for(byte i = 0; i < 8; i++){
//     pressure[i][0] = pressure[i][1];
//     pressure[i][1] = pressure[i][2];
//   }
// }

template <typename T>
void shift(T* bufToShift, uint16_t n){ // (массив, размер)
  // Serial.print("shift() "); Serial.println(n);
  // for(byte i = 0; i < n; i++){
  //   Serial.print(bufToShift[i]);
  //   Serial.print(" ");
  // }
  // Serial.println();

  for(byte i = 1; i < n; i++){
    bufToShift[i-1] = bufToShift[i];
  }
}

void calibration(){
  //if(calibrationFlag) Serial.println("Calibration...");
  if(calibrationFlag && calibrationSamples < calibrationSamplesCount){
    calibrationSamples++;
    for(byte i = 0; i < 8; i++){
      averageCalibration[i] += (sumForAveragePressure[i] / (double)pressureSamplesCountToAverage);
    }
  }
  if(calibrationFlag && !calibrationLog){
    logInfo += "Calibration...";
    calibrationLog = true;
  }
  if(calibrationFlag && calibrationSamples == calibrationSamplesCount){
    for(byte i = 0; i < 8; i++){
      averageCalibration[i] /= calibrationSamplesCount;
    }
    calibrationSamples = 0;
    calibrationFlag = false;
    calibrationLog = false;

    for(byte j = 0; j < 7; j++){
      Serial.print(averageCalibration[j]);
      Serial.print(" ");
    }
    Serial.println(averageCalibration[7]);

    for(byte a = 0; a < 8; a++){
      Serial.print((sumForAveragePressure[a] / (double)pressureSamplesCountToAverage));
      Serial.print(" ");
    }

    Serial.println();
    Serial.println("CALIBRATION SUCCES!");
    logInfo += "\nCalibration succes!";
  }
}

void check_port(){
  if(Serial2.available() > 0){
    //Serial.print(Serial2.readString());
  }
}

void motor_control(uint8_t steps){
  // ledcWrite(motorPWMchannel, (map(steps, 0, 255, 3276, 6553) - 10));
  motor.writeMicroseconds(map(steps, 0, 255, 1000, 2000));
  // if(millis() - lastMotorPrint > 500){
  //   Serial.print("Motor speed: ");
  //   Serial.print(steps);
  //   Serial.print("steps ");
  //   Serial.print(map(steps, 0, 255, 1000, 2000));
  //   Serial.println("us");
  //   lastMotorPrint = millis();
  // }
}

void update_motor(){
  //Serial.println("Updating motor...");
  if(millis() - lastMotorUpdating > motorIterationTime){ 
    if(motorON && motorIteration >= motorIterationCount){
      motor_control(0);
      motorIteration = 0;
      motorON = false;
    }
    if(motorON && motorIteration < motorIterationCount){
      motor_control((motorIteration + 1) * (256 / motorIterationCount));
      motorIteration++;
    }
    lastMotorUpdating = millis();
  }
  if(!motorON){
    motor_control(0);
    motorIteration = 0;
  }
}