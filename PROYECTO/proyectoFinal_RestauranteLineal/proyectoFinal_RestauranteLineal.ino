//Proyecto Control Embebidos 2023A
#include <ESP32_FastPWM.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <LiquidCrystal_I2C.h>

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//Timers
hw_timer_t * timer_0 = NULL;

//Wifi
#define WIFI_SSID "Restaurante"
#define WIFI_PASSWORD "Mr305yatusa"

//Firebase
FirebaseData fbdo;

// Assign the project host and api key
#define FIREBASE_HOST "https://proyecto-embebidos-3f143-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "0T15chf4F9eNwvLV3jc3DmAVQRNduIWC3xRDbjy2"

struct Orden {
  int pedido;        // Initialize pedido array with zeros
  int mesa;          // Initialize mesa array with zeros
};

// Create an instance of the "Orden" structure
Orden order;

int Cola[4][20] = {    //[plato][vector de mesas en orden de llegada de pedido]
  {0},  //P1
  {0},  //P2
  {0},  //P3
  {0}   //P4
};

int ContPlatos[] = {0,0,0,0}; //Cuenta el número de pedidos por plato vigentes
const String platos[] = {"Salchipapa", "Chaulafan", "Encebollado", "Guatita"};
const float precio[] = {1.5, 2.5, 2.25, 2.75};

float Ganancia = 0.0;
String Time = " ";
String lastTime = " ";

int j = 0;    //Aux actualizar vector pedidos

//Medir tiempo
long prevTime = 0.0;

//Declaracion de PWMs
ESP32_FAST_PWM * PWM_01; //Control de velocidad de motor DC (banda)
ESP32_FAST_PWM * PWM_02; //Control de frecuencia de Emisores IR

//Declaracion de OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Declaracion de pines
byte IR = 15;
byte ENA = 2;
byte IN1 = 0;
byte IN2 = 4;
byte SM[5] = {19, 18, 5, 17, 16};
byte IRR[5] = {32, 33, 25, 26, 27};

//Definicion de Servos
Servo SM1; //Piston mesa 1
Servo SM2; //Piston mesa 2
Servo SM3; //Piston mesa 3
Servo SM4; //Piston mesa 4
Servo SM5; //Piston mesa 5

//Definicion de receptores (Clases)
/*IRrecv IRR1(IRR[4]);
IRrecv IRR2(IRR[1]);
IRrecv IRR3(IRR[2]);
IRrecv IRR4(IRR[3]);
IRrecv IRR5(IRR[0]);
decode_results results; // create a results object of the decode_results class
*/
//Variables globales
int frecuencia_motor = 500;
int dutyCycle_motor = 20;
byte plato = 0;
byte flag_despacho = 0; //bandera para proceso de despacho
byte flag_servo = 0;
byte max_SM1 = 170;
byte max_SM2 = 170;
byte max_SM3 = 170;
byte max_SM4 = 170;
byte max_SM5 = 175;

byte min_SM1 = 133;
byte min_SM2 = 132;
byte min_SM3 = 130;
byte min_SM4 = 129;
byte min_SM5 = 138;

unsigned int motor_down_1 = 250;
unsigned int motor_down_2 = 250;
unsigned int motor_down_3 = 250;
unsigned int motor_down_4 = 250;
unsigned int motor_down_5 = 250;

unsigned int servo_wait_1 = 700;
unsigned int servo_wait_2 = 700;
unsigned int servo_wait_3 = 700;
unsigned int servo_wait_4 = 700;
unsigned int servo_wait_5 = 700;

byte flag_pulse_IRR1 = 0;
byte flag_pulse_IRR2 = 0;
byte flag_pulse_IRR3 = 0;
byte flag_pulse_IRR4 = 0;
byte flag_pulse_IRR5 = 0;


//Funciones
void Agregar_Orden();
void Orden_Despachada(int);
void motorOff();
void motorHorario();
void motorAntiHorario();
void IRAM_ATTR onTimer(); //Sensor
void IRAM_ATTR IRR1_PULSE(); //Pulso de IRR1
void IRAM_ATTR IRR2_PULSE(); //Pulso de IRR2
void IRAM_ATTR IRR3_PULSE(); //Pulso de IRR3
void IRAM_ATTR IRR4_PULSE(); //Pulso de IRR4
void IRAM_ATTR IRR5_PULSE(); //Pulso de IRR5


