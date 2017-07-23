#include <LiquidCrystal.h>
#include <Wire.h>

LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);

const String road1 = "Lorem ipsum dolor sit amet, eget a sed nunc arcu id, vel sit urna facilisis, facilisis ipsum diam sollicitudin lacus usto magna integer.s velit dictum, montes a ac nibh dictum, ac mauris pellentesque morbi tellus purus eiusmod, pulvinar neque.";
const String road2 = "Et erat hendrerit lectus tempus sociis sed, leo pellentesque augue massa, erat etiam, et quam. Tempor felis Pede rhoncus sapien orci aenean at elit, u ante at lobortispis pellentesque, in lorem tristique in ullamcorper adipiscing corrupti";

class Car
{
  public:
  //register port for lane(left or right),
  //         port for speed(analog)
  Car(int lane_port, int speed_port);
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
  private:
  double _score;
  long starttime;
  long ntime;
  double pos;
};

Car car(9, 1);
Court court;

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
}

void loop()
{
  if (court.time() < 12000)
  {
    car.read();
    court.update(car);
    print_lane(lcd, court.lane1, court.lane2);
  }
  else
  {
    print_lane(lcd, "Score:             ", String(court.score()) + String("               "));
    delay(15000);
  }
}

//specify ports
Car::Car(int lane_port, int speed_port)
{
  _lane_port = lane_port;
  _speed_port = speed_port;
}

//read data
void Car::read()
{
  lane = digitalRead(_lane_port);
  int _speed = analogRead(_speed_port);
  //speed ranges from 1.0x to 2.0x
  speed = 1 + _speed / 1023.0;
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
  if (flag)
    for(int i = 1; i < 16; i++)
    {
      int j = (int)(pos + i) >> 3;
      int k = (int)(pos + i) & 7;
      lane1.setCharAt(i, bitRead(road1[j], k) ? '-' : ' ');
      lane2.setCharAt(i, bitRead(road2[j], k) ? '-' : ' ');
    }
  if (car.lane)
  {
    if (flag && lane1.charAt(1) != ' ')
      _score += 1;
    lane1.setCharAt(0, '>');
    lane2.setCharAt(0, '|');
  }
  else
  {
    if (flag && lane2.charAt(1) != ' ')
      _score += 1;
    lane1.setCharAt(0, '|');
    lane2.setCharAt(0, '>');
  }
}

