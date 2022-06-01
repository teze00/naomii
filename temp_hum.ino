
// Include drivers
// DHT_sensor_library
#include <DHT.h>            // This is the DHT sensor library by Adafruit
#define DATA_PIN 4                    // Pin used to collect data from the DHT
#define DHTTYPE DHT11                 // DHT Type  
//#define DHTTYPE DHT22               // If you have a DHT22 comment the DHT11 and
                                      // uncomment DHT22
// Create the sensor object and assign pin
DHT dht(DATA_PIN, DHTTYPE);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// The pins for I2C are defined by the Wire-library.
// On an arduino UNO/Nano and Pro Mini: A4(SDA), A5(SCL)
// On an arduino MEGA 2560:             20(SDA), 21(SCL)
// On an arduino LEONARDO:              2(SDA),  3(SCL), ...
#define OLED_RESET   -1 // Reset pin # (or -1 if no reset on display)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address (Yours could be 0x3D)

#define MODE_BUTTON 2             // Activate display and switched between temperature, graph and humidity
#define RESET_BUTTON 3            // If pressed when data displayed resets min and max settings

float temp;       //Current temperature
float humid;      //Current humidity
float minTemp;    //The minimum temperature recorded
float maxTemp;    //The maximum temperature recorded
float minHumid;   //The minimum humidity recorded
float maxHumid;   //The maximum humidity recorded

unsigned long lastReadingUpdate;  // Time readings last updated

// Battery Monitor
#define MONITOR_PIN A0              // Pin used to monitor supply voltage
const float voltageDivider = 4.0;   // Used to calculate the actual voltage fRom the monitor pin reading
                                    // Using 1m and 330k ohm resistors dividS the voltage by approx 4
                                    // You may wany to substitute actual values of resistors in an equation (R1 + R2)/R2
                                    // E.g. (1000 + 330)/330 = 4.03
                                    // Alternatively take the voltage reading across the battery and from the joint between 
                                    // the 2 resistors to ground and divide one by the other to get the value.


// Variables for History stored in EPROM
// First version stored history in arrays but program would not run as may have 
//    run out of memory even though there appeared to be memory to spare
// 1 day's history is stored 96 readings at 15 minute intervals so each 
//   EPROM position is written once per day.
// EPROM has a limited life of approx 100,000 writes per postion so as we  
//   are writin to each postion once a day the EPROM should last 100,000 days (274 years!)
const int intSize = sizeof(int);      // Size of int to calculate storage location
const int tempStart = 0;              // Start postion of temperature history - length = 96 * intSize
const int humidStart = 100 * intSize; // Start postion of humidity
int histPointer = - 1;                  // Last last postion updated (-1 means no history yet stored)
unsigned long histUpdated;            // time last updated
unsigned long interval = 900000ul;   //(15 * 60 * 1000); 15 minutes in milliseconds


enum mode {
  dispTemp,        // Temperature currently displayed
  dispTempGraph,   // Temperature history graph displayed
  dispHumid,       // Humidity currently displayed
  dispHumidGraph   // Humidity history graph displayed
};

mode currentMode;   // Current display setting

bool displaying;                         // True if currently displaying data
unsigned long timeDisplay;               // Time started to display, or last button pressed
unsigned long keepDisplayFor = 15000;    // Number of milliseconds before enter screensave mode

unsigned long lastMoved;                 // Time last moved screensave bitmap
                                         // The battery indicator is displayed as the screensaver
unsigned long moveEvery = 10000;         // Number of milliseconds between moves

// Arrows used to indicate maximum and minimum
static const unsigned char PROGMEM upArrow[] {
  B00000000,
  B00011000,
  B00111100,
  B01111110,
  B11111111,
  B00011000,
  B00011000,
  B00011000
};

static const unsigned char PROGMEM downArrow[] {
  B00011000,
  B00011000,
  B00011000,
  B11111111,
  B01111110,
  B00111100,
  B00011000,
  B00000000
};

// Battery indicator bitmaps
static const unsigned char PROGMEM full[] {
  B00001110,B00000000,
  B00001110,B00000000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000
};

static const unsigned char PROGMEM three4[] {
  B00001110,B00000000,
  B00001110,B00000000,
  B11111111,B11100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000
};

static const unsigned char PROGMEM half[] {
  B00001110,B00000000,
  B00001110,B00000000,
  B11111111,B11100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000
};

static const unsigned char PROGMEM one4[] {
  B00001110,B00000000,
  B00001110,B00000000,
  B11111111,B11100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000,
  B11111111,B11100000
};

static const unsigned char PROGMEM empty[] {
  B00001110,B00000000,
  B00001110,B00000000,
  B11111111,B11100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B10000000,B00100000,
  B11111111,B11100000
};

