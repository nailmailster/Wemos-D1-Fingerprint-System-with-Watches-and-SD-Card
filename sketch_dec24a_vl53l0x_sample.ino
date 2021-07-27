#define _MicroSD
//#define _VL53L0X
#define _LCD
#define _DS1302
#define _FPM10A

#ifdef _MicroSD
  #include <SPI.h>
  #include "SdFat.h"

  const uint8_t SOFT_MISO_PIN = D12;
  const uint8_t SOFT_MOSI_PIN = D11;
  const uint8_t SOFT_SCK_PIN  = D13;

  const uint8_t SD_CHIP_SELECT_PIN = D2;
  
  SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> sd;
  SdFile file;
#endif

#ifdef _VL53L0X
  #include "Adafruit_VL53L0X.h"
//  #include <Wire.h>   //  уже включен в Adafruit_VL53L0X.h
#endif

#ifdef _LCD
  #include <LiquidCrystal_I2C.h>
#endif

#ifdef _DS1302
  #include <ErriezDS1302.h>
  #include <NTPtimeESP.h>
#else
  #include <ESP8266WiFi.h>    //  видимо, уже включен в <NTPtimeESP.h>
#endif

#include <WiFiClient.h>

#ifdef _VL53L0X
  Adafruit_VL53L0X lox = Adafruit_VL53L0X();
//  Adafruit_VL53L0X lox;
#endif

#ifdef _LCD
//  LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  //  NEGATIVE
  LiquidCrystal_I2C *lcd;
#endif

#ifdef _DS1302
  #define DS1302_CLK_PIN      D9  //D6  //D4  белый
  #define DS1302_IO_PIN       D10 //D7  //D3  желтый
  #define DS1302_CE_PIN       D8  //D2  синий
  DS1302 rtc = DS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);
  DS1302_DateTime dt;
#endif

#ifdef _FPM10A
  #include <Adafruit_Fingerprint.h>

  #include <SoftwareSerial.h>
  SoftwareSerial *mySerial;
  Adafruit_Fingerprint *finger;
#endif

#ifdef _DS1302
  NTPtime NTPch("ch.pool.ntp.org");   // Choose server pool as required
  bool NTPTimeIsSet = false;
#endif

bool outOfRange = true;
int counter = 0;

char ssid[] = "DIR-615";         // your network SSID (name)
char pass[] = "76543210";        // your network password
char server[] = "parfum.nailmail.h1n.ru";
int status = WL_IDLE_STATUS;     // the Wifi radio's status
WiFiClient client;

unsigned long lastTimeMillis = 0;

bool microSDIsOK = false, FPM10AIsOK = false;

#ifdef _DS1302
  strDateTime dateTime;
#endif
char buf[32];
char timeBuf[9];
    
