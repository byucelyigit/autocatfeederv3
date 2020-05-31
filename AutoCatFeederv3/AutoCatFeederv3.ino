/*
-- Aşağıdaki kod Mega'ya yüklendiğinde düzgün bir şekilde grafik ve yazılar
-- gösterildi. Ek olarak step motor da çalıştı.
-- Bir sebeple tüm kütüphane dosylaırnın aşağıdaki dizinde olması gerekiyor. Ek olarak bunu kütüphane yöneticisi ile eklemek gerekiyor olabilir.
-- D:\OneDrive\Documents\Arduino\libraries
-- #error("Height incorrect, please fix Adafruit_SSD1306.h!");
-- yukarıdaki şekilde hata verdiğinde kütüphanedeki ilgili bölümün dınanıma göre uygun şekilde değişitilrmesi lazım.
-- Bunun bir iyi versiyonu daha vardı. orada otomatik olarak oled ekranı kapatan kodlar da vardı. Nereye gittiklerini bilmiyorum.
-- onenote içinde daha detaylı açıklamalar var.
--testtest

mode values
0 do nothing
1 display "init"
2 run for -> init pos
3 display "Opening..."
4 run for -> Open
5 display "Closing..." 
6 run for -> close
9 init pos achieved
10 Yemek vakti
11 Yemek vakti sonu

doorStatus
0 unknown
1 Closed
2 Open

buttonStatus
0 open/close
1 set time
2 set alarm


*/

#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <X113647Stepper.h>
#include <RTClib.h>
#include <SoftwareWire.h>
#include <HX711.h>



#define DOUT1 7
#define CLK1  6

HX711 scale;


RTC_DS1307 RTC;

static const int STEPS_PER_REVOLUTION = 32 * 64;

static const int PIN_IN1_BLUE         = 8;
static const int PIN_IN2_PINK         = 9;
static const int PIN_IN3_YELLOW       = 10;
static const int PIN_IN4_ORANGE       = 11;

// initialize the stepper library
tardate::X113647Stepper myStepper(
  STEPS_PER_REVOLUTION,
  PIN_IN1_BLUE, PIN_IN2_PINK, PIN_IN3_YELLOW, PIN_IN4_ORANGE
);


#define OLED_RESET 4
const int button1Pin = 3; 
const int button2Pin = 2;               
const int button3Pin = 4; //mode
const int stopPin = 5;
const int slideDistance = 1750;

int buttonStatus = 0;
int doorStatus = 0;
bool positionKnown = false;
String displayText = "";
String clockText ="";
String scaleText = "";
String alarmText ="";
int mode = 0;
int timerCount = 0;
bool button3Pressed = false;
bool button2Pressed = false;
bool button1Pressed = false;
int alarmHr = 0;
int alarmMin = 0;
int clockHr = 0;
int clockMin = 0;
float calibration_factor = -1050; //-7050 worked for my 440lb max scale setup
  


Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


/*
#define LOGO16_GLCD_HEIGHT 32
#define LOGO16_GLCD_WIDTH  16 
/*
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
  */

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   {                
  Serial.begin(9600);
  Serial.println("setup");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();

  scale.begin(DOUT1, CLK1);




  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);


  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0

  //Serial.println("Readings:");


  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // This will reflect the time that your sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
    //RTC.adjust(DateTime(2019, 1,21,3,0,0));
    Serial.println("Time set!");
  }

  //delay ( 2000 );

  // Setup Time library  
  display.clearDisplay();
  Serial.println("clearDisplay");

  //delay(2000);

  display.display(); 

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);
  pinMode(stopPin, INPUT_PULLUP);


  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.

 

  // set the speed in rpm:
  myStepper.setSpeed(120);
  Serial.println("setSpeed");

}

  String SaatFormat(int hr, int min, int sec)
    {
      //String timestr = String() + (hr < 10 ? "0" : "") + hr  + ':' + (min < 10 ? "0" : "") + min + ":" + (sec < 10 ? "0" : "") + sec;
      char timestr[6];
      timestr[0] = '0' + hr / 10;
      timestr[1] = '0' + hr % 10;
      timestr[2] = ':';
      timestr[3] = '0' + min / 10;
      timestr[4] = '0' + min % 10;
      //timestr[5] = ':';
      //timestr[6] = '0' + sec / 10;      
      //timestr[7] = '0' + sec % 10;
      timestr[5] = '\0';
      //char format[16];
      //snprintf(format, sizeof(format), "%02d:%02d:%02d", hr, min, sec);
      return timestr;
    }

  String AlarmFormat(int hr, int min)
    {
      //char format[16];
      //snprintf(format, sizeof(format), "%02d:%02d\0", hr, min);
      char alarmstr[6];
      alarmstr[0] = '0' + hr / 10;
      alarmstr[1] = '0' + hr % 10;
      alarmstr[2] = ':';
      alarmstr[3] = '0' + min / 10;
      alarmstr[4] = '0' + min % 10;
      alarmstr[5] = '\0';
      return  alarmstr;
    }

  String ScaleFormat(float scale)
    {
      int iscale = scale;
      char scalestr[3];
      scalestr[0] = '0' + iscale / 10;
      scalestr[1] = '0' + iscale % 10;
      scalestr[1] = '\0';
      return  scalestr;
    }    


