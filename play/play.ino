#include <LiquidCrystal.h>
#include <Wire.h>

LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);

const String road1 = "Lorem ipsum dolor sit amet, eget a sed nunc arcu id, vel sit urna facilisis, facilisis ipsum diam sollicitudin lacus usto magna integer.s velit dictum, montes a ac nibh dictum, ac mauris pellentesque morbi tellus purus eiusmod, pulvinar neque.";
const String road2 = "Et erat hendrerit lectus tempus sociis sed, leo pellentesque augue massa, erat etiam, et quam. Tempor felis Pede rhoncus sapien orci aenean at elit, u ante at lobortispis pellentesque, in lorem tristique in ullamcorper adipiscing corrupti";

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
  void retime();
  private:
  double _score;
  long starttime;
  long ntime;
  double pos;
};

Car car(tilt);
Court court;

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
  Serial.setTimeout(300);
  lcd.begin(16, 2);
  connect = run = result = select_speed = false;
  select_mode = true;
  is_host = true;
}

void loop()
{
  String str;
  int length = 125;
  
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
            s += "-";
          else
            s += "*";
       }
       print_lane(lcd, s, String(1 + int_speed / 1023.0));
       if (digitalRead(button)) {
          select_speed = false;
          car.speed = 1 + (int_speed / 1023.0);
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
    court.update(car);
    print_lane(lcd, court.lane1, court.lane2);
  }
  
  if (result)
  {
    print_lane(lcd, String("Score:             "), String(court.score()) + String("               "));
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
  ntime = millis();
}
//update lane
void Court::update(Car &car)
{
  //reposition
  double old_pos = pos;
  pos += (millis() - ntime) * car.speed / 200;
  //see if new block reached
  int flag = (int)pos - (int)old_pos;
  ntime = millis();
  //TO-DO: check collision
  if (flag) {
    if (car.lane) {
      if (flag && lane1.charAt(1) != ' ')
        _score += 1;
      lane1.setCharAt(0, '>');
      lane2.setCharAt(0, '|');
    }
    else {
      if (flag && lane2.charAt(1) != ' ')
        _score += 1;
      lane1.setCharAt(0, '|');
      lane2.setCharAt(0, '>');
    }
    for(int i = 1; i < 16; i++)
    {
      int j = (int)(pos + i) >> 3;
      int k = (int)(pos + i) & 7;
      lane1.setCharAt(i, bitRead(road1[j], k) ? '-' : ' ');
      lane2.setCharAt(i, bitRead(road2[j], k) ? '-' : ' ');
    }
  }
  //force stop if endpoint reached
  if (pos > 125)
  {
    run = false;
    result = true;
  }
}

void refine(char str[])
{
  
}