////////////////////////////////////////////////////////////////////////////////
//////////                       Conf. Inicial              ////////////////////
////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.println();

  //Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Conectando al Wi-Fi");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.println("Conectado");

  //Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Serial.println("------------------------------------");
  Serial.println("-----------   Conectedo   ----------");
  Serial.println("------------------------------------");
  Firebase.getString(fbdo,"/Proy_Embebidos/Aux/Tiempo");
  lastTime = fbdo.stringData();
  prevTime = millis();

  //Definicion de pines
  pinMode(IR, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  for(int i = 0; i < 5; i++){
    pinMode(SM[i], OUTPUT);
    pinMode(IRR[i], INPUT_PULLUP);
  }
  //Inicializacion de LCD
  lcd.init();
  lcd.backlight();

  //Inicializacion de OLED
  if(!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setRotation(2);
  oled.display();

  //Inicializacion de servos
  SM1.attach(SM[0], 520, 2485);
  SM2.attach(SM[1], 520, 2485);
  SM3.attach(SM[2], 520, 2485);
  SM4.attach(SM[3], 520, 2485);
  SM5.attach(SM[4], 520, 2485);
  SM1.setPeriodHertz(50);
  SM2.setPeriodHertz(50);
  SM3.setPeriodHertz(50);
  SM4.setPeriodHertz(50);
  
  //Interrupciones sensor
  attachInterrupt(IRR[0], IRR1_PULSE, FALLING);
  attachInterrupt(IRR[1], IRR2_PULSE, FALLING);
  attachInterrupt(IRR[2], IRR3_PULSE, FALLING);
  attachInterrupt(IRR[3], IRR4_PULSE, FALLING);
  attachInterrupt(IRR[4], IRR5_PULSE, FALLING);
  

  //Inicializacion de PWMs
  PWM_01 = new ESP32_FAST_PWM(IR, 28, 100-10, 5, 16);
  PWM_02 = new ESP32_FAST_PWM(ENA, frecuencia_motor, dutyCycle_motor, 6, 12);
  if(PWM_01 && PWM_02){
    PWM_01 -> setPWM();
    PWM_02 -> setPWM();
  }

  //Inicializacion del timer
  timer_0 = timerBegin(0, 80, true); //Prescaler de 80 (reloj del timer es de 80MHz)
  timerAttachInterrupt(timer_0, &onTimer, true); //Funcion que se ejecute cada vez que termine la cuenta
  timerAlarmWrite(timer_0, (1000000)/(60), true); //Define el tiempo del timer en us (por el prescaler de 8)
  timerAlarmEnable(timer_0); //Habilita el timer

  motorOff();
  delay(500);
  
  //Mostrar LCD Cola
  lcd.clear();
  for(int i = 0; i<4; i++){
    lcd.setCursor(0,i);
    lcd.print(platos[i]);
    lcd.setCursor(11, i);
    lcd.print(": ");
    
    lcd.setCursor(12, i);
    for(int k = 0; k < 8; k++){
      if(Cola[i][k] != 0){
        lcd.print(Cola[i][k]);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//////////                          PROGRAMA                ////////////////////
////////////////////////////////////////////////////////////////////////////////
void loop() {
  //Leo tiempo de FireBase
  Firebase.getString(fbdo,"/Proy_Embebidos/Aux/Tiempo");
  Time = fbdo.stringData();

  //Orden de despacho
  if(Serial.available()){
    flag_despacho = 1;
    plato = Serial.readStringUntil('D').toInt();
  }

  //Proceso de despacho
  if(flag_despacho == 1){
    //Run conveyor
    motorHorario();
    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.println("  Plato en entrega  ");
    oled.print("\nPlato: ");
    oled.println(platos[plato - 1]);
    oled.print("Mesa: ");
    oled.println(Cola[plato-1][0]);
    oled.display();

    //Determina mesa a entregar
    switch (Cola[plato -1][0]){
      case 1:{
        if(flag_pulse_IRR1 == 1){
          timerRestart(timer_0);
          flag_servo = 0;
          flag_pulse_IRR1 = 0;
        }
        if(flag_servo == 1){
          //Detiene el motor
          delay(motor_down_1);
          motorOff();

          //Acciona servo
           delay(servo_wait_1);
          for(int i = min_SM1; i < max_SM1; i++){
            SM1.write(i);
            delay(16);
          }
          for(int i = max_SM1; i >= min_SM1; i--){
            SM1.write(i);
            delay(5);
          }
          flag_servo = 0;
          flag_despacho = 0;
          Orden_Despachada(plato);
        }
        break;
      }
      case 2:{
        if(flag_pulse_IRR2 == 1){
          timerRestart(timer_0);
          flag_servo = 0;
          flag_pulse_IRR2 = 0;
        }
        if(flag_servo == 1){
          //Detiene el motor
          delay(motor_down_2);
          motorOff();

          //Acciona servo
          delay(servo_wait_2);
          for(int i = min_SM2; i < max_SM2; i++){
            SM2.write(i);
            delay(16);
          }
          for(int i = max_SM2; i >= min_SM2; i--){
            SM2.write(i);
            delay(5);
          }
          flag_servo = 0;
          flag_despacho = 0;
          Orden_Despachada(plato);
        } 
        break;
      }
      case 3:{
        if(flag_pulse_IRR3 == 1){
          timerRestart(timer_0);
          flag_servo = 0;
          flag_pulse_IRR3 = 0;
        }
        if(flag_servo == 1){
          //Detiene el motor
          delay(motor_down_3);
          motorOff();

          //Acciona servo
          delay(servo_wait_3);
          for(int i = min_SM3; i < max_SM3; i++){
            SM3.write(i);
            delay(16);
          }
          for(int i = max_SM3; i >= min_SM3; i--){
            SM3.write(i);
            delay(5);
          }
          flag_servo = 0;
          flag_despacho = 0;
          Orden_Despachada(plato);
        }
        break;
      }
      case 4:{
        if(flag_pulse_IRR4 == 1){
          timerRestart(timer_0);
          flag_servo = 0;
          flag_pulse_IRR4 = 0;
        }
        if(flag_servo == 1){
          //Detiene el motor
          delay(motor_down_4);
          motorOff();

          //Acciona servo
          delay(servo_wait_4);
          for(int i = min_SM4; i < max_SM4; i++){
            SM4.write(i);
            delay(16);
          }
          for(int i = max_SM4; i >= min_SM4; i--){
            SM4.write(i);
            delay(5);
          }
          flag_servo = 0;
          flag_despacho = 0;
          Orden_Despachada(plato);
        }
        break;
      }
      case 5:{
        if(flag_pulse_IRR5 == 1){
          timerRestart(timer_0);
          flag_servo = 0;
          flag_pulse_IRR5 = 0;
        }
        if(flag_servo == 1){
          //Detiene el motor
          delay(motor_down_5);
          motorOff();

          //Acciona servo
          delay(servo_wait_5);
          for(int i = min_SM5; i < max_SM5; i++){
            SM5.write(i);
            delay(16);
          }
          for(int i = max_SM5; i >= min_SM5; i--){
            SM5.write(i);
            delay(5);
          }
          flag_servo = 0;
          flag_despacho = 0;
          Orden_Despachada(plato);
        }
        break;
      }
      default:{
        break;
      }
    }
  }
  //Nuevo pedido//
  if(!(Time.equals(lastTime))){
    Agregar_Orden();
  }

  //Mostrar en cocina (LaVIEW)
    Serial.print("T");
    Serial.print(ContPlatos[0]);
    Serial.print("M");
    Serial.print(ContPlatos[1]);
    Serial.print("C");
    Serial.print(ContPlatos[2]);
    Serial.print("B");
    Serial.print(ContPlatos[3]);
    Serial.print("E");
    Serial.print(Ganancia);
    Serial.println("G");
}

////////////////////////////////////////////////////////////////////////////////
//////////                         FUNCIONES                ////////////////////
////////////////////////////////////////////////////////////////////////////////
void Agregar_Orden(){
 Firebase.getInt(fbdo,"/Proy_Embebidos/Aux/Mesa");
  order.mesa = fbdo.intData();
  Firebase.getInt(fbdo,"/Proy_Embebidos/Aux/Pedido");
  order.pedido = fbdo.intData();
  j = 0;
  while(j<20){
    if(Cola[order.pedido-1][j] == 0){
      Cola[order.pedido-1][j] = order.mesa;
      Ganancia = Ganancia + precio[order.pedido-1];
      j = 20;
    }
    j++;
  }

  for(int i = 0; i<4; i++){
    j = 0;
    for(int k = 0; k < 20; k++){
      if(Cola[i][k] != 0){
        j++;
      }
    }
    ContPlatos[i] = j;
  }
  lastTime = Time;

  //Mostrar LCD Cola
  lcd.clear();
  for(int i = 0; i<4; i++){
    lcd.setCursor(0,i);
    lcd.print(platos[i]);
    lcd.setCursor(11, i);
    lcd.print(": ");
    
    lcd.setCursor(12, i);
    for(int k = 0; k < 8; k++){
      if(Cola[i][k] != 0){
        lcd.print(Cola[i][k]);
      }
    }
  }

}
void Orden_Despachada(int plato){
  for(int k = 0; k < 20; k++){
    if(Cola[plato-1][k] != 0){
      Cola[plato-1][k] = Cola[plato-1][k+1];
    }
  }
  for(int i = 0; i<4; i++){
    j = 0;
    for(int k = 0; k < 20; k++){
      if(Cola[i][k] != 0){
        j++;
      }
    }
    ContPlatos[i] = j;
  }

  //Mostrar LCD Cola
  lcd.clear();
  for(int i = 0; i<4; i++){
    lcd.setCursor(0,i);
    lcd.print(platos[i]);
    lcd.setCursor(11, i);
    lcd.print(": ");

    lcd.setCursor(12, i);
    for(int k = 0; k < 8; k++){
      if(Cola[i][k] != 0){
        lcd.print(Cola[i][k]);
      }
    }
  }
  //OLED ENTREGADO
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("\n    - ENTREGADO -    ");
  oled.display();
}
void motorOff(){
  //Motor detenido
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}
void motorHorario(){
  //Motor preparado para sentido horario
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}
void motorAntiHorario(){
  //Motor preparado para sentido anti-horario
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}
void IRAM_ATTR onTimer(){
  flag_servo = 1;
}
void IRAM_ATTR IRR1_PULSE(){
  flag_pulse_IRR1 = 1;
}
void IRAM_ATTR IRR2_PULSE(){
  flag_pulse_IRR2 = 1;
}
void IRAM_ATTR IRR3_PULSE(){
  flag_pulse_IRR3 = 1;
}
void IRAM_ATTR IRR4_PULSE(){
  flag_pulse_IRR4 = 1;
}
void IRAM_ATTR IRR5_PULSE(){
  flag_pulse_IRR5 = 1;
}