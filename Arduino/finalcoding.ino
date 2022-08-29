#include <DS18B20.h>
#include <Adafruit_NeoPixel.h>
#include "HX711.h" //This library can be obtained here http://librarymanager/All#Avia_HX711
#ifdef __AVR__
#include <avr/power.h>
#endif

//LED
#define PIN        6
#define NUMPIXELS 60 

//LPF 필터 설정
static long analogPinTimer = 0;

#define ANALOG_PIN_TIMER_INTERVAL 2 //milliseconds
unsigned long thisMillis_old;

int fc = 10; // cutoff frequency 5~10 Hz 정도 사용해보시기 바랍니다
double dt = ANALOG_PIN_TIMER_INTERVAL/1000.0; // sampling time
double lambda = 2*PI*fc*dt;
double kg = 0.0;
double kg_f = 0.0;
double kg_fold = 0.0;

//로드셀 영점 조정
#define calibration_factor -12050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

//로드셀
#define LOADCELL_DOUT_PIN  5 
#define LOADCELL_SCK_PIN  4

HX711 scale;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int cds=A5; // 조도센서
int temp=2; // 온도센서

int led=6; //조도제어 (LED띠)
int pump1=8; // 수위제어 (펌프AA)
int pump2=9; // 수위제어 (펌프AB)
int pump3=10; // 수위제어 (펌프BA)
int pump4=11; // 수위제어 (펌프BB)
int relay=12; // 온도제어 (릴레이모듈)

DS18B20 ds(2);
uint8_t address[] = {40, 250, 31, 218, 4, 0, 0, 52};
uint8_t selected;

// 제어기 컨트롤 0=off/ 1=on
int controlmode = 0; //자동제어 or 사용자제어
int led_con = 0;
int pum1 = 0; //입수
int pum2 = 0; //출수
int heater_con = 0;

//조도,수위값
int cdsvalue = 0;
float waterlevel = 0;

//시리얼 통신을 위한 센서값(string)
String cdsvalue_s;
String tempdata;
String waterlevel_s;

//////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(9600);

  #if defined(__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif

  pinMode(11, OUTPUT);
  pixels.begin();

  pinMode(pump1,OUTPUT);
  pinMode(pump2,OUTPUT);
  pinMode(pump3,OUTPUT);
  pinMode(pump4,OUTPUT);

  digitalWrite(pump1,LOW);
  digitalWrite(pump2,HIGH);
  digitalWrite(pump3,LOW);
  digitalWrite(pump4,HIGH);
  
  pinMode(relay,OUTPUT);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(1000);  //Assuming there is no weight on the scale at start up, reset the scale to 0

}

//////////////////////////////////////////////////////////////////////////

void loop() {
  unsigned long deltaMillis = 0; // clear last result
  unsigned long thisMillis = millis();  
  if (thisMillis != thisMillis_old) { 
    deltaMillis = thisMillis-thisMillis_old; 
    thisMillis_old = thisMillis;   
  } 

  analogPinTimer -= deltaMillis; 
  
  if (analogPinTimer <= 0){
    analogPinTimer += ANALOG_PIN_TIMER_INTERVAL;
  
    float lbs = scale.get_units();
    kg = lbs * 0.453592; // 아날로그값 읽기
    kg_f = lambda/(1+lambda)*kg+1/(1+lambda)*kg_fold; //필터된 값
    kg_fold = kg_f; // 센서 필터 이전값 업데이트
    waterlevel = kg_f/(45*30)*1000; //단위 cm
    waterlevel_s = String(waterlevel);

    // 값 센싱
    cdsvalue = analogRead(cds);
    cdsvalue_s = String(cdsvalue);
    tempdata = String(ds.getTempC());
    
    // 라즈베리파이로 전송
    Serial.println(cdsvalue_s+","+tempdata+","+waterlevel_s+",0"); //scale.get_units() returns a float
  }
  
  
  //시리얼통신
  while(Serial.available()) //파이썬에서(라즈베리에서) 입력이 있을 때
  {
    String inString = Serial.readStringUntil('\n');
    controlmode = int(inString[0])-48;
    heater_con = int(inString[1])-48;
    led_con = int(inString[2])-48;
    pum1 = int(inString[3])-48;
    pum2 = int(inString[4])-48;
  }

  
  // Auto control
  // 조도 제어
  if (controlmode == 0){
    if ( cdsvalue < 650 ) {
      for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(250, 250, 250)); //컬러설정(R,G,B)
        pixels.show();
      }
    }
    else if ( cdsvalue > 800 ) {
      for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0)); //컬러설정(R,G,B)
        pixels.show();
      }
    }
 // 수위 제어
    if ( waterlevel < 10 ) {  
      digitalWrite(pump1,LOW);
      digitalWrite(pump2,LOW);
      digitalWrite(pump3,LOW);
      digitalWrite(pump4,HIGH);
    }
    else if ( waterlevel > 20 ) { 
      digitalWrite(pump1,LOW);
      digitalWrite(pump2,HIGH);
      digitalWrite(pump3,LOW);
      digitalWrite(pump4,LOW);
    }
    else {
      digitalWrite(pump1,LOW);
      digitalWrite(pump2,HIGH);
      digitalWrite(pump3,LOW);
      digitalWrite(pump4,HIGH);
    }

  // 온도 제어
    if ( ds.getTempC() < 25.0 ) {
      digitalWrite(relay,HIGH);
    }
    else if ( ds.getTempC() > 28.0 ) {
      digitalWrite(relay,LOW);
    }
  }
  // user control
  // 조도 제어
  if (controlmode == 1){
    if ( led_con == 1 ) {
      for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(250, 250, 250)); //컬러설정(R,G,B)
        pixels.show();
      }
    }
    if ( led_con == 0 ) {
      for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0)); //컬러설정(R,G,B)
        pixels.show();
      } 
    }
    
    // 수위 제어
    if ( pum1 == 1 ) {  
      digitalWrite(pump1,LOW);
      digitalWrite(pump2,LOW);
    }
    else if( pum1 == 0 ){
      digitalWrite(pump1,LOW);
      digitalWrite(pump2,HIGH);
    }
    if ( pum2 == 1 ) {  
      digitalWrite(pump3,LOW);
      digitalWrite(pump4,LOW);
    }
    else if ( pum2 == 0 ) {
      digitalWrite(pump3,LOW);
      digitalWrite(pump4,HIGH);
    }

  // 온도 제어
    if ( heater_con == 1 ) {
      digitalWrite(relay,HIGH);
    }
    else if ( heater_con == 0 ) {
      digitalWrite(relay,LOW);
    }
  }
}
