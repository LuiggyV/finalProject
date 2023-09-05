/* Finding the key codes for your remote. More info: https://www.makerguides.com */
#include <IRremote.h> // include the IRremote library
#include <ESP32_FastPWM.h>

//PWM EPS32 instancia
ESP32_FAST_PWM * PWM_IRS;

#define RECEIVER_PIN 32 // define the IR receiver pin
IRrecv receiver(RECEIVER_PIN); // create a receiver object of the IRrecv class
decode_results results; // create a results object of the decode_results class

//Declaracion de pine
byte IRS_1 = 15;
byte IRR_1 = 32;

//Declaracion de variables
unsigned int frecuencia = 25;
byte dutyCycle = 92;
byte PWM_Resolution = 16;

void setup() {
  Serial.begin(115200); // begin serial communication with a baud rate of 9600
  receiver.enableIRIn(); // enable the receiver
  //receiver.blink13(true); // enable blinking of the built-in LED when an IR signal is received

  //Instancia de PWM para S1
  PWM_IRS = new ESP32_FAST_PWM(IRS_1, frecuencia, dutyCycle, 5, PWM_Resolution);
  if(PWM_IRS){
    PWM_IRS -> setPWM();
  }
  PWM_IRS -> setPWM(IRS_1, frecuencia, dutyCycle);

}
void loop() {
  if (receiver.decode()) { // decode the received signal and store it in results
    Serial.println("pulso");
    receiver.resume();
  }
}