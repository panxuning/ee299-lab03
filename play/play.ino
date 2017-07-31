#include <LiquidCrystal.h>
#include <Wire.h>
#include "pitches.h"

LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);

// notes in the melody:
#define LENGTH 102
int melody[] = {
  NOTE_E5, NOTE_E5, NOTE_E5, 0, NOTE_C5, NOTE_E5, NOTE_G5, 0, NOTE_G4, 0,
  NOTE_C5, 0, NOTE_G4, 0, NOTE_E4, 0, NOTE_A4, NOTE_B4, 0, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_F5, NOTE_G5, 0, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4, 0,
  NOTE_C5, 0, NOTE_G4, 0, NOTE_E4, 0, NOTE_A4, NOTE_B4, 0, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_F5, NOTE_G5, 0, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4, 0,
  0, NOTE_G5, NOTE_FS5, NOTE_F5, NOTE_DS5, NOTE_E5, 0, NOTE_GS4, NOTE_A4, NOTE_C5, 0, NOTE_A4, NOTE_C5, NOTE_D5,
  0, NOTE_G5, NOTE_FS5, NOTE_F5, NOTE_DS5, NOTE_E5, 0, NOTE_C6, NOTE_C6, NOTE_C6, 0,
  0, NOTE_G5, NOTE_FS5, NOTE_F5, NOTE_DS5, NOTE_E5, 0, NOTE_GS4, NOTE_A4, NOTE_C5, 0, NOTE_A4, NOTE_C5, NOTE_D5,
  0, NOTE_DS5, 0, NOTE_D5, 0, NOTE_C5, 0
};

byte noteDurations[] = {
  8, 4, 8, 8, 8, 4, 4, 4, 4, 4,
  4, 8, 8, 4, 4, 8, 4, 8, 8, 8, 4,
  6, 6, 6, 4, 8, 8, 8, 4, 8, 8, 4, 8,
  4, 8, 8, 4, 4, 8, 4, 8, 8, 8, 4,
  6, 6, 6, 4, 8, 8, 8, 4, 8, 8, 4, 8,
  4, 8, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  4, 8, 8, 8, 4, 8, 8, 4, 8, 4, 4, 
  4, 8, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
  4, 4, 8, 4, 8, 4, 4, 
};

bool note_lane[LENGTH + 1];
long note_time[LENGTH + 1];
//indicate which note is playing now
int curr_note = 0;
//is a note playing now
bool playing = false;

const int tilt = 9;
const int knob = 1;
const int button = 10;

//temp variable
int temp;


class Car
{
  public:
  //register port for lane(left or right),
  //         port for speed(analog)
  Car(int lane_port);
  //read data from ports
  void read();
  void print();
  int lane;
  double speed;
  private:
  int _lane_port;
  int _speed_port;
};

class Court
{
  public:
  Court();
  void update(Car &car);
  String lane1;
  String lane2;
  double score() {return _score;}
  long time() {return millis() - starttime;}
  bool over(const long &duetime) {return (millis() - starttime >= duetime);}
  void retime();
  void getscore(const double &x) {_score += x;}
  int combo;
  int maxcombo;
  private:
  double _score;
  long starttime;
  double pos;
};

Car car(tilt);
Court court;
void generate_map();
void print_lane(LiquidCrystal &lcd, String str1, String str2);
void play();
void paint_court(String &lane1, String &lane2);

bool connect;
bool select_mode;
bool select_speed;
bool is_host;
bool run;
bool result;

void setup()
{
  Serial.begin(9600);
  //set a shorter timeout
  generate_map();
  Serial.setTimeout(300);
  lcd.begin(16, 2);
  connect = run = result = select_speed = false;
  select_mode = true;
  is_host = true;
}

void loop()
{
  String str;  
  if (select_mode) {
    int temp = (analogRead(knob) / 256 ) & 1;
    if (!temp)
      lcd.print('>');
    else
      lcd.print(' ');
    lcd.print("Single");
    if (temp)
      lcd.print(" >");
    else
      lcd.print("  ");
    lcd.print("Double");
    
    //return cursor
    lcd.setCursor(0, 0);
    
    if (digitalRead(button)) {
      delay(150);
      select_mode = false;
      //branch on single or double
      if (temp)
         connect = true;
      else
      {
          connect = false;
          //single player the console is always the host
          is_host = true;
          select_speed = true;
      }
      //wait for the button to release
      while(digitalRead(button));
    }
  }
  
  if (connect) {
    lcd.begin(16, 2);
    temp = 0;
    print_lane(lcd, String("Pairing...      "), String("                "));
    //TCP/IP style connection  
    //there are still no timeout and recognition now
      if (Serial.available()) {
        str = Serial.readString();
        
        //send signal
        if (str == "ACK") {
          connect = false;
          select_speed = true;
          is_host = false;
        }
        if (str == "SYN-ACK") {
          Serial.print("ACK");
          //wait for timeout
          delay(300);
          is_host = true;
          connect = false;
          select_speed = true;
          
          //wait for the away console
        } else
        if (str == "SYN") {
          //who received
          Serial.print("SYN-ACK");
          //wait for timeout
          delay(300);
        }        
        else {
          Serial.print("SYN");
          //wait for timeout
          delay(300);
        }
      }
      else
      {
        Serial.print("SYN");
        //wait for timeout
        delay(300);
      }
  }

  
  if (select_speed)
  {
    if (is_host) {
       int int_speed = analogRead(knob);
       String s = "Speed:";
       //print speed gauge
       for (int i = 1;i <= 10; ++i) {
          if (i * 102.2 < int_speed)
            s += ">";
          else
            s += "-";
       }
       print_lane(lcd, s, String(1 + int_speed / 127.875));
       if (digitalRead(button)) {
          select_speed = false;
          car.speed = 1 + (int_speed / 127.875);
          Serial.print(int_speed);
          run = true;
          
          //empty wait time, for the away console to sync
          delay(200);
          //wait for the button to release
          while(digitalRead(button));
          //random wait time
          Serial.print("\a");
          court.retime();
       }
    }
    else {
      print_lane(lcd, String("Waiting for host"), String("to set speed"));
      while(!Serial.available());
      car.speed = 1 + (Serial.parseInt() / 1023.0);
      run = true;
      select_speed = false;

      //wait for start signal
      while(!Serial.available());
      court.retime();
    }
  }
  
  if (run)
  {
    car.read();
    play();
    court.update(car);
    print_lane(lcd, court.lane1, court.lane2);
  }
  
  if (result)
  {
    lcd.begin(16, 2);
    print_lane(lcd, String("Score: ") + String(court.score()) + String("               "), String("Max combo: ") + String(court.maxcombo));
    result = false;
  }
}

