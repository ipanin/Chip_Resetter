// позволяет взаимодействовать с различными устройствами по интерфейсу I2C / TWI.
#include <I2C.h>

// позволяет управлять микросхемами 24CXX подключать их на ПИН A4 (SDA), A5 (SCL)
// работает с 24C01 24C02 24C04 24C08 24C16
#include <Eeprom24C01_16.h> 

// позволяет перевести в спящий режим ардуино
#include <avr/sleep.h>

// позволяет работать с внутренней памятью Arduino
#include <EEPROM.h>
  
// позволяет управлять различными жидкокристаллическими дисплеями (LCD)
#include <LiquidCrystal.h>  

// для записи статических строк во FLASH, а не в RAM 
// Serial.print(F(Тут_статическая_строка)) или const PROGMEM до вызова SETUP
// Serial.println(pgm_read_byte(&dump_ricoh_sp_150[i]), HEX); чтение переменной без изменений из FLASH
#include <avr/pgmspace.h>

#include "ricoh.h"

#define POWER_PIN A2 // Пин питания у Вас может быть другой

// Пины LCD 1602 (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd( 8, 9, 4, 5, 6, 7 );

// Значение кнопок для разных версий LCD Keypad shield. 
// Настроить под себя если клавиатура плохо работает.
// Указать значение БОЛЬШЕ чем у вас выдает кнопка
                              // LCD Keypad shield v 1        // LCD Keypad shield v 1.1  (тестировал на 2х Keypad shield)   
int BUTTON_UP = 132;          // Пример у меня значение 132   // 96 или 100
int BUTTON_DOWN = 309;        // Пример у меня значение 334   // 251 или 255
int BUTTON_RIGHT = 0;         // Пример у меня значение 3     // 0 или 0
int BUTTON_LEFT = 481;        // Пример у меня значение 482   // 404 или 407
int BUTTON_SELECT = 723;      // Пример у меня значение 720   // 637 или 638

void setup() 
{   
  lcd.begin(16, 2);  // Инициализируем LCD 16x2
  Serial.begin(9600); //инициализируем последовательное соединение для работы с ПК
  while (!Serial) { ; } // Ждем когда подключится ардуино к пк по usb

  //инициализируем библиотеку I2C / TWI для работы с I2C устройствами
  I2c.begin();
  I2c.timeOut(5);
  I2c.pullup(true);

  // Пин А2 для питания чипа устанавливаем в положение OUTPUT
  // Пин A0-14 A1-15 A2-16 A3-17 A4-18 A5-19
  pinMode(POWER_PIN, OUTPUT);
  
  // Очистка EEPROM
  //clear_value_button();
  
  // Если значения кнопок в памяти есть, то считываем их, иначе просим ввести их
  //if (eeprom_is_empty()) { write_value_button(); } else { read_value_button(); } 

  // показываем на экране первый чип 
  display_dump_info(0);
}


