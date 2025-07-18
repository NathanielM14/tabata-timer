#include "SevSeg.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define NUM_PROFILES 5

struct Profile {
  uint8_t skill;
  int work;
  int rest;
  int sets;
  int storeWork;
  int storeRest;
};

Profile profiles[NUM_PROFILES] = { 
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
}; // 5 profiles

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

enum TimerState { IDLE, WORK, REST, DONE, START, PAUSE, DIFF };
TimerState currentState = IDLE;
TimerState previousState = DIFF;

enum DisplayState { MENU, INFO, OPTIONS, TIMER};
DisplayState displayState = MENU;

SevSeg sevseg;
bool value = false, prmoptShown = false, inputSets = false, inputWork = false, startTimer = false, pauseTimer = false, workTimer = false, restTimer = false;


// button variables
const uint8_t button4 = A3;
const uint8_t button3 = A2;
const uint8_t button2 = A1;
const uint8_t button1 = 2;
const uint8_t buzzer = A0;

uint8_t button4_state, prev_button4 = HIGH; 
uint8_t button3_state, prev_button3 = HIGH; 
uint8_t button2_state, prev_button2 = HIGH; 
uint8_t button1_state, prev_button1 = HIGH; 
uint8_t reading;

// timer variables
uint8_t trackSets = 0;
unsigned int count = 0;
unsigned int storeCount = 0;
unsigned long previous_micro = 0;  // stores last time updated
const long interval = 1000000; // interval at which to blink in ms
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long resumeTime;

int startTime = 6;
int displayVal;

// selection variables
uint8_t selected = 0;
int entered = -1;

// profile variables
uint8_t currProfile = 10;

// add buzzer funcitonallity for when time is about to be up

