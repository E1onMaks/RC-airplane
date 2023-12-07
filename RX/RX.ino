//--------------------- БИБЛИОТЕКИ ----------------------
#include "nRF24L01.h"   // библиотека радиомодуля
#include "RF24.h"       // ещё библиотека радиомодуля
#include "Servo.h"

RF24 radio(9, 10);   // "создать" модуль на пинах 9 и 10 для НАНО/УНО
//--------------------- БИБЛИОТЕКИ ----------------------

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
#define MOTOR_PIN 2
#define ELERON_PIN 3
#define ELEVATOR_PIN 4
#define VOLTAGE_PIN A0

// ДАТЧИК НАПРЯЖЕНИЯ
#define VREF 5.25       // точное напряжение на пине
#define DIV_R1 30000.0  // точное значение резистора R1
#define DIV_R2 7500.0   // точное значение резистора R2
//--------------------- НАСТРОЙКИ ----------------------

//--------------------- ПЕРЕМЕННЫЕ ----------------------
byte pipeNo;
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; // возможные номера труб
float telemetry;       // массив данных телеметрии (то что шлём на передатчик)

Servo motor;
Servo eleron;
Servo elevator;

struct {
  int motor;
  byte eleron;
  byte elevator;
} recieved_data;
//--------------------- ПЕРЕМЕННЫЕ ----------------------

void setup() {   
  Serial.begin(9600);
  radioSetup();
  motor.attach(MOTOR_PIN);
  motor.writeMicroseconds(2000);
  delay(2500);
  motor.writeMicroseconds(1000);
  delay(2500);
  eleron.attach(ELERON_PIN);
  elevator.attach(ELEVATOR_PIN);
}

void loop() {
  while (radio.available(&pipeNo)) {                    // слушаем эфир
    radio.read(&recieved_data, sizeof(recieved_data));  // чиатем входящий сигнал
    motor.write(recieved_data.motor);
    eleron.write(recieved_data.eleron);
    elevator.write(recieved_data.elevator);
    
    

    // формируем пакет данных телеметрии (напряжение АКБ, скорость, температура...)
    telemetry = (float)analogRead(VOLTAGE_PIN) * VREF * ((DIV_R1 + DIV_R2) / DIV_R2) / 1024.0;
    

    // отправляем пакет телеметрии
    radio.writeAckPayload(pipeNo, &telemetry, sizeof(telemetry));
  }
  Serial.print("motor: ");
    Serial.print(recieved_data.motor);
    Serial.print("\t eleron: ");
    Serial.print(recieved_data.eleron);
    Serial.print("\t elevaror: ");
    Serial.print(recieved_data.elevator);
    Serial.print("\t battery voltage: ");
    Serial.println(telemetry);
}

void radioSetup() {                     // настройка радио
  radio.begin();                        // активировать модуль
  radio.setAutoAck(1);                  // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);              // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();             // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);             // размер пакета, байт
  radio.openReadingPipe(1, address[0]); // хотим слушать трубу 0
  radio.setChannel(CH_NUM);             // выбираем канал (в котором нет шумов!)
  radio.setPALevel(SIG_POWER);          // уровень мощности передатчика
  radio.setDataRate(SIG_SPEED);         // скорость обмена
    // должна быть одинакова на приёмнике и передатчике!
    // при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  radio.powerUp();                      // начать работу
  radio.startListening();               // начинаем слушать эфир, мы приёмный модуль
}