void loop() 
{
  // Текущий номер выбранного дампа чипа
  static int selected_dump_index = 0;

  // количество нажатий на кнопку select
  static int selected_program = 0; 
  
  /* ОБРАБОТКА НАЖАТИЯ КНОПОК */

  switch ( button(BUTTON_DOWN) )
  {
    case 0:
      break;
    case 1: // Увеличиваем счетчик и показываем на экране следующий чип
      //Serial.println(F("BUTTON_DOWN SHORT")); 
      if (++selected_dump_index == DATABASE_ROW_COUNT) {
        selected_dump_index = 0;
      }
      display_dump_info(selected_dump_index);
      break;
    case 2:
      Serial.println(F("BUTTON_DOWN LONG")); 
      break;
  }

  switch ( button(BUTTON_UP) )
  {
    case 0:
      break;
    case 1: // Уменьшаем счетчик и показываем предыдущий чип
      //Serial.println(F("BUTTON_UP SHORT")); 
      if (--selected_dump_index < 0) {
        selected_dump_index = DATABASE_ROW_COUNT - 1;
      }
      display_dump_info(selected_dump_index);
      break;
    case 2:
      Serial.println(F("BUTTON_UP LONG")); 
      break;
  }

  switch ( button(BUTTON_RIGHT) )
  {
    case 0:
      break;
    case 1: 
      //Serial.println(F("BUTTON_RIGHT SHORT")); 
      firmware_chip_with_timer(selected_dump_index, 0);
      display_dump_info(selected_dump_index); // Возврат в меню
      break;
    case 2:
      Serial.println(F("BUTTON_RIGHT LONG")); 
      break;
  }

  switch ( button(BUTTON_LEFT) )
  {
    case 0:
      break;
    case 1: 
      //Serial.println(F("BUTTON_LEFT SHORT")); 
      // Считываем чип и показываем его на lcd, и выводи его в порт монитора
      read_chip_and_display_it(CHIP_MEMORY_128);  

      display_dump_info(selected_dump_index); // Возврат в меню
      break;
    case 2:
      Serial.println(F("BUTTON_LEFT LONG")); 
      break;
  }

  switch ( button(BUTTON_SELECT) )
  {
    case 0:
      break;

    case 1: 
      //Serial.println(F("BUTTON_SELECT SHORT"));

      // Если закончились программы то сбрасываем на 1
      if ( ++selected_program > 3 ) { // 3 это кол-во подпрограмм
        selected_program = 1;
      }

      // Показываем подпрограмму
      switch(selected_program)
      {
        case 0:
            break;
        case 1:
          lcd.clear();
          lcd.print(F("Calibration"));
          lcd.setCursor(0,1);
          lcd.print(F("keypad shield"));               
          break;
        case 2:
          lcd.clear();
          lcd.print(F("See total pages")); 
          lcd.setCursor(0,1);
          lcd.print(F("Only SP 111/150"));               
          break;
        case 3:
          lcd.clear();
          lcd.print(F("Firmware chip"));
          lcd.setCursor(0,1);
          lcd.print(F("With timer"));
          break;
      }
      break;

    case 2: // Долгое зажатие приводит к выполнению подпрограммы
      //Serial.println(F("BUTTON_SELECT LONG")); 
      switch(selected_program)
      {
      case 0:
        break;
      case 1:
        calibration_button(); // Калибровка кнопок
        break;
      case 2:
        total_pages_on_display_ricoh(); // Выводим на дисплей количество страниц из чипа (только для Ricoh)
        display_dump_info(selected_dump_index); // Возврат в меню
        break;
      case 3:
        firmware_chip_with_timer(selected_dump_index, 5); // 5 секунд ждать перед пршивкой   
        display_dump_info(selected_dump_index); // Возврат в меню
        break;
      } 
      
      // Сбрасываем меню SELECT
      selected_program = 0;
      break;
  }

  // Переводим в спящий режим ардуино
  time_to_sleep();
}

/********************************************************************************************************/

// Вывод на LCD информацию по чипу
// Показываем на LCD brand, page, pinout, note
void display_dump_info(int index)
{
  char message[17] = "";

  const Struct_DB* row = &Database[index];


  lcd.clear();
  lcd.setCursor(0,0);

  strcpy_P(message, row->brand);
  lcd.print(message);

  lcd.print(" ");

  strcpy_P(message, row->page);
  lcd.print(message);

  strcpy_P(message, row->pinout);
  lcd.setCursor(12,0);
  lcd.print(message);

  strcpy_P(message, row->note);
  lcd.setCursor(0,1);  
  lcd.print(message);
}