void setup() {
  // put your setup code here, to run once:
  byte displayNum = 4;                           // num of seven segment displays
  byte digitPins[] = { 10, 13, 12, 11 };         // display pins 1, 2, 3, 4
  byte segmentPins[] = { 7, 9, 5, 4, 3, 8, 6 };  // segment pins A, B, C, D, E, F, G
  bool resistorsOnSegments = true;               // 330 ohm resistors on
  byte hardwareConfig = COMMON_CATHODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;

  sevseg.begin(hardwareConfig, displayNum, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros);
  sevseg.setBrightness(90);
  Serial.begin(9600);

  pinMode(button4, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button1, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  Serial.print(sizeof(Profile));
  Serial.println("\n");
  for (int i = 0; i < 5; i++) {
    EEPROM.get(i * sizeof(profiles[i]), profiles[i]);
    Serial.print(F("LOADED PROFILE: "));
    Serial.println(i + 1);
  }
}

void loop() {
  switch (displayState) {
    case MENU:
      display.clearDisplay();
      displayMenu();
      display.display();

      for (int i = 0; i < 5; i++) {
        if (entered == i) {
          if (profiles[currProfile].skill == 0) {
            displayState = INFO; // if no skill yet go to info
          }
          else {
            displayState = OPTIONS; // if there is a skill give options
          }
          selected = 0;
          entered = -1;
        }
      }
      break;
    case INFO:
      // needs to go to the current profile selected from menu
      display.clearDisplay();
      displayInfo(currProfile);
      display.display();

      if (entered == 4) {
        Serial.print("SAVE WORK: ");
        Serial.println(profiles[currProfile].work);
        saveProfile(profiles[currProfile], currProfile); // saves the profile to EEPROM memory
        // save profile to start at addr 
        displayState = MENU; // go to menu after saved
        entered = -1;
        selected = 0;
        currProfile = 10;
      }
      break;
    case OPTIONS:
      display.clearDisplay();
      displayOptions();
      display.display();
  
      if (entered == 0) {
        profiles[currProfile].storeWork = profiles[currProfile].work;
        profiles[currProfile].storeRest = profiles[currProfile].rest;
        displayState = TIMER; // go to timer if 1
        entered = -1;
      }
      else if (entered == 1) {
        displayState = INFO; // go to info if 2
        selected = 0;
        entered = -1;
      }
      else if (entered == 2) {
        // delete profile
        profiles[currProfile].skill = 0;
        profiles[currProfile].rest = 0;
        profiles[currProfile].work = 0;
        profiles[currProfile].sets = 0;
        profiles[currProfile].storeRest = 0;
        profiles[currProfile].storeWork = 0;

        saveProfile(profiles[currProfile], currProfile); // "delete" any values stored 

        displayState = MENU;
        selected = 0;
        entered = -1;
        // will need to have method to delete EEPROM
      }
      break;
    case TIMER:
      // refresh the display value only if the previous state and current state are different
      if (currentState != previousState) {
        previousState = currentState; // update previousState to current state
        display.clearDisplay();

        // will only show once exercise info is done
        displayTimer(currentState, currProfile);
        display.display(); 
      }
      break;
  }

  // all 4 buttons
  firstButton();
  secondButton();
  thirdButton();
  fourthButton();
  
  // FSM for the timer state
  if (displayState == TIMER) {
    switch (currentState) {
    case IDLE:
      displayVal = 0;
      if (startTimer) {
        currentState = START;
      }
      break;
    case START:
      if (startTime) {
        displayVal = counter(startTime);
      }
      else if (startTime == 0) {
        currentState = WORK;
      }
      break;
    case WORK:
      if (profiles[currProfile].work > 0) {
        workTimer = true;
        displayVal = counter(profiles[currProfile].work);
      } 
      else  {
        workTimer = false;
        currentState = REST;
        profiles[currProfile].work = profiles[currProfile].storeWork;
      }
      break;
    case REST:
      if (profiles[currProfile].rest > 0) {
        restTimer = true;
        displayVal = counter(profiles[currProfile].rest);
      } 
      else if (trackSets < profiles[currProfile].sets - 1) {
        currentState = WORK;
        restTimer = false;
        profiles[currProfile].rest = profiles[currProfile].storeRest;
        trackSets++;
        Serial.print("SET: ");
        Serial.println(trackSets);
      }
      else {
        restTimer = false;
        Serial.print("SET: ");
        Serial.println(profiles[currProfile].sets);
        profiles[currProfile].rest = profiles[currProfile].storeRest;
        currentState = DONE;
        noTone(buzzer);
      }
      break;
    }

    // only display sevseg display if currentState is active (not IDLE)
    if (currentState != IDLE) {
      sevseg.setNumber(displayVal);
      sevseg.refreshDisplay();
    }
    else {
      sevseg.blank();
    }
  }
}

// decrements the count every second
int counter(int &count) {
  if (count > 0) {
    noTone(buzzer);
    unsigned long current_micro = micros();
    // if the difference in time is greater than or equal to the interval time it will update the count (indicating one second);
    if (current_micro - previous_micro >= interval) {
      previous_micro = current_micro;  // saves the last time it was updated

      Serial.println(count - 1);
      if (count < 5) {
        tone(buzzer, 3000);
      }
      count--;
    }
  }
  return count;
}

void firstButton() {
  // up for MENU
  // up for OPTIONS
  // up for INFO
    // increment values
  // start for TIMER

  reading = digitalRead(button1);

  if (reading != prev_button1) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != button1_state) {
      button1_state = reading;
      if (button1_state == LOW) {
        // for options
        if (displayState == OPTIONS) {
          if (selected > 0) {
            selected -= 1;
          }
        }

        // for menu
        if (displayState == MENU) {
          if (selected > 0) {
            selected -= 1;
          }
        }

        // for info
        if (displayState == INFO) {
          if (entered == -1) {
            if (selected > 0) {
              selected -= 1;
            }
          }
          else if (entered == 0) {
            if (profiles[currProfile].skill <= 6) {
              profiles[currProfile].skill += 1;
              Serial.println(profiles[currProfile].skill);
            }
          }
          else if (entered == 1) {
            profiles[currProfile].work += 1;
            profiles[currProfile].storeWork = profiles[currProfile].work;
            Serial.print("WORK IS: ");
            Serial.println(profiles[currProfile].work);
          }
          else if (entered == 2) {
            profiles[currProfile].rest += 20;
            profiles[currProfile].storeRest = profiles[currProfile].rest;
            Serial.println(F("ADD 20 REST"));
          }
          else if (entered == 3) {
            profiles[currProfile].sets += 1;
            Serial.println(F("ADD 1 SET"));
          }
        }

        // for timer
        if (displayState == TIMER) {
          // if pauseTimer and startTimer are both false then start
          if (!pauseTimer && !startTimer) {
            Serial.println(F("START"));
            count += 1;
            startTimer = true;
            currentState = START;
          }
          // if pauseTimer is false and startTimer is true then pause
          else if (!pauseTimer && startTimer && currentState != START && currentState != DONE) {
            Serial.println(F("PAUSE"));
            pauseTimer = true;
            startTimer = false;
            currentState = PAUSE;
          }
          // if pauseTimer is true and startTimer is false then resume
          else if (pauseTimer && !startTimer) {
            Serial.println(F("RESUME"));
            count += 1;
            pauseTimer = false;
            startTimer = true;
            // dependent on what state it was previously at
            if (restTimer) {
              currentState = REST;
            }
            else if (workTimer) {
              currentState = WORK;
            }
          }
        }
      }
    }
  }
  prev_button1 = reading;
}

