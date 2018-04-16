#define BTN_HOURS_PIN            2  // Button 1
#define BTN_MINUTES_PIN          3  // Button 2

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
#include <TM1637Display.h>

#define CLK 9 //can be any digital pin
#define DIO 8 //can be any digital pin

// Init the DS3231 using the hardware interface
TM1637Display display(CLK, DIO);
DS3231 rtc;

// Instantiate button objects
ButtonHandler btn_hours(BTN_HOURS_PIN);
ButtonHandler btn_minutes(BTN_MINUTES_PIN);
DateTime curr_time;
RTClib RTC;

void setup() {
  Serial.begin(9600);

  // Initialize the rtc object
  Wire.begin();
  display.setBrightness(0x0a);

  btn_hours.init();
  btn_minutes.init();
}

void print_current_time() {
  curr_time = RTC.now();
  int current_hour = curr_time.hour();
  int current_minute = curr_time.minute();

  uint8_t data[4] = {
    current_hour / 10,
    current_hour % 10,
    current_minute / 10,
    current_minute % 10
  };

  display.setSegments(data);

  uint8_t segto = 0x0;;
  if (curr_time.second() % 2 == 0) {
    int value = 1244;
    segto = 0x80 | display.encodeDigit((value / 100) % 10);
  }
  display.setSegments(&segto, 1, 1);
}

void print_temperature() {
  int temperature = (int) rtc.getTemperature();
  uint8_t data[] = {
    temperature / 10,
    temperature % 10,
    0x63, // Degrees symbol
    0x0
  };
  display.setSegments(data);
}

void loop() {
  rtc.setClockMode(false);  // set to 24h
  
  // handle button
  int event_btn_hours = btn_hours.handle();
  int event_btn_minutes= btn_minutes.handle();

  if (event_btn_hours == EV_LONGPRESS) {
    int current_hour = curr_time.hour();
    int new_hour = current_hour < 24 ? current_hour + 1 : 0;
    rtc.setHour(new_hour);
  } else if (event_btn_minutes == EV_LONGPRESS) {
    int current_minute = curr_time.minute();
    int new_minute = current_minute < 60 ? current_minute + 1 : 0;
    rtc.setMinute(new_minute);
  }

  print_current_time();

  if (curr_time.second() >= 57) {
    print_temperature();
  }

  delay(DELAY);
}