// ПОКАЗ ДАМПА НА LCD И МОНИТОРЕ ПОРТА
void read_chip_and_display_it(int sizeof_chip)
{
  byte eeprom_address;

  power_on_chip();
  if (search_chip_address(eeprom_address)) { 
    Eeprom24C01_16 eeprom(eeprom_address);
    eeprom.initialize(); 

    //char c = (char)eeprom.readByte_24C04_16(0); // получил hex to ascii
    const int byte_in_str = 16;
    int num_str_in_chip = sizeof_chip / byte_in_str;

    for(int i = 0; i < num_str_in_chip; i++) {
      lcd.clear();
      lcd.print(F("STRING # "));
      lcd.print(i);
      lcd.setCursor(0,1);
      for (int j = 0; j < byte_in_str; j++) {
        byte b = eeprom.readByte_24C04_16(j + i * byte_in_str);
        
        Serial.print(" 0x");
        if (b <= 16) { 
          lcd.print(0, HEX);
          Serial.print("0");
        }        
        
        lcd.print(b, HEX); 
        Serial.print(b, HEX);
        Serial.print(",");
      }
      Serial.println("");
      delay(500);
    }
  
  //lcd.clear();
  }    

  power_off_chip();
}

// ПРОШИВКА ЧИПА С ТАЙМЕРОМ
void firmware_chip_with_timer(int dump_index, int timer)
{
  // Таймер обратного отсчета
  countdown_timer(timer);
      
  // Подаем питание на чип
  power_on_chip();
  
  //сканируем шину i2c на наличие чипа, если есть ошибка перепрыгиваем на error_i2c_scan
  byte eeprom_address;
  if (search_chip_address(eeprom_address))
  {
    const Struct_DB* row = &Database[dump_index];
    int dump_size = row->dump_size; 
    
    byte dump_data[dump_size];  // Создаем в ОЗУ массив под дамп

    for (int i = 0; i < dump_size; i++) {    
      dump_data[i] = pgm_read_byte(&row->dump[i]); // копируем дамп с flash в ОЗУ
      Serial.print(dump_data[i], HEX);
    }
    Serial.println("");

     //Скоростная прошивка чипа
    firmware(eeprom_address, dump_data, dump_size);
  }
  
  // Выключаем питание
  power_off_chip();
}

// ТАЙМЕР ОБРАТНОГО ОТСЧЕТА
// timer - время в сек, которое ждем перед прошивкой чипа
void countdown_timer(int seconds)
{
  for(int i = seconds; i > 0; i--) {
    lcd.clear(); 
    lcd.print(F("COUNTDOWN TIMER"));
    lcd.setCursor(0,1);
    lcd.print(i); 
    delay(1000); // Задержка в 1 сек
  }
}

void get_dump(int index, const byte* dump_data, int dump_size) {
}

// ВКЛЮЧАЕМ ПИТАНИЯ ЧИПА
void power_on_chip()
{
  digitalWrite(POWER_PIN, HIGH); // Подаем питания на A2 для запитки чипа
  delay(500); // Задержка для поднятия напряжения
}

// ВЫКЛЮЧАЕМ ПИТАНИЯ ЧИПА
void power_off_chip()
{
  digitalWrite(POWER_PIN, LOW); // Выключаем питания на A2 пине
}