void secondButton() {
  // down for MENU
  // down for INFO
    // decrements value
  // down for OPTIONS
  // reset for TIMER
  reading = digitalRead(button2);

  // case for if switch changed due to noise or pressing
  if (reading != prev_button2) {
    lastDebounceTime = millis();
  }

  // if the time between current and last debounce is greater than delay its a press
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != button2_state) {
      button2_state = reading;  // update the button state
      if (button2_state == LOW) {
        // case for when in TIMER go to MENU
        if (displayState == OPTIONS) {
          if (selected < 2) {
            selected += 1;
          }
        }

        // menu state
        if (displayState == MENU) {
          if (selected < 4) {
            selected += 1;
          }
        }

        // info state
        if (displayState == INFO) {
          if (entered == -1) {
            if (selected < 4) {
              selected += 1;
            }
          }
          else if (entered == 0) {
            if (profiles[currProfile].skill >= 2)
            profiles[currProfile].skill -= 1;
            Serial.println(profiles[currProfile].skill);
          }
          else if (entered == 1) {
            if (profiles[currProfile].work >= 2) {
              profiles[currProfile].work -= 1;
              profiles[currProfile].storeWork = profiles[currProfile].work;
            }
            Serial.println(F("SUB 1 WORK"));
          }
          else if (entered == 2) {
            if (profiles[currProfile].rest >= 40) {
              profiles[currProfile].rest -= 20;
              profiles[currProfile].storeRest = profiles[currProfile].rest;
            }
            Serial.println(F("SUB 20 REST"));
          }
          else if (entered == 3) {
            if (profiles[currProfile].sets >= 2) {
              profiles[currProfile].sets -= 1;
            }
            Serial.println(F("SUB 1 SET"));
          }
        }

        // for timer state
        if (displayState == TIMER) {
          Serial.println(F("RESET"));
          startTimer = false;
          pauseTimer = false;
          count = storeCount;
          currentState = IDLE;
          profiles[currProfile].work = profiles[currProfile].storeWork;
          profiles[currProfile].rest = profiles[currProfile].storeRest;
          trackSets = 0;
          startTime = 6;
          Serial.println(count);
        }
      }
    }
  }
  prev_button2 = reading;  // change the present state to current reading
}

uint8_t trackEnter = 0;

void thirdButton() {
  // enter for MENU
  // enter for EDIT
    // save value
  // enter for INFO
    // save value
  reading = digitalRead(button3);

  // case for if switch changed due to noise or pressing
  if (reading != prev_button3) {
    lastDebounceTime = millis();
  }

  // if the time between current and last debounce is greater than delay its a press
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != button3_state) {
      button3_state = reading;  // update the button state
      // indicates a button press
      if (button3_state == LOW) {
        // enter button
        if (displayState == MENU) {
          entered = selected;
          currProfile = selected;
        }
        if (displayState == INFO) {
          // allows reuse
          if (trackEnter == 1) {
            trackEnter = 0;
            entered = -1;
            return;
          }
          entered = selected; // the entered value is the profile it goes to
          if (entered == 0) {
            Serial.println("ENTER A SKILL");
          }
          trackEnter++;
        }
        if (displayState == OPTIONS) {
          entered = selected;
        }
      }
    }
  }
  prev_button3 = reading;  // change the present state to current reading
}

void fourthButton() {
  reading = digitalRead(button4);

  if (reading != prev_button4) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != button4_state) {
      button4_state = reading;
      if (button4_state == LOW && (currentState == IDLE || currentState == DONE)) {
        if (displayState == TIMER) {
          if (currentState == IDLE || currentState == DONE) {
            currentState = IDLE;
            previousState = DIFF;
          }
        }
        if (displayState == TIMER || displayState == OPTIONS || displayState == INFO || displayState == MENU) {
          selected = 0;
          entered = -1;
          currProfile = 10;
          displayState = MENU;
        } 
        // reset button
        // add an option to delete profile later
      }
    }
  }
  prev_button4 = reading;
} 

