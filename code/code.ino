//App user Name: home_energyy
//App Pass Word: Projectiot@123

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFi.h>
#include "EmonLib.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

/******************** LCD ********************/
LiquidCrystal lcd(2, 15, 19, 18, 5, 4);

/******************** ENERGY MONITOR ********************/
EnergyMonitor emon;

/******************** EEPROM ********************/
#define EEPROM_SIZE 512

/******************** CLOUD VARIABLES ********************/
String on_off;
String power;
float curri;
float volti;
float unit;

/******************** LOCAL VARIABLES ********************/
unsigned long lastMillis = 0;
float energyUnits = 0;
float powerValue = 0;
float cost = 0;
const float RATE_PER_UNIT = 10.0;

/******************** RELAYS ********************/
#define RELAY_LIGHT 25
#define RELAY_FAN   26

/******************** IOT CLOUD ********************/
const char DEVICE_LOGIN_NAME[] = "6d6a330e-5687-405c-89a8-091ff2ae4587";
const char SSID[] = "projectiot";
const char PASS[] = "123456789";
const char DEVICE_KEY[] = "FnBG7vkdpi1#izeZBckfs0htC";

/******************** CALIBRATION ********************/
#define V_CALIBRATION 144.3
#define I_CALIBRATION 30.0

/******************** CLOUD CALLBACK ********************/
void onOnOffChange() {

  if (on_off == "A") {
    digitalWrite(RELAY_LIGHT, HIGH);
    Serial.println("Light ON");
  } 
  else if (on_off == "a") {
    digitalWrite(RELAY_LIGHT, LOW);
    Serial.println("Light OFF");
  } 
  else if (on_off == "B") {
    digitalWrite(RELAY_FAN, HIGH);
    Serial.println("Fan ON");
  } 
  else if (on_off == "b") {
    digitalWrite(RELAY_FAN, LOW);
    Serial.println("Fan OFF");
  } 
  else if (on_off == "C") {
    // 🔥 RESET EEPROM
    energyUnits = 0;
    EEPROM.put(0, energyUnits);
    EEPROM.commit();

    Serial.println("Energy Reset Done!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Energy Reset");
    lcd.setCursor(0, 1);
    lcd.print("Units = 0");
    delay(2000);
    lcd.clear();
  }
}

void onPowerChange() {}
void onCurriChange() {}
void onVoltiChange() {}
void onUnitChange() {}

/******************** INIT CLOUD ********************/
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);

  ArduinoCloud.addProperty(on_off, READWRITE, ON_CHANGE, onOnOffChange);
  ArduinoCloud.addProperty(power, READWRITE, ON_CHANGE, onPowerChange);
  ArduinoCloud.addProperty(curri, READWRITE, ON_CHANGE, onCurriChange);
  ArduinoCloud.addProperty(volti, READWRITE, ON_CHANGE, onVoltiChange);
  ArduinoCloud.addProperty(unit, READWRITE, ON_CHANGE, onUnitChange);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

/******************** SETUP ********************/
void setup() {

  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.print(" Smart Energy ");
  lcd.setCursor(0, 1);
  lcd.print(" IoT Monitor ");
  delay(2000);
  lcd.clear();

  pinMode(RELAY_LIGHT, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);

  digitalWrite(RELAY_LIGHT, LOW);
  digitalWrite(RELAY_FAN, LOW);

  /******** EEPROM INIT ********/
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, energyUnits);

  if (isnan(energyUnits) || energyUnits < 0 || energyUnits > 100000) {
    energyUnits = 0;
  }

  /******** ADC CONFIG ********/
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  analogSetPinAttenuation(35, ADC_11db);
  analogSetPinAttenuation(34, ADC_11db);

  emon.voltage(35, V_CALIBRATION, 1.7);
  emon.current(34, I_CALIBRATION);

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  WiFi.setSleep(false);
}

/******************** LOOP ********************/
bool showCost = false;
unsigned long lcdResetTimer = 0;

void loop() {

  ArduinoCloud.update();

  unsigned long now = millis();

  if (now - lastMillis >= 1000) {
    readEnergy();
    updateDisplay();
    lastMillis = now;
  }

  if (millis() - lcdResetTimer > 60000) {
    lcd.begin(16, 2);
    lcdResetTimer = millis();
  }
}

/******************** ENERGY ********************/
void readEnergy() {

  emon.calcVI(20, 2000);

  volti = emon.Vrms;
  curri = emon.Irms;

  if (volti < 50 || volti > 260) volti = 0;
  if (curri < 0.05) curri = 0;

  powerValue = volti * curri;

  energyUnits += (powerValue / 1000.0) * (1.0 / 3600.0);
  unit = energyUnits;

  cost = energyUnits * RATE_PER_UNIT;

  power = String(powerValue, 0) + "W | Rs." + String(cost, 2);

  Serial.print("Units: "); Serial.println(energyUnits, 4);

  /******** EEPROM SAVE ********/
  static unsigned long lastSave = 0;
  if (millis() - lastSave > 10000) {
    EEPROM.put(0, energyUnits);
    EEPROM.commit();
    lastSave = millis();
  }
}

/******************** LCD ********************/
void updateDisplay() {

  lcd.setCursor(0, 0);
  lcd.print("V");
  lcd.print(volti,1);
  lcd.print(" I");
  lcd.print(curri,1);
  lcd.print("  ");

  lcd.setCursor(0, 1);
  lcd.print("P");
  lcd.print(powerValue,0);

  if (!showCost) {
    lcd.print("W U");
    lcd.print(energyUnits,2);
    lcd.print(" ");
  } else {
    lcd.print("W Rs");
    lcd.print(cost,2);
    lcd.print(" ");
    
  }

  showCost = !showCost;
}