// ПОИСК ЧИПА НА ШИНЕ I2C
// Возвращаем true если нашли, false - если все плохо
// Адрес чипа динамический, меняется от чипа к чипу
/*
Фирма Ricoh использует в своих чипах микросхемы EEPROM Serial-I2C: 24c01 и 24c02.
Используется аппаратная адресация с помощью резисторов, запаянных на самом чипе, указывающих на адрес микросхемы EEPROM. 
в 24c08/24c04 пакетная запись по 16 байт, а для 24c01/24c02 - по 8 байт. 
Можно записывать побайтно, после записи байта или пакета нужно выдержать паузу (в зависимости от производителя от 1 мс до 10 мс), поэтому побайтовая запись будет очень медленной.
Резисторами на самом чипе выбирается адрес микросхемы: для желтого картриджа равен 0, для пурпурного картриджа равен 1, 
для голубого картриджа равен 2, для черного картриджа равен 3, 
поэтому при чтении чипа как 24c04 дамп будет размещен с разных адресов: 0x000, 0x100, 0x200, 0x300.
*/
bool search_chip_address(byte& eeprom_address) 
{
    byte error;
    byte count_device = 0;

    Serial.println("Search device");
    // Сканируем шину I2C
    for (byte address = 0; address <= 127; address++) {
      error = I2c._start(); // возвращает 0 если все хорошо, 1 если првышено время ожидания шины, 2 и более другие ошибки
       
      if ( error == 0 ) {           
        // I2c._sendAddress возвращает 0 если все хорошо, 1 если првышено время ожидания шины, 2 и более другие ошибки.
        // Старшие 7 бит - адрес. После адреса надо указывать бит чтения 1 или бит записи 0
        error = I2c._sendAddress((address << 1) + 1);
        if ( error == 0) {
            Serial.print(F("I2C slave found on address ")); Serial.println(address);

            // Берем адрес шины и пытаемся считать данные с чипа по умолчанию
            Eeprom24C01_16 eeprom(address);
            eeprom.initialize();
            
            // Ищем первый адрес с подходящими параметрами, их может быть больше 1 нам нужен лишь 1  
            if (eeprom.readByte_24C01_02(0) != 0) 
            {        
              eeprom_address = address; // Сохраняем адрес чипа
              Serial.println(F("Test read is OK"));
              I2c._stop();
              return true;
            }         
          }
        }
        
      // Отпускаем шину с адресом
      I2c._stop();
    }
    
    // либо плохой контакт или чипа нет
    lcd.clear(); 
    lcd.print(F("BAD CONTACT OR"));  
    lcd.setCursor(0,1); 
    lcd.print(F("NO CHIP"));
    Serial.println(F("BAD CONTACT OR NO CHIP")); 
    delay (2000);
    return false; 
}

// СКОРОСТНАЯ ПРОШИВКА ЧИПОВ
void firmware(byte eeprom_address, byte dump_bytes[], int dump_size)
{
  lcd.clear();
  lcd.print(F("FIRMWARE CHIP")); 
  lcd.setCursor(0, 1);      
  lcd.blink(); // влючаем мигание курсора для информативности
 
  Eeprom24C01_16 eeprom(eeprom_address); 
  eeprom.initialize(); 
 
  // Записываем в чип
  if (dump_size <= 256) {
    eeprom.writeBytes_24C01_02(0, dump_size, dump_bytes); 
  }
  else {
    eeprom.writeBytes_24C04_16(0, dump_size, dump_bytes); 
  }
  
  lcd.print(F("FIRMWARE DONE"));
  lcd.noBlink(); // отключаем мигание курсора
  Serial.println(F("FIRMWARE DONE"));
  
  // Проверка чипа
  verify_dump(eeprom_address, dump_bytes, dump_size);    
}
 

// ПРОВЕРКА ДАМПА ПОСЛЕ ПРОШИВКИ
void verify_dump(byte eeprom_address, byte dump_bytes[], int dump_size)
{
  lcd.clear();
  lcd.print(F("VERIFICATION")); 
  lcd.setCursor(0,1);
  Serial.print(F("VERIFICATION "));
    
  Eeprom24C01_16 eeprom(eeprom_address);
  eeprom.initialize();  

  Serial.print("eeprom_address = ");
  Serial.println(eeprom_address, HEX);
  
  // Кол-во ошибок
  byte error = 0;
  for(int i = 0; i < dump_size; i++) {
    byte b = eeprom.readByte_24C04_16(i); // 24C01_02 ?
    if(b != dump_bytes[i]) {
      error++;
    }
    
    Serial.print(F("number byte = "));
    Serial.print(i);
    Serial.print(F(" PROGMEM = "));
    Serial.print(dump_bytes[i], HEX);
    Serial.print(F(" EEPROM = "));
    Serial.println(b, HEX);
  }

  // Если ошибок нет то GOOD иначе ERROR
  if ( error == 0 ) { 
    lcd.print(F("GOOD")); 
    Serial.println(F("GOOD")); 
    delay(1000); 
  } 
  else { 
    lcd.print(F("ERROR")); 
    Serial.println(F("ERROR"));
    delay(2000);
  }
}

