#include <Arduino.h>
/*
SDA (INA219 / OLED) => D2 (на Wemos D1 Mini)
SCL (INA219 / OLED) => D1 (на Wemos D1 Mini)
          VCC / VIN => 3V3 или 5V (в зависимости от питания INA219 и OLED)
                GND => G (общая земля)
*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "SSD1306Wire.h" 
#include <Adafruit_INA219.h>

// Create OLED and INA219 globals.
SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_64_48);  // ADDRESS, SDA, SCL, OLEDDISPLAY_GEOMETRY  -  Extra param required for 128x32 displays.
Adafruit_INA219 ina219;

// Keep track of total time and milliamp measurements for milliamp-hour computation.
uint32_t total_sec = 0;
float total_mA = 0.0;

void printSIValue(float value, const char* units, int precision, int maxWidth) {
  display.print("   ");
  // Print a value in SI units with the units left justified and value right justified.
  // Will switch to milli prefix if value is below 1.

  // Add milli prefix if low value.
  if (fabs(value) < 1.0) {
    display.print('m');
    maxWidth -= 1;
    value *= 1000.0;
    precision = max(0, precision-3);
  }

  // Print units.
  display.print(units);
  maxWidth -= strlen(units);

  // Leave room for negative sign if value is negative.
  if (value < 0.0) {
    maxWidth -= 1;
  }

  // Find how many digits are in value.
  int digits = ceil(log10(fabs(value)));
  if (fabs(value) < 1.0) {
    digits = 1; // Leave room for 0 when value is below 0.
  }

  // Handle if not enough width to display value, just print dashes.
  if (digits > maxWidth) {
    // Fill width with dashes (and add extra dash for negative values);
    for (int i=0; i < maxWidth; ++i) {
      display.print('-');
      Serial.print('-');
    }
    if (value < 0.0) {
      display.print('-');
      Serial.print('-');
    }
    return;
  }

  // Compute actual precision for printed value based on space left after
  // printing digits and decimal point.  Clamp within 0 to desired precision.
  int actualPrecision = constrain(maxWidth-digits-1, 0, precision);

  // Compute how much padding to add to right justify.
  int padding = maxWidth-digits-1-actualPrecision;
  for (int i=0; i < padding; ++i) {
    display.print(' ');
  }

  // Finally, print the value!
  display.println(value, actualPrecision);
  Serial.print(value, actualPrecision);
}

void update_power_monitor() {
  // Read voltage and current from INA219.
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();

  // Compute load voltage, power, and milliamp-hours.
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  float power_mW = loadvoltage * current_mA;
  (void)power_mW;

  total_mA += current_mA;
  total_sec += 1;
  float total_mAH = total_mA / 3600.0;
  (void)total_mAH;

  // for debug
  // float loadvoltage = random(500)/100;
  // float current_mA = random(1000);
  // total_mA += current_mA;
  // float total_mAH = total_mA / 3600.0;
  // Update display.
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH); 

  // Mode 0, display volts and amps.
  printSIValue(loadvoltage, "V:", 2, 10);
  Serial.print(";");
  printSIValue(current_mA/1000.0, "A:", 5, 10);
  Serial.print(";");
  printSIValue(total_mAH, "mAh:", 1, 10);
  Serial.println();

 // write the buffer to the display
  display.display();
}

void setup()   {
  Serial.begin(9600);
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1); // Update display.
  }
 

  // Setup the OLED display.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  Serial.println("Power monitor v1.0");

  // Try to initialize the INA219
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH); 
    display.println("INA219 err");
    display.println("Check wiring");
    while (1) { delay(100); }
  }
  // By default the INA219 will be calibrated with a range of 32V, 2A.
  // However uncomment one of the below to change the range.  A smaller
  // range can't measure as large of values but will measure with slightly
  // better precision.
  //ina219.setCalibration_32V_1A();
  ina219.setCalibration_16V_400mA();
}

void loop() {
  update_power_monitor();
  
  delay(100);
}