void setup() {
  minTemp = 99;    //Set to a figure that is going to be too high
  maxTemp = -9;    //Set to a figure that is going to be too low
  minHumid = 99;   //Set to a figure that is going to be too high
  maxHumid = -9;   //Set to a figure that is going to be too low
  pinMode(MODE_BUTTON, INPUT_PULLUP);  // Use INPUT_PULLUP - will go LOW when button pressed
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  analogReference(INTERNAL);     // Sets the reference voltage for the analog pins to 1.1v
  pinMode(MONITOR_PIN, INPUT);   // Set input on pin used to monitor the voltage
  // Start sensor
  dht.begin();
  // Initialise display
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(WHITE);
  currentMode = dispTemp;
  displaying = true;
  timeDisplay = millis();   // Set so initially displays for set period
  histUpdated = millis();   // First update will be interval after startup
  lastMoved = millis();     // Time bitmap last moved
  clearEEPROM();            // Clear EPROM ready to start storing history
}

void loop() {
  // Check if the time since last display of data started is greater than
  // the time set to keeo the display showing
  if ((millis() - timeDisplay) > keepDisplayFor) {
    // If so in screen save mode
    displaying = false;               // Flag to indicate data not being displayed
    // Check if we need to move the bitmap
    if ((millis() - lastMoved) > moveEvery) {
      // Update time last moved
      lastMoved = millis();
      // Dislpay battery bitmap in random position
      DrawScreenSave(random(10, 110), random(10, 45));
    }
  }
  // Update the readings every 2 seconds
  if ((millis() - lastReadingUpdate) > 2000ul) {
    // Get readings, update minimum and maximum values
    lastReadingUpdate = millis();
    humid = dht.readHumidity();                   // read humidity
    temp = dht.readTemperature();                 // read temperature
    if (temp < minTemp)
      minTemp = temp;
    if (temp > maxTemp)
      maxTemp = temp;
    if (humid < minHumid)
      minHumid = humid;
    if (humid > maxHumid)
      maxHumid = humid;
    if (displaying) {
      // Update display if data being displayed
      switch (currentMode) {
        case dispTemp:
          ShowTemperature();
          break;
        case dispTempGraph:
          ShowGraph();
          break;
        case dispHumid:
          ShowHumidity();
          break;
        case dispHumidGraph:
          ShowGraph();
          break;
      }
    }
  }
  // Check if any buttons have been pressed
  CheckButtons();
  // If history update interval has passed, update history
  if ((millis() - histUpdated) >= interval)
    UpdateHistory();
  delay(150);           // Short pause before continuing
}

// Read the monitor pin and calculate the voltage
float BatteryVoltage(){
  float reading = analogRead(MONITOR_PIN);
  // Calculate voltage - reference voltage is 1.1v
  return 1.1 * (reading/1023) * voltageDivider;
}

void DrawScreenSave(int x, int y){
      // Get battery voltage and display approprate bitmap
      display.clearDisplay();
      float voltage = BatteryVoltage();
      if (voltage > 3.6)
        display.drawBitmap(x, y, full, 16, 16, WHITE);
      else
        if (voltage > 3.5)
          display.drawBitmap(x, y, three4, 16, 16, WHITE);
        else
          if (voltage > 3.4)
            display.drawBitmap(x, y, half, 16, 16, WHITE);
        else
          if (voltage > 3.3)
            display.drawBitmap(x, y, one4, 16, 16, WHITE);
          else
            display.drawBitmap(x, y, empty, 16, 16, WHITE);
      /* Debug code to display reading and voltage for checking
      display.setCursor(random(0,96), random(15,35));
      display.print(voltage);
      float reading = analogRead(MONITOR_PIN);
      display.setCursor(random(0,75), random(45,60));
      display.print(reading);*/
      display.display();
}

void UpdateHistory() {
  histUpdated += interval;
  // Use a circular list
  // Increment to next element in EPROM and circulate back to 0 if end
  // of list reached
  histPointer++;
  if (histPointer > 95)
    histPointer = 0;
  saveTemperature(histPointer, int(temp));
  saveHumidity(histPointer, int(humid));
}

// Clear area we are using for storage
void clearEEPROM()
{
  for (int i = 0 ; i < 200 * intSize ; i++) {
    if(EEPROM.read(i) != 0)                     //skip already "empty" addresses
    {
      EEPROM.write(i, 0);                       //write 0 to address i
    }
  }
}

void saveTemperature(int postion, int value){
  EEPROM.put(tempStart + postion * intSize, value);
}

void saveHumidity(int postion, int value){
  EEPROM.put(humidStart + postion * intSize, value);
}

int getTemperature(int postion){
  int value = 0;
  EEPROM.get(tempStart + postion * intSize, value);
  return value;
}

int getHumidity(int postion){
  int value = 0;
  EEPROM.get(humidStart + postion * intSize, value);
  return value;
}