void loop() {
  DateTime now = RTC.now(); 
  clockText =  SaatFormat(now.hour(), now.minute(),now.second());
  alarmText = AlarmFormat(alarmHr, alarmMin);
  Serial.println(clockText);
  clockMin = now.minute();
  clockHr = now.hour();

  //Serial.print("Reading: ");
  float scl = scale.get_units();
  Serial.println(scl); //scale.get_units() returns a float
  scaleText = ScaleFormat(scl);
  //Serial.print(" gr"); //You can change this to kg but you'll need to refactor the calibration_factor
  //Serial.println();


  int buttonState1 = digitalRead(button1Pin);
  int buttonState2 = digitalRead(button2Pin);
  int buttonState3 = digitalRead(button3Pin);
  int stopPinState = digitalRead(stopPin);

  if(mode==0)
  {
    if((clockMin==alarmMin) && (clockHr==alarmHr) && (now.second() == 0))
    {
      mode = 10;
    }
  }

  if(buttonState3==1)
  {
    button3Pressed = false;
  }

  if (buttonState3 == 0) {
    //mode = 1; // motor runs
    if(!button3Pressed)
    {
      switch(buttonStatus) {
        case 1: 
          buttonStatus = 2;
          displayText = "Mode: Set Alarm";
          //RTC.adjust(DateTime(2019,1,21,clockHr,clockMin,0));
          break;
        case 2:
          buttonStatus = 0;
          displayText = "Mode: O/C ";
          //burada alarm değeri değişmiş ise eproma kayıt etmesi lazım. elektrik gidince yeniden okur
          //
          break;
        
        case 0:
          buttonStatus = 1;
          displayText = "Mode: Set Time";
          break;
      }
      button3Pressed = true;
    }
  }

  if(buttonState2 == 1)
  { 
    button2Pressed = false;
  }

  if (buttonState2 == 0) {
    if(!button2Pressed)
    {
      if(buttonStatus==2)
      {
        alarmMin = alarmMin + 10;
        if(alarmMin==60) {alarmMin = 0;}
      }
      if(buttonStatus == 1)
      {
        clockMin = now.minute();
        clockMin = clockMin + 1;
        if(clockMin == 60) {clockMin = 0;}
        RTC.adjust(DateTime(2019,1,21,now.hour(),clockMin,0));
      }
    }
    button2Pressed = true;
  }

  if(buttonState1 == 1)
  { 
    button1Pressed = false;
  }

  if (buttonState1 == 0) {
    timerCount = 0;
    if(!button1Pressed)
    {
      if(buttonStatus==1)
      {
        clockHr = now.hour();
        clockHr = clockHr + 1;
        if(clockHr == 24) { clockHr = 0;}
        RTC.adjust(DateTime(2019,1,21,clockHr,now.minute(),0));
      }

      if(buttonStatus==2)
      {
        alarmHr = alarmHr + 1;
        if(alarmHr == 24) { alarmHr = 0;}
      }

      if(buttonStatus==0)
      {
        if(mode==0)
        {
          if(positionKnown)
          {
            if(doorStatus == 1) //kapalı
            {
              mode = 3;
            }
            else //açık
            {
              mode = 5;
            }
          }
          else
          {
            mode = 1;      
          }
        }
      }
    }
    button1Pressed = true;
  }

  if (stopPinState == 0) {
    if(mode==2)
    {
      mode = 9;
    }
  }

  //Serial.println(buttonStatus);

  if(mode==9)
  {
    positionKnown = true; //acil duruş
    doorStatus = 1;
    mode = 3; //kapak açılır.
    timerCount = 0;
  }

  if(mode==10) //yemek vakti
  {
    mode = 3;
    delay(2000);  //tekrar alarm tetiklenmesin diye 2 saniye sonra açar  
    buttonStatus = 0;    
  }


  if(mode==2)
  {
    myStepper.step(20);
    timerCount = timerCount +1;
  }

  if(timerCount>50)
  {
    mode = 0;
    displayText = "Error";
  }

  if(mode==1)
  {
    displayText = "Init..";
    mode = 2;
  }

  if(mode==4)
  {
    myStepper.step(-slideDistance);
    mode = 0;
    displayText = "Open";
    doorStatus = 2;
  }

  if(mode==3)
  {
    if(doorStatus==1)
    {
      displayText = "Opening..";
      mode = 4;
      doorStatus = 0;
    }
  }

  if(mode==6)
  {
    myStepper.step(slideDistance);
    mode = 0;
    displayText = "Closed";    
    doorStatus = 1;
  }

  if(mode==5)
  {
    displayText = "Closing...";    
    mode = 6;
    doorStatus = 0;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(displayText);

  display.setCursor(0,18);
  display.setTextSize(2);

  display.print(clockText);

  
  display.setCursor(0,40);
  display.setTextSize(2);
  display.print(alarmText);

  display.setCursor(0,55);
  display.setTextSize(1);
  display.print(scaleText);

  display.display();

  //Serial.println(buttonStatus);


  // step one revolution in one direction:
  //myStepper.step(1);
  //delay(200);

  // step one revolution in the other direction:
  //myStepper.step(-STEPS_PER_REVOLUTION/2);
  //delay(1000);
}
