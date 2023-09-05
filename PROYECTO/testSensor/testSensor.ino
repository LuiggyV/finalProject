#include <ESP32_FastPWM.h>
#include <HCSR04.h>

//PWM EPS32 instancia
ESP32_FAST_PWM * PWM_Conveyor;

//Declaracion de pines
byte trigger = 23;
byte echoS1 = 36;
byte echoS2 = 39;
int PWM_Conveyor_out = 19;

unsigned long frecuencia = 5000;
byte dutyCycle = 50;
byte PWM_Resolution = 12;

//Sensores ultrasonicos
HCSR04 hc(trigger, new int[2]{echoS1, echoS2}, 2);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(trigger, OUTPUT);
  pinMode(echoS1, INPUT);
  pinMode(echoS2, INPUT);
  pinMode(PWM_Conveyor_out, OUTPUT);

  //Instancia de PWM para S1
  PWM_Conveyor = new ESP32_FAST_PWM(PWM_Conveyor_out, frecuencia, dutyCycle, 0, PWM_Resolution);
  if(PWM_Conveyor){
    PWM_Conveyor -> setPWM();
  }
  PWM_Conveyor -> setPWM(PWM_Conveyor_out, frecuencia, dutyCycle);
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  //digitalWrite(IR, HIGH);
  PWM_Conveyor -> setPWM(PWM_Conveyor_out, frecuencia, dutyCycle);

  Serial.print("Sensor 1: ");
  Serial.print(hc.dist(0));
  Serial.print("  Sensor 2: ");
  Serial.println(hc.dist(1));

  delay(60);
  
}