void setup() {
  Serial.begin(9600);
  while (! Serial) {
    delay(1);
  }

//  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);

  #ifdef _VL53L0X
    Serial.println("Adafruit VL53L0X test");
    if (!lox.begin(0x29)) {
      Serial.println(F("Failed to boot VL53L0X"));
//      while(1);
    }
    Serial.println(F("VL53L0X API Simple Ranging example\n\n"));
  #endif

  #ifdef _LCD
    lcd = new LiquidCrystal_I2C(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  //  NEGATIVE
    lcd->begin(16, 2);
    lcd->clear(); Serial.println(F("lcd->clear()"));
    lcd->print(F("Hello, world!"));
    lcd->setCursor(0, 1);
    lcd->print(F("Counter = "));
    lcd->print(counter);
  #endif

  #ifdef _DS1302
    rtc.begin();
    rtc.halt(false);
    /*dt.second = 0;
    dt.minute = 53;
    dt.hour = 0;
    dt.dayWeek = 4; // 1 = Monday
    dt.dayMonth = 27;
    dt.month = 12;
    dt.year = 2018;
    rtc.setDateTime(&dt);
    // Check write protect state
    if (rtc.isWriteProtected()) {
        Serial.println(F("Error: DS1302 write protected"));
        while (1);
    }
    // Check write protect state
    if (rtc.isHalted()) {
        Serial.println(F("Error: DS1302 halted"));
        while (1);
    }*/
  #endif

  #ifdef _MicroSD
    if (sd.begin(SD_CHIP_SELECT_PIN)) {
      microSDIsOK = true;
      Serial.println(F("SD ok"));
    }
    else {
      microSDIsOK = false;
      Serial.println(F("SD error"));
    }
  #endif

  status = WiFi.begin(ssid, pass);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("IP address: " + WiFi.localIP());
  printWifiStatus();

  #ifdef _FPM10A
    mySerial = new SoftwareSerial(D4, D5);

    finger = new Adafruit_Fingerprint(mySerial);
    finger->begin(57600);

    if (finger->verifyPassword()) {
      FPM10AIsOK = true;
      Serial.println(F("Found fingerprint sensor!"));
    } else {
      FPM10AIsOK = false;
      Serial.println(F("Did not find fingerprint sensor :("));
//      while (1) { delay(1); }
    }
  
    if (FPM10AIsOK) {
      finger->getTemplateCount();
      Serial.print(F("Sensor contains ")); Serial.print(finger->templateCount); Serial.println(F(" templates"));
      Serial.println(F("Waiting for valid finger..."));
    }
  #endif

  #ifdef _LCD
    lcd->clear(); Serial.println(F("lcd->clear()"));
    if (microSDIsOK)
      lcd->print(F("SD ok "));
    else
      lcd->print(F("SD er "));
    if (FPM10AIsOK)
      lcd->print(F("FPM ok"));
    else
      lcd->print(F("FPM er"));
    lcd->setCursor(0, 1);
    lcd->print(F("Counter = "));
    lcd->print(counter);
  #endif
  
//  wdtEnable();
}

void loop() {
  #ifdef _VL53L0X
    VL53L0X_RangingMeasurementData_t measure;
  #endif

  #ifdef _DS1302
    if (!NTPTimeIsSet) {
      dateTime = NTPch.getNTPtime(3.0, 0);
      if (dateTime.valid) {
        NTPch.printDateTime(dateTime);
  
        dt.second = dateTime.second;
        dt.minute = dateTime.minute;
        dt.hour = dateTime.hour;
        dt.dayWeek = dateTime.dayofWeek; // 1 = Sunday
        dt.dayMonth = dateTime.day;
        dt.month = dateTime.month;
        dt.year = dateTime.year;
        rtc.setDateTime(&dt);
        // Check write protect state
        if (rtc.isWriteProtected()) {
            Serial.println(F("Error: DS1302 write protected"));
            while (1);
        }
        // Check write protect state
        if (rtc.isHalted()) {
            Serial.println(F("Error: DS1302 halted"));
            while (1);
        }
        NTPTimeIsSet = true;
        Serial.println(F("NTPTimeIsSet = true"));
      }
    }
  #endif

  #ifdef _VL53L0X
    do
      lox.rangingTest(&measure, false);
    while (measure.RangeMilliMeter == 0);
  
    if (measure.RangeStatus != 4) {
      Serial.print(F("Distance (mm): "));
      Serial.println(measure.RangeMilliMeter);
      if (outOfRange) {
        outOfRange = false;
        #ifdef _LCD
          lcd->clear(); Serial.println(F("lcd->clear()"));
          lcd->print(F("D="));
          lcd->setCursor(0, 1);
          lcd->print(F("C="));
          lcd->print(counter);
        #endif
      }
      #ifdef _LCD
        lcd->setCursor(3, 0);
        lcd->print(measure.RangeMilliMeter);
      #endif
      //delay(50);
    }
    else {
      if (!outOfRange) {
        outOfRange = true;
        #ifdef _LCD
          lcd->clear(); Serial.println(F("lcd->clear()"));
          lcd->print(F("D="));
          lcd->print(F("MAX+"));
        #endif
  
        counter++;
        Serial.println("Counter = " + String(counter));

        #ifdef _LCD
          lcd->setCursor(0, 1);
          lcd->print(F("C="));
          lcd->print(counter);
        #endif

        #ifdef _DS1302
          getCurrentDateTime();
          if (!rtc.getDateTime(&dt)) {
              Serial.println(F("Error: DS1302 read failed"));
              #ifdef _LCD
                lcd->setCursor(17 - sizeof(timeBuf), 0);
                lcd->print(F("CLOCK ERR"));
              #endif
          }
          else {
            snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
                    dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
            Serial.println(buf);
            snprintf(timeBuf, sizeof(timeBuf), "%d:%02d:%02d",
                    dt.hour, dt.minute, dt.second);
            Serial.println(timeBuf);
            #ifdef _LCD
              if (dt.hour > 9)
                lcd->setCursor(17 - sizeof(timeBuf), 0);
              else
                lcd->setCursor(18 - sizeof(timeBuf), 0);
              lcd->print(timeBuf);
            #endif
            #ifdef _MicroSD
              Serial.println(F("Creating counter.txt..."));
              if (!file.open("counter.txt", O_RDWR | O_CREAT | O_APPEND)) {
                sd.errorHalt(F("open failed"));
                #ifdef _LCD
                  lcd->setCursor(7, 0);
                  lcd->print(F("F"));
                #endif
              }
              else {
                #ifdef _LCD
                  lcd->setCursor(7, 0);
                  lcd->print(F("T"));
                #endif
              }
              file.println(timeBuf);
              file.println(counter);
              file.close();
            #endif
          }
        #endif
      }
    }
  #endif

  #ifdef _FPM10A
    if (FPM10AIsOK)
      if (getFingerprintIDez() != -1) {
        #ifdef _MicroSD
          if (getCurrentDateTime()) {
            Serial.println("Creating fingers.txt...");
            if (!file.open("fingers.txt", O_RDWR | O_CREAT | O_APPEND)) {
              sd.errorHalt(F("open failed"));
              #ifdef _LCD
                lcd->clear();
                lcd->setCursor(7, 0);
                lcd->print(F("F"));
                if (dt.hour > 9)
                  lcd->setCursor(17 - sizeof(timeBuf), 0);
                else
                  lcd->setCursor(18 - sizeof(timeBuf), 0);
                lcd->print(timeBuf);
                lcd->setCursor(0, 1);
                lcd->print(F("Found ID #")); lcd->print(finger->fingerID);
              #endif
            }
            else {
              #ifdef _LCD
                lcd->clear();
                lcd->setCursor(7, 0);
                lcd->print(F("T"));
                if (dt.hour > 9)
                  lcd->setCursor(17 - sizeof(timeBuf), 0);
                else
                  lcd->setCursor(18 - sizeof(timeBuf), 0);
                lcd->print(timeBuf);
                lcd->setCursor(0, 1);
                lcd->print(F("Found ID #")); lcd->print(finger->fingerID);
              #endif
              file.println(timeBuf);
              file.println(finger->fingerID);
              file.close();
            }
          } else {
            Serial.println(F("Couldn't read time"));
          }
        #endif
      }
    delay(50);
  #endif

  #ifdef _VL53L0X
    if (millis() - lastTimeMillis > 60000 * 1) {
      httpRequest();
      lastTimeMillis = millis();
    }
  #endif
}

void printWifiStatus()
{
  Serial.print(F("SSID: "));                 // print the SSID of the network you're attached to
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();          // print your WiFi shield's IP address
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  long rssi = WiFi.RSSI();                // print the received signal strength
  Serial.print(F("Signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

void httpRequest()                        // this method makes a HTTP connection to the server
{
  Serial.println();
    
  client.stop();                          // close any connection before send a new request // this will free the socket on the WiFi shield

  if (client.connect(server, 80)) {       // if there's a successful connection
    Serial.println(F("Connecting..."));
    
    // send the HTTP PUT request
//    client.println("GET /api.php?id=61&temperature=" + String(temperature, DEC) + "&humidity=" + String(humidity, DEC) + "&esp_restarts=0" + " HTTP/1.1");
    client.println("GET /api.php?id=51&dt=20181126184900&placeid=2&ins=" + String(counter, DEC) + "&outs=0 HTTP/1.1");
    client.println(F("Host: parfum.nailmail.h1n.ru"));
    client.println(F("Connection: close"));
    client.println();

    #ifdef _MicroSD
//      Serial.println("Creating wi-fi.txt...");
//      File myFile = SD.open("wi-fi.txt", FILE_WRITE);
//      if (myFile) {
//        myFile.println("Connecting...");
//        myFile.close();
//        #ifdef _LCD
//          lcd->setCursor(7, 0);
//          lcd->print("T");
//        #endif
//      }
//      else {
//        Serial.println("Error opening wi-fi.txt");
//        #ifdef _LCD
//          lcd->setCursor(7, 0);
//          lcd->print("F");
//        #endif
//      }
//    
//      // Check to see if the file exists:
//      if (SD.exists("wi-fi.txt")) {
//        Serial.println("wi-fi.txt exists.");
//      } else {
//        Serial.println("wi-fi.txt doesn't exist.");
//      }
    #endif
  }
  else {
    Serial.println(F("Connection failed"));  // if you couldn't make a connection
  }
}

uint8_t getFingerprintID() {
  #ifdef _FPM10A
    uint8_t p = finger->getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println(F("Image taken"));
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(F("No finger detected"));
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println(F("Communication error"));
        return p;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println(F("Imaging error"));
        return p;
      default:
        Serial.println(F("Unknown error"));
        return p;
    }
  
    // OK success!
  
    p = finger->image2Tz();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println(F("Image converted"));
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println(F("Image too messy"));
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println(F("Communication error"));
        return p;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println(F("Could not find fingerprint features"));
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println(F("Could not find fingerprint features"));
        return p;
      default:
        Serial.println(F("Unknown error"));
        return p;
    }
    
    // OK converted!
    p = finger->fingerFastSearch();
    if (p == FINGERPRINT_OK) {
      Serial.println(F("Found a print match!"));
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println(F("Communication error"));
      return p;
    } else if (p == FINGERPRINT_NOTFOUND) {
      Serial.println(F("Did not find a match"));
      return p;
    } else {
      Serial.println(F("Unknown error"));
      return p;
    }   
    
    // found a match!
    Serial.print(F("Found ID #")); Serial.print(finger->fingerID);
    Serial.print(F(" with confidence of ")); Serial.println(finger->confidence); 
  
    return finger->fingerID;
  #endif
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  #ifdef _FPM10A
    uint8_t p = finger->getImage();
    if (p != FINGERPRINT_OK)  return -1;
  
    p = finger->image2Tz();
    if (p != FINGERPRINT_OK)  return -1;
  
    p = finger->fingerFastSearch();
    if (p != FINGERPRINT_OK)  return -1;
    
    // found a match!
    Serial.print(F("Found ID #")); Serial.print(finger->fingerID);
    Serial.print(F(" with confidence of ")); Serial.println(finger->confidence);
    return finger->fingerID;
  #endif
}

bool getCurrentDateTime() {
  #ifdef _DS1302
    if (!rtc.getDateTime(&dt)) {
        Serial.println(F("Error: DS1302 read failed"));
        #ifdef _LCD
          lcd->setCursor(17 - sizeof(timeBuf), 0);
          lcd->print(F("CLOCK ERR"));
        #endif
        return false;
    }
    else {
      snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
              dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
      Serial.println(buf);
      snprintf(timeBuf, sizeof(timeBuf), "%d:%02d:%02d",
              dt.hour, dt.minute, dt.second);
      Serial.println(timeBuf);
      #ifdef _LCD
        if (dt.hour > 9)
          lcd->setCursor(17 - sizeof(timeBuf), 0);
        else
          lcd->setCursor(18 - sizeof(timeBuf), 0);
        lcd->print(timeBuf);
      #endif
      return true;
    }
  #endif
  return false;
}