// ПОКАЗ ОТПЕЧАТАННЫХ СТРАНИЦ ДЛЯ RICOH
void total_pages_on_display_ricoh()
{
  power_on_chip();
  
  //сканируем шину i2c на наличие чипа
  byte eeprom_address;
  if (search_chip_address(eeprom_address)) {
    Eeprom24C01_16 eeprom(eeprom_address);
    eeprom.initialize(); 
  
    lcd.clear();
    lcd.print(F("TOTAL PAGE"));
    lcd.setCursor(0,1);
      
    byte HigherByte = eeprom.readByte_24C01_02(65); // Считываем 65 байт это старшый разряд
    byte LowerByte = eeprom.readByte_24C01_02(64); // Считываем 64 байт это младший разряд
    int Result = (HigherByte << 8) | LowerByte; // соединяем 2 разряда в одно
    
    lcd.print(Result); // показываем число на экран
    
    delay(5000); // 5 секунд  
  }
  
  // Выключаем питание
  power_off_chip();
}

/*** Работа с кнопками. ЗАПИСЬ ЧТЕНИЕ ОБНУЛЕНИЕ КАЛИБРОВКА В EEPROM КНОПОК LCD KEYPAD SHIELD ***/

// Считываем из EEPROM
void read_value_button()
{
    BUTTON_UP     = word( EEPROM.read(0), EEPROM.read(1) ); Serial.print(F("UP => "));     Serial.println(BUTTON_UP);
    BUTTON_DOWN   = word( EEPROM.read(2), EEPROM.read(3) ); Serial.print(F("DOWN => "));   Serial.println(BUTTON_DOWN);
    BUTTON_RIGHT  = word( EEPROM.read(4), EEPROM.read(5) ); Serial.print(F("RIGHT => "));  Serial.println(BUTTON_RIGHT);
    BUTTON_LEFT   = word( EEPROM.read(6), EEPROM.read(7) ); Serial.print(F("LEFT => "));   Serial.println(BUTTON_LEFT);
    BUTTON_SELECT = word( EEPROM.read(8), EEPROM.read(9) ); Serial.print(F("SELECT => ")); Serial.println(BUTTON_SELECT);
}

// записываем в EEPROM
void write_value_button()
{
  for (int i = 0; i < 5; i++) {
    lcd.clear();
    lcd.print(F("PRESS"));
    lcd.setCursor(0,1);
    
    switch(i) {
      case 0:
        lcd.print(F("BUTTON UP"));
        break;
      case 1:
        lcd.print(F("BUTTON DOWN"));
        break;
      case 2:
        lcd.print(F("BUTTON RIGHT"));
        break;
      case 3:
        lcd.print(F("BUTTON LEFT"));
        break;
      case 4:
        lcd.print(F("BUTTON SELECT"));
        break;
    } 
    
    // Считываем значение каждой кнопки 5 раз
    int value;
    for(int k = 0; k < 5; k++) {
      delay(1000);
      value = analogRead(0); // Считываем значение кнопки
      Serial.println(value);
    }

    EEPROM.write(i*2, highByte(value)); 
    EEPROM.write(i*2+1, lowByte(value));

    lcd.setCursor(0,0);
    lcd.print(F("RELEASE"));
    delay(3000);

    // обнуляем счетчик сна
    reset_time_to_sleep();
  }
}

// Очищаем EEPROM
void clear_value_button()
{
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 255);
  }
}

// Проверка EEPROM на чистоту -- true eeprom чистая
bool eeprom_is_empty()
{
  for (int i = 0; i < 10; i++) {
    if (0xff != EEPROM.read(i))
      return false;
  }
  return true;
}