//specify ports
Car::Car(int lane_port)
{
  _lane_port = lane_port;
}

//read data
void Car::read()
{
  lane = digitalRead(_lane_port);
  //speed ranges from 1.0x to 2.0x
}

//for debugging use
void Car::print()
{
  Serial.print("Lane: ");
  Serial.println(lane ? "left" : "right");
  Serial.print("Speed: ");
  Serial.println(speed);  
}

Court::Court()
{
  _score = 0;
  pos = 0;
  maxcombo = combo = 0;
  starttime = millis();
  lane1 = String(65535, BIN);
  lane2 = String(65535, BIN);
}

//print strings to lcd display
void print_lane(LiquidCrystal &lcd, String str1, String str2)
{
  
  char chrstr[16];
  //print first line
  lcd.setCursor(0, 0);
  str1.toCharArray(chrstr, 16);
  lcd.print(chrstr);
  //to second line
  lcd.setCursor(0, 1);

  //print second line
  str2.toCharArray(chrstr, 16);
  lcd.print(chrstr);
}

void print_lane(LiquidCrystal &lcd, char str1[], char str2[])
{
  
  //print first line
  lcd.setCursor(0, 0);
  lcd.print(str1);
  //to second line
  lcd.setCursor(0, 1);

  //print second line
  lcd.print(str2);
}

void Court::retime()
{
  starttime = millis();
}

//update lane
void Court::update(Car &car)
{
    paint_court(court.lane1, court.lane2);
    if (car.lane) {
      lane1.setCharAt(0, '>');
      lane2.setCharAt(0, '|');
    }
    else {
      lane1.setCharAt(0, '|');
      lane2.setCharAt(0, '>');
    }
}

void play() {
    if (court.over(note_time[curr_note]) && !playing) {
        // to calculate the note duration, take one second divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int noteDuration = 1000 / noteDurations[curr_note];
        tone(8, melody[curr_note], noteDuration);

        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        playing = true;
    }
    if (court.over(note_time[curr_note + 1]) || result) {
        //score now
        if (!result) {
            if (car.lane == note_lane[curr_note + 1]) {
                court.getscore(1 + (++court.combo)/100.0);
                if (court.combo > court.maxcombo)
                    court.maxcombo = court.combo;
            }
            else
              if (note_lane[curr_note + 1] < 2)
                court.combo = 0;
        }
        // stop the tone playing:
        curr_note++;
        noTone(8);
        playing = false;
        if (curr_note == LENGTH - 1) {
           run = false; 
           result = true;
        }
    }
}

void generate_map()
{
    //unconnected pin used to get randomness
    randomSeed(analogRead(0));
    //decide which lane the note is on
    curr_note = 0;
    note_time[0] = 0;
    note_lane[0] = random(3);
    for (int i = 1; i < LENGTH; ++i) {
        note_time[i] = note_time[i - 1] + 1300 / noteDurations[i - 1];
        note_lane[i] = random(random(12)) % 3;
    }
}

void paint_court(String &lane1, String &lane2)
{
    lane1 = String("                ");
    lane2 = String("                ");
    int printtime = court.time();
    int i;
    // one character accounts for 250/speed ms.
    for(int j = curr_note - 1; ; ++j)
    {
        i = (note_time[j] - court.time()) / (250/car.speed);
        if (i >= 16)
            break;
        else {
            if (note_lane[j]) {
                if (i >= 0)
                  lane2.setCharAt(i, 'O');
                for (int k = 1; 0 <= i + k && i + k < 16 && note_time[j] + 1000/noteDurations[j] - court.time() > (i + k)*(250/car.speed); ++k)
                    lane2.setCharAt(i + k, '=');
            }
            if (!note_lane[j]) {
                if (i >= 0)
                  lane1.setCharAt(i, 'O');
                for (int k = 1; 0 <= i + k && i + k < 16 && note_time[j] + 1000/noteDurations[j] - court.time() > (i + k)*(250/car.speed); ++k)
                    lane1.setCharAt(i + k, '=');
            }
        }
    }
}
