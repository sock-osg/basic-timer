#define BUTTON1_PIN               2  // Button 1, setting mode
#define BUTTON2_PIN               3  // Button 2, decrease
#define BUTTON3_PIN               4  // Button 3, increase

#define DEFAULT_LONGPRESS_LEN    25  // Min nr of loops for a long press
#define DELAY                    20  // Delay per loop in ms

//////////////////////////////////////////////////////////////////////////////

enum { EV_NONE=0, EV_SHORTPRESS, EV_LONGPRESS };

//////////////////////////////////////////////////////////////////////////////
// Class definition
class ButtonHandler {

  public:
    // Constructor
    ButtonHandler(int pin, int longpress_len=DEFAULT_LONGPRESS_LEN);

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

ButtonHandler::ButtonHandler(int p, int lp)
  : pin(p), longpress_len(lp) {
}

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
    if (pressed_counter < longpress_len) {
      event = EV_SHORTPRESS;
    } else {
      event = EV_LONGPRESS;
    }
  } else {
    event = EV_NONE;
  }

  // update press running duration
  if (now_pressed) {
    ++pressed_counter;
  } else {
    pressed_counter = 0;
  }

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
DS3231  rtc(SDA, SCL);
TM1637Display display(CLK, DIO);
Time curr_time;

// Instantiate button objects
ButtonHandler btn_setup(BUTTON1_PIN, DEFAULT_LONGPRESS_LEN * 2);
ButtonHandler btn_decrease(BUTTON2_PIN);
ButtonHandler btn_increase(BUTTON3_PIN);

boolean setup_mode = false;
boolean setup_hours = false;
boolean setup_minutes = false;

byte bcd_map[] = {
  0b0000, // 0
  0b0001, // 1
  0b0010, // 2
  0b0011, // 3
  0b0100, // 4
  0b0101, // 5
  0b0110, // 6
  0b0111, // 7
  0b1000, // 8
  0b1001, // 9
  };

void setup() {
  Serial.begin(9600);

  rtc.begin();
  display.setBrightness(0x0a);

  // init buttons pins; I suppose it's best to do here
  btn_setup.init();
  btn_decrease.init();
  btn_increase.init();
}

void print_current_time() {
  curr_time = rtc.getTime();

  uint8_t data[] = {0x0, 0x0, 0x0, 0x0};
  
  if (setup_mode) {
    if (setup_hours) {
      data[0] = 0;
      data[1] = 0;
    } else if (setup_minutes) {
      data[2] = 0;
      data[3] = 0;
    }
  } else {
    
  }
}

void loop() {
  // handle button
  int event_set = btn_setup.handle();
  int event_dec = btn_decrease.handle();
  int event_inc = btn_increase.handle();

  if (event_set == EV_LONGPRESS && !setup_mode) {
    setup_mode = true;
    setup_hours = true;
  } else if (event_set == EV_SHORTPRESS && setup_mode) {
    setup_hours = !setup_hours;
    setup_minutes = !setup_minutes;
  } else if (setup_mode && event_set == EV_LONGPRESS) {
    // stop blinking
    setup_mode = false;
    setup_hours = false;
    setup_minutes = false;
  } else if (event_dec == EV_SHORTPRESS && setup_mode) {
    curr_time = rtc.getTime();
    
    if (setup_hours) {
      rtc.setTime(curr_time.hour - 1, curr_time.min, curr_time.sec);
    } else if (setup_minutes) {
      rtc.setTime(curr_time.hour, curr_time.min - 1, curr_time.sec);
    }
  } else if (event_inc == EV_SHORTPRESS && setup_mode) {
    curr_time = rtc.getTime();
    
    if (setup_hours) {
      rtc.setTime(curr_time.hour + 1, curr_time.min, curr_time.sec);
    } else if (setup_minutes) {
      rtc.setTime(curr_time.hour, curr_time.min + 1, curr_time.sec);
    }
  }

  print_current_time();

  delay(DELAY);
}