// logic for showing which item user is hovering
void selection(const __FlashStringHelper* label, int current, int y) {
  display.setCursor(0, y);

  if (selected == current) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  }
  else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print(label);
}

// saves the inputted user data
void saveProfile(Profile &p, uint8_t curr) {
  Serial.println(profiles[curr].skill);
  Serial.println(profiles[curr].work);
  Serial.println(profiles[curr].rest);
  Serial.println(profiles[curr].sets);
  // gets the size of the profile to find the address
  int addr = curr * sizeof(p);
  EEPROM.put(addr, p); // stores the values of p
  Serial.println("SAVED");
}

// displays the different profiles user has saved
void displayMenu() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);     // Start at top-left corner
  display.print(F("TABATA MODULE REV 1.0"));

  
  selection(F(" 1. "), 0, 15);
  if (profiles[0].skill > 0) {
    skillName(0);
  }
  else {
    display.print(F("SET PROFILE"));
  }

  selection(F(" 2. "), 1, 25);
  if (profiles[1].skill > 0) {
    skillName(1);
  }
  else {
    display.print(F("SET PROFILE"));
  }

  selection(F(" 3. "), 2, 35);
  if (profiles[2].skill > 0) {
    skillName(2);
  }
  else {
    display.print(F("SET PROFILE"));
  }

  selection(F(" 4. "), 3, 45);
  if (profiles[3].skill > 0) {
    skillName(3);
  }
  else {
    display.print(F("SET PROFILE"));
  }

  selection(F(" 5. "), 4, 55);
  if (profiles[4].skill > 0) {
    skillName(4);
  }
  else {
    display.print(F("SET PROFILE"));
  }
  
  
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);
}

// add more skill names here
void skillName(uint8_t profile) {
  if (profiles[profile].skill == 1) {
    display.print(F("FRONT LEVER"));
  }
  if (profiles[profile].skill == 2) {
    display.print(F("PLANCHE"));
  }
  if (profiles[profile].skill == 3) {
    display.print(F("HANDSTAND"));
  }
  if (profiles[profile].skill == 4) {
    display.print(F("BACKLEVER"));
  }
  if (profiles[profile].skill == 5) {
    display.print(F("MALTESE"));
  }
  if (profiles[profile].skill == 6) {
    display.print(F("L-SIT"));
  }
  if (profiles[currProfile].skill == 7) {
    display.print(F("HUMAN FLAG"));
  }
}

void displayInfo(uint8_t curr) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  selection(F("1. SKILL: "), 0, 5);
  skillName(curr);
  selection(F("2. EXERCISE TIME: "), 1, 15);
  display.print(profiles[curr].work);
  selection(F("3. REST TIME: "), 2, 25);
  display.print(profiles[curr].rest);
  selection(F("4. SETS: "), 3, 35);
  display.print(profiles[curr].sets);
  selection(F("5. SAVE PROFILE"), 4, 45);
}

// displays the options to either start or edit a profile
void displayOptions() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  selection(F(" 1. START "), 0, 0);
  selection(F(" 2. EDIT  "), 1, 20);
  selection(F(" 3. DELETE"), 2, 40);
}

// displays info that user set or saved previously and shows whether its resting, working, or starting
void displayTimer(TimerState currentState, uint8_t curr) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // exercise text
  display.setCursor(0, 0);     // Start at top-left corner
  skillName(curr);

  // exercise interval text
  display.setCursor(0, 20);
  display.print(F("Exercise Interval: "));
  display.print(profiles[curr].storeWork);

  // rest interval text
  display.setCursor(0, 30);
  display.print(F("Rest Interval: "));
  display.print(profiles[curr].storeRest);

  // sets text
  display.setCursor(0, 40);
  display.print(F("Sets: "));
  display.print(profiles[curr].sets);

  // state text
  display.setCursor(0, 57); // 57
  if (currentState == START) {
    display.print(F("STARTING"));
  }
  else if (currentState == WORK) {
    display.print(F("EXERCISING"));
  }
  else if (currentState == REST) {
    display.print(F("RESTING"));
  }
  else if (currentState == PAUSE) {
    display.print(F("PAUSED"));
  }
  else if (currentState == DONE) {
    display.print(F("COMPLETED"));
  }
  else if (currentState == IDLE) {
    display.print(F(""));
  }

  // horizontal lines
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);
  display.drawFastHLine(0, 52, 128, SSD1306_WHITE);
}