// Калибровка кнопок
void calibration_button()
{
   // Очищаем EEPROM
   clear_value_button();
   
   // Запускаем ввод значений
   write_value_button();

   // Говорим что нужно перезапустить arduino
   lcd.clear();
   lcd.print(F("PRESS BUTTON"));
   lcd.setCursor(0,1);
   lcd.print(F("RESET"));
}

// ЗАЩИТА ОТ ДРЕБЕЗГА КНОПОК И ПРОВЕКА СОСТОЯНИЯ КНОПКИ
// Возвращаем true - кнопка нажата, false - не нажата
bool button_pressed(int RESISTOR_BUTTON)
{
  // Параметры резистора с допусками +-20
  int RESISTOR_BUTTON_MIN = RESISTOR_BUTTON - 20;
  int RESISTOR_BUTTON_MAX = RESISTOR_BUTTON + 20; 

   // Сопротивление кнопки которую нажали
  int RESISTOR_NOW = analogRead(0); 
  if ( RESISTOR_NOW > RESISTOR_BUTTON_MIN && RESISTOR_NOW < RESISTOR_BUTTON_MAX ) {
    // Защита от дребезга кнопки
    delay(50);
    RESISTOR_NOW = analogRead(0); 
    if ( RESISTOR_NOW > RESISTOR_BUTTON_MIN && RESISTOR_NOW < RESISTOR_BUTTON_MAX ) {   
      // Если нажали любую кнопку то обнуляем счетчик сна
      reset_time_to_sleep();
      // Кнопка нажата
      return true;
    }
  }
  // Был дребезг кнопки или кнопка отпущена
  return false; 
}

// ПРОВЕРКА НА КОРОТКОЕ ИЛИ ДОЛГОЕ НАЖАТИЕ НА КНОПКУ
// Возвращаем 1 - SHORT click, 2 - LONG click
int button(int RESISTOR_BUTTON)
{  
  // Кнопка была нажата или нет
  static bool BUTTON_BUSY = false;

  if (!button_pressed(RESISTOR_BUTTON)) {
    return 0;
  }

  // Запускаем таймер
  unsigned long time_push_button = millis();
  // Время срабатывание Long click
  int time_delay = 2000;

  while ( (millis() - time_push_button) < time_delay ) {
    // Если кнопку отпустили 
    if(!button_pressed(RESISTOR_BUTTON)) {
      if (!BUTTON_BUSY) { // и до этого не нажимали 
        return 1;
      }
      else { // Если кнопку отпустили и до этого нажимали
        BUTTON_BUSY = false; 
        return 0;
      }
    }
  } 

  // Если кнопку не отпускали дольше чем time_delay, то это Long click
  BUTTON_BUSY = true;
  return 2;
}

// ПРОВЕРКА ЗНАЧЕНИЙ КНОПОК
void print_sensor_value(String name_button)
{
  Serial.print(F("\nValue of button "));
  Serial.print(name_button);
  Serial.print(F(":"));
  Serial.println(analogRead(0)); // Проверка кнопок, какие они выдают значения.
}

/*************************************************************************************/

// время последнего нажатия любой кнопки кроме reset
unsigned long global_timer_to_sleep = 0;

// ОТПРАВЛЯЕТ В СОН АРДУИНО, если пришло время
// Не забудьте выпаять светодиоды 
void time_to_sleep()
{
  // Время до сна 120 сек
  unsigned long timer = 120000;
  
  // Секундомер
  unsigned long stopwatch = millis() - global_timer_to_sleep; 

  if ( stopwatch > timer) {
    //Отключаем дисплей
    lcd.noDisplay();  
    delay(500);
    
    //Отключаем подсветку
    pinMode(10,OUTPUT);
    
    // Запрещаем просыпаться от всего кроме как по кнопке reset 
    noInterrupts();
    
    // Включаем глубокий сон
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();  
  }
}

// обнуление счетчика сна происходит при нажатии на любую кнопку (в функции button_on)
void reset_time_to_sleep()
{
  global_timer_to_sleep = millis();
}
