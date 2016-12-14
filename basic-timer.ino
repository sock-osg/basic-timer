#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire
#include <TM1637Display.h>

#define CLK 4
#define DIO 5
#define PIN_RELE 2

#define INIT_HOUR 19
#define END_HOUR 7

TM1637Display display(CLK, DIO);
uint8_t data[] = { 0x00, 0x38, 0x79, 0x3f };

void setup(void) {
  Serial.begin(115200);
  
  display.setBrightness(0x0f);
  display.setSegments(data);
  delay(5000);
  
  //setSyncProvider() causes the Time library to synchronize with the
  //external RTC by calling RTC.get() every five minutes by default.
  setSyncProvider(RTC.get);
  Serial << F("RTC Sync");

  if (timeStatus() != timeSet) {
    Serial << F(" FAIL!");
  } else {
    // minutes, hours, dayOrDate);
    RTC.setAlarm(ALM2_MATCH_HOURS, 0, 19, 0);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_2, true);

    // seconds, minutes, hours, dayOrDate);
    RTC.setAlarm(ALM1_MATCH_HOURS, 0, 0, 7, 0);
    RTC.alarm(ALARM_1);
    RTC.alarmInterrupt(ALARM_1, true);

    int currentHour = hour(now());
    Serial << "current hour: " << currentHour << endl;

    pinMode(PIN_RELE, OUTPUT);
    if (currentHour > END_HOUR && currentHour < INIT_HOUR) {
      Serial << F("Setting OFF the rele") << endl;
      digitalWrite(PIN_RELE, HIGH);
    }
  }

  Serial << endl;
}

void loop(void) {
  static time_t tLast;
  time_t t;
  tmElements_t tm;

  //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  if (Serial.available() >= 12) {
    //note that the tmElements_t Year member is an offset from 1970,
    //but the RTC wants the last two digits of the calendar year.
    //use the convenience macros from Time.h to do the conversions.
    int y = Serial.parseInt();
    if (y >= 100 && y < 1000) {
      Serial << F("Error: Year must be two digits or four digits!") << endl;
    } else {
      if (y >= 1000) {
        tm.Year = CalendarYrToTm(y);
      } else { //(y < 100)
        tm.Year = y2kYearToTm(y);
      }
      tm.Month = Serial.parseInt();
      tm.Day = Serial.parseInt();
      tm.Hour = Serial.parseInt();
      tm.Minute = Serial.parseInt();
      tm.Second = Serial.parseInt();
      t = makeTime(tm);
      RTC.set(t);        //use the time_t value to ensure correct weekday is set
      setTime(t);        
      Serial << F("RTC set to: ");
      printDateTime(t);
      Serial << endl;

      //dump any extraneous input
      while (Serial.available() > 0) Serial.read();
    }
  } else {
    if (RTC.alarm(ALARM_2)) {
      Serial.println("Matches 19 Hours");
      // Turn on
      digitalWrite(PIN_RELE, LOW);
    }
    if (RTC.alarm(ALARM_1)) {
      Serial.println("Matches 07 Hours");
      // Turn off
      digitalWrite(PIN_RELE, HIGH);
    }
  }
  
  t = now();
  if (t != tLast) {
    tLast = t;
    printDateTime(t);
    if (second(t) == 0) {
      float c = RTC.temperature() / 4.;
      float f = c * 9. / 5. + 32.;
      Serial << F("  ") << c << F(" C  ") << f << F(" F");
    }
    Serial << endl;
  }
}

//print date and time to Serial
void printDateTime(time_t t) {
  printDate(t);
  Serial << ' ';
  printTime(t);
}

//print time to Serial
void printTime(time_t t) {
  printI00(hour(t), ':');
  printI00(minute(t), ':');
  printI00(second(t), ' ');

  printToDisplay(hour(t), minute(t));
}

void printToDisplay(int hours, int minutes) {
  if (hours < 10) {
    data[0] = 0x00;
    data[1] = display.encodeDigit(hours);
  } else {
    data[0] = display.encodeDigit(hours / 10);
    data[1] = display.encodeDigit(hours % 10);
  }

  if (minutes < 10) {
    data[2] = 0x3f;
    data[3] = display.encodeDigit(minutes);
  } else {
    data[2] = display.encodeDigit(minutes / 10);
    data[3] = display.encodeDigit(minutes % 10);
  }

  display.setSegments(data);
}

//print date to Serial
void printDate(time_t t) {
  printI00(day(t), 0);
  Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void printI00(int val, char delim) {
  if (val < 10) {
    Serial << '0';
  }
  Serial << _DEC(val);
  if (delim > 0) {
    Serial << delim;
  }
  return;
}