// Routine to display numbers aligned and with sign
void dispNumber(int y, float value, bool tDisp) {
  if (value < 10.0 && value > 0.0)
    display.setCursor(72, y);
  else
    display.setCursor(60, y);
  display.print(value);
  if (tDisp)
    display.print("c");
  else
    display.print("%");
}

// Routine to display graph
void ShowGraph() {
  // Calculate scale
  int histValue;
  int minimum = 99;
  int maximum = -9;
  float yScale;
  for (int  i = 0; i < 96; i++) {
    if (currentMode == dispTempGraph)
      histValue = getTemperature(i);
    else
      histValue = getHumidity(i);
    if (histValue < minimum)
      minimum = histValue;
    if (histValue > maximum)
      maximum = histValue;
  }
  if ((maximum - minimum) != 0)
    yScale = 63.0 / (maximum - minimum);
  else
    yScale = 1;
  // Display axis data
  display.clearDisplay();
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print(minimum);
  if (currentMode == dispTempGraph)
    display.print("c");
  else
    display.print("%");
  display.setCursor(0, 0);
  display.print(maximum);
  if (currentMode == dispTempGraph)
    display.print("c");
  else
    display.print("%");
  display.setCursor(0, 25);
  if (currentMode == dispTempGraph)
    display.print("Temp.");
  else
    display.print("Humid.");
  // Graph list from histPointer + 1 circulating to histPointer
  int pos = histPointer + 1;
  int priorPos;
  int histValuePrior;
  // Count 1 less than list as stating at point 2
  for (int i = 1; i < 96; i++) {
    pos++;
    if (pos > 95)
      pos = 0;
    if (pos > 0)
      priorPos = pos - 1;
    else
      priorPos = 95;
    if (currentMode == dispTempGraph) {
      histValuePrior = getTemperature(priorPos);
      histValue = getTemperature(pos);
    }
    else {
      histValuePrior = getHumidity(priorPos);
      histValue = getHumidity(pos);
    }    
    display.drawLine(32 + i - 1, 63 - (histValuePrior - minimum) * yScale,
                     32 + i, 63 - (histValue - minimum) * yScale, SSD1306_WHITE);
  }
  display.display();
}

void ShowTemperature() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setFont(&FreeSans9pt7b);  // Font used for text
  display.setCursor(0, 15);
  display.print("Temp:");
  display.drawBitmap(20, 35 - 9, upArrow, 8, 8, WHITE);
  display.drawBitmap(20, 55 - 9, downArrow, 8, 8, WHITE);
  display.setFont(&FreeMonoBold9pt7b);  // Font used for numbers
  dispNumber(15, temp, true);
  dispNumber(35, maxTemp, true);
  dispNumber(55, minTemp, true);
  display.display();
}

void ShowHumidity() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setFont(&FreeSans9pt7b);  // Font used for text
  display.setCursor(0, 15);
  display.print("Humid:");
  display.drawBitmap(20, 35 - 9, upArrow, 8, 8, WHITE);
  display.drawBitmap(20, 55 - 9, downArrow, 8, 8, WHITE);
  display.setFont(&FreeMonoBold9pt7b);    // Font used for numbers
  dispNumber(15, humid, false);
  dispNumber(35, maxHumid, false);
  dispNumber(55, minHumid, false);
  display.display();
}

void CheckButtons() {
  // If the mode or reset button has been pressed pin will go LOW
  // Check if mode button pressed if so show data or switch mode between displays
  if (digitalRead(MODE_BUTTON) == LOW) {
    timeDisplay = millis();           // Reset so remains display for reset period after switching mode
    if (!displaying) {
      displaying = true;              // Set so displays data
      // Check level of battery
      float voltage = BatteryVoltage();
      if (voltage < 3.3)
        DisplayLowBattery();
      currentMode = dispTemp;
      ShowTemperature();
      delay(1000);
      return;
    }
    // If already displaying move to next display
    switch (currentMode) {
      case dispTemp:
        currentMode = dispTempGraph;
        ShowGraph();
        break;
      case dispTempGraph:
        currentMode = dispHumid;
        ShowHumidity();
        break;
      case dispHumid:
        currentMode = dispHumidGraph;
        ShowGraph();
        break;
      case dispHumidGraph:
        currentMode = dispTemp;
        ShowTemperature();
        break;
    }
    delay(1000);                     // Give time to release button
  }

  // Check if reset button pressed
  // Is the system displaying?
  // If so the reset min and max values
  if (displaying) {
    if (digitalRead(RESET_BUTTON) == LOW) {
      maxTemp = -9;
      minTemp = 99;
      maxHumid = -9;
      minHumid = 99;
      delay(1000);                     // Give time to release button
    }
  }
}
