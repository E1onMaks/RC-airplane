//--------------------- НАСТРОЙКИ ----------------------
#define CH_NUM 0x60   // номер канала (должен совпадать с передатчиком)

// УРОВЕНЬ МОЩНОСТИ ПЕРЕДАТЧИКА
// На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
#define SIG_POWER RF24_PA_MAX

// СКОРОСТЬ ОБМЕНА
// На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
// должна быть одинакова на приёмнике и передатчике!
// при самой низкой скорости имеем самую высокую чувствительность и дальность!!
// ВНИМАНИЕ!!! enableAckPayload НЕ РАБОТАЕТ НА СКОРОСТИ 250 kbps!
#define SIG_SPEED RF24_1MBPS

// ПИНЫ
#define motorPin A5
#define eleronPin A6
#define elevatorPin A7
#define buzzerPin 2
#define voltPin A3

#define RATE 65

// ДАТЧИК НАПРЯЖЕНИЯ
#define VREF 4.89       // точное напряжение на пине 5V
#define DIV_R1 30000.0  // точное значение резистора R1
#define DIV_R2 7500.0   // точное значение резистора R2
//--------------------- НАСТРОЙКИ ----------------------

//--------------------- БИБЛИОТЕКИ ----------------------
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"         // ещё библиотека радиомодуля
#include <EncButton.h>
#include "pitches.h"

EncButton<EB_TICK, 4> btn;
RF24 radio(9, 10);    // "создать" модуль на пинах 9 и 10 Для Уно
//--------------------- БИБЛИОТЕКИ ----------------------

//--------------------- ПЕРЕМЕННЫЕ ----------------------
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};   //возможные номера труб
bool motor_switch = false, flag = 0;
uint16_t tmr;
float vinC, vinP;

int melody_On[] = {NOTE_DS5, NOTE_GS5, NOTE_C6};
int noteDur_On[] = {8, 8, 8};
int melody_Contr[] = {NOTE_C6, NOTE_C6};
int noteDur_Contr[] = {10, 10};
int melody_Plane[] = {NOTE_C7, NOTE_C7, NOTE_C7};
int noteDur_Plane[] = {10, 10, 10};

struct{
  int motor;
  byte eleron;
  byte elevator;
} trans_data;
//--------------------- ПЕРЕМЕННЫЕ ----------------------

void setup() {
  Serial.begin(9600);
  radioSetup();
  melody(melody_On, noteDur_On, 3);
} 

void loop() {
  uint16_t timeLeft = millis() - tmr;
  if (timeLeft >= 8192) {
    tmr += 8192 * (timeLeft / 8192);
    vinC = voltSensor();
    if (vinC <= 7 && flag == 1) {
      melody(melody_Contr, noteDur_Contr, 2);
    }
    if (vinP <= 7 && flag == 0) {
      melody(melody_Plane, noteDur_Plane, 3);
    };
    flag = !flag;
  }
  Serial.print("Кнопка: ");  Serial.print(motor_switch);
  Serial.print(";\tМотор: ");  Serial.print(trans_data.motor);
  Serial.print(";\tПоворот: ");  Serial.print(trans_data.eleron);
  Serial.print(";\tВысота: ");  Serial.print(trans_data.elevator);
  Serial.print(";\tПульт V = "); Serial.print(vinC, 2);
  Serial.print(";\tСамолёт V = "); Serial.println(vinP, 2);
  delay(100);
}

void yield(){
  btn.tick();
  if (btn.press()) {
    motor_switch = !motor_switch;
    if (motor_switch) {
      tone(buzzerPin, NOTE_C7, 1000/8);
    }
    else {
      tone(buzzerPin, NOTE_C6, 1000/8);
    }
  }
  trans_data.eleron = map(analogRead(eleronPin), 0, 1023, (1.8 * (RATE + ((100 - RATE) / 2))), ((100 - RATE) / 2 * 1.8));  // элероны 
  trans_data.elevator = map(analogRead(elevatorPin), 0, 1023, 55, 125);  // руль высоты
  if (motor_switch) {
    trans_data.motor = map(analogRead(motorPin), 0, 1023, 1000, 2000);   // мотор
  }
  else {
    trans_data.motor = 1000;
  }

  if (radio.write(&trans_data, sizeof(trans_data))) {
    if (radio.available()) {              // если получаем ответ
      while (radio.available() ) {        // пока в ответе что-то есть
        radio.read(&vinP, sizeof(vinP));  // читаем
      }
    } else vinP = 0;
  }
}

float voltSensor() {
  return (float)(analogRead(voltPin) * VREF * ((DIV_R1 + DIV_R2) / DIV_R2) / 1024.0);
}

void melody(int melody[], int noteDurations[], int num){
  // выполняем перебор нот в мелодии:
  for (int thisNote = 0; thisNote < num; thisNote++) {

    // чтобы рассчитать время продолжительности ноты, 
    // берем одну секунду и делим ее цифру, соответствующую нужному типу ноты:
    // например, четвертная нота – это 1000/4,
    // а восьмая нота – это 1000/8 и т.д.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote],noteDuration);

    // чтобы выделить ноты, делаем между ними небольшую задержку.
    // в данном примере сделаем ее равной продолжительности ноты
    // плюс еще 30% от продолжительности ноты:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // останавливаем проигрывание мелодии:
    noTone(buzzerPin);
  }
}

void radioSetup() {
  radio.begin();                      // активировать модуль
  radio.setAutoAck(1);                // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);            // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();           // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);           // размер пакета, в байтах
  radio.openWritingPipe(address[0]);  // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(CH_NUM);           // выбираем канал (в котором нет шумов!)
  radio.setPALevel(SIG_POWER);        // уровень мощности передатчика
  radio.setDataRate(SIG_SPEED);       // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    //должна быть одинакова на приёмнике и передатчике!
    //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  radio.powerUp();                    // начать работу
  radio.stopListening();              // не слушаем радиоэфир, мы передатчик
}
