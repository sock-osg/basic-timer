#define BTN_HOURS_PIN            4  // Button 1
#define BTN_MINUTES_PIN          5  // Button 2

#define DEFAULT_LONGPRESS_LEN    25  // Min nr of loops for a long press
#define DELAY                    20  // Delay per loop in ms

//////////////////////////////////////////////////////////////////////////////

enum { EV_NONE = 0, EV_SHORTPRESS, EV_LONGPRESS };

//////////////////////////////////////////////////////////////////////////////
// Class definition

class ButtonHandler {

  public:
    // Constructor
    ButtonHandler(int pin, int longpress_len = DEFAULT_LONGPRESS_LEN);

    // Initialization done after construction, to permit static instances
    void init();

    // Handler, to be called in the loop()
    int handle();

  protected:
    boolean was_pressed;     // previous state
    int pressed_counter;     // press running duration
    const int pin;           // pin to which button is connected
    const int longpress_len; // longpress duration
};

ButtonHandler::ButtonHandler(int p, int lp) : pin(p), longpress_len(lp) { }

void ButtonHandler::init() {
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH); // pull-up
  was_pressed = false;
  pressed_counter = 0;
}

int ButtonHandler::handle() {
  int event;
  int now_pressed = !digitalRead(pin);

  if (!now_pressed && was_pressed) {
    // handle release event
    event = pressed_counter < longpress_len ? EV_SHORTPRESS : EV_LONGPRESS;
  } else {
    event = EV_NONE;
  }

  // update press running duration
  pressed_counter = now_pressed ? pressed_counter + 1 : 0;

  // remember state, and we're done
  was_pressed = now_pressed;
  return event;
}

//////////////////////////////////////////////////////////////////////////////

#include <DS3231.h>
#include <Wire.h>
#include <TM1637Display.h>

#define CLK              3 //can be any digital pin
#define DIO              2 //can be any digital pin

#define _24_HRS_MODE_PIN 6 //can be any digital pin
#define _24_HRS_LED_PIN 7 //can be any digital pin

// Init the DS3231 using the hardware interface
TM1637Display display(CLK, DIO);
DS3231 rtc;

// Instantiate button objects
ButtonHandler btn_hours(BTN_HOURS_PIN);
ButtonHandler btn_minutes(BTN_MINUTES_PIN);

bool h12 = false;
bool PM;
bool _24HrsMode = false;

void setup() {
  Serial.begin(9600);

  // Initialize the rtc object
  Wire.begin();
  display.setBrightness(0x0a);

  btn_hours.init();
  btn_minutes.init();

  pinMode(_24_HRS_MODE_PIN, INPUT);
  pinMode(_24_HRS_LED_PIN, OUTPUT);
}

byte getHour() {
  byte hour = rtc.getHour(h12, PM);

  if (_24HrsMode) {
    if (hour > 12) {
      digitalWrite(_24_HRS_LED_PIN, HIGH);
    } else {
      digitalWrite(_24_HRS_LED_PIN, LOW);
    }

    if (hour == 0) {
      hour = 12;
    } else if (hour >= 13) {
      hour = hour - 12;
    }
  } else {
    digitalWrite(_24_HRS_LED_PIN, LOW);
  }

  return hour;
}

void print_current_time() {
  byte current_hour = getHour();
  byte current_minute = rtc.getMinute();
  
  byte tens_curr_hour = current_hour / 10;
  byte units_curr_hour = current_hour % 10;

  uint8_t data[] = {
    tens_curr_hour == 0 ? 0x0 : display.encodeDigit(tens_curr_hour),
    rtc.getSecond() % 2 == 0 ? 0x80 | display.encodeDigit(units_curr_hour) : display.encodeDigit(units_curr_hour),
    display.encodeDigit(current_minute / 10),
    display.encodeDigit(current_minute % 10)
  };

  display.setSegments(data);
}

void print_temperature() {
  byte temperature = rtc.getTemperature();
  uint8_t data[] = {
    display.encodeDigit(temperature / 10),
    display.encodeDigit(temperature % 10),
    0x63, // Degrees symbol
    0x39
  };
  display.setSegments(data);
}

void loop() {
  _24HrsMode = digitalRead(_24_HRS_MODE_PIN);

  // handle button
  int event_btn_hours = btn_hours.handle();
  int event_btn_minutes = btn_minutes.handle();

  if (event_btn_hours == EV_SHORTPRESS) {
    int current_hour = rtc.getHour(h12, PM);
    int new_hour = current_hour < 23 ? current_hour + 1 : 0;
    rtc.setHour(new_hour);
  } else if (event_btn_minutes == EV_SHORTPRESS) {
    int current_minute = rtc.getMinute();
    int new_minute = current_minute < 59 ? current_minute + 1 : 0;
    rtc.setMinute(new_minute);
  }

  if (rtc.getSecond() < 57) {
    print_current_time();
  } else {
    print_temperature();
  }

  delay(DELAY);
}

