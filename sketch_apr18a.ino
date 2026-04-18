#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>

// === НАСТРОЙКИ ===
#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define I2C_ADDR 0x27          // Чаще всего 0x27 или 0x3F. Проверьте вашим сканером I2C

// Пины UART для HC-05
#define HC05_RX_PIN 16         // Подключаем к TX HC-05
#define HC05_TX_PIN 17         // Подключаем к RX HC-05

// Текст бегущей строки
String scrollText = "Система мониторинга v1.0   "; // Пробелы в конце для отступа
// =================

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);
HardwareSerial hc05Serial(2);   // Используем UART2

unsigned long lastScrollTime = 0;
int scrollPosition = 0;
unsigned long lastStatusSendTime = 0;
String currentStatus = "OK";    // Переменная для имитации статуса

void setup() {
  // Инициализация USB Serial для отладки
  Serial.begin(115200);
  Serial.println("System starting...");

  // 1. Инициализация LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  
  // 2. Инициализация Bluetooth HC-05
  // Скорость 9600 - стандартная для HC-05 в режиме данных
  // Аргументы: скорость, протокол, RX пин, TX пин
  hc05Serial.begin(9600, SERIAL_8N1, HC05_RX_PIN, HC05_TX_PIN);
  
  // Небольшая задержка для старта модуля
  delay(2000);
  
  lcd.clear();
  Serial.println("Ready. Connecting to BT...");
}

void loop() {
  // 1. ОБНОВЛЕНИЕ БЕГУЩЕЙ СТРОКИ (каждые 300 мс)
  if (millis() - lastScrollTime > 300) {
    lastScrollTime = millis();
    updateScrollingText();
  }

  // 2. ОТПРАВКА СТАТУСА В HC-05 (каждые 5 секунд)
  if (millis() - lastStatusSendTime > 5000) {
    lastStatusSendTime = millis();
    sendStatusOverBluetooth();
    
    // Имитация изменения статуса для демонстрации (можно заменить на чтение датчиков)
    static int counter = 0;
    counter++;
    if (counter % 2 == 0) {
      currentStatus = "System: OK";
    } else {
      currentStatus = "Temp: 24.5C";
    }
  }

  // 3. ПРИЕМ ДАННЫХ ИЗ ТЕРМИНАЛА (Опционально)
  // Если вы отправляете команды с телефона на ESP32, они появятся здесь
  if (hc05Serial.available()) {
    String command = hc05Serial.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      Serial.print("Command from BT: ");
      Serial.println(command);
      // Обработка команд (например, "STATUS" или "RESET")
      if (command == "STATUS") {
        hc05Serial.println("Current status: " + currentStatus);
      }
    }
  }
}

// Функция обновления дисплея (бегущая строка на 1й строке, статус на 2й)
void updateScrollingText() {
  // Создаем строку длиннее, чем ширина экрана для эффекта прокрутки
  String displayString = scrollText + scrollText.substring(0, LCD_COLUMNS);
  
  // Вырезаем текущий кусок
  String line1 = displayString.substring(scrollPosition, scrollPosition + LCD_COLUMNS);
  
  // Выводим на LCD
  lcd.setCursor(0, 0);
  lcd.print(line1);
  
  // Вторая строка - статичный статус
  lcd.setCursor(0, 1);
  lcd.print("Status: ");
  lcd.print(currentStatus);
  // Заполняем пробелами, если строка короче 16 символов, чтобы стереть старый текст
  for (int i = 7 + currentStatus.length(); i < LCD_COLUMNS; i++) {
    lcd.print(" ");
  }
  
  // Увеличиваем позицию, зацикливаем
  scrollPosition++;
  if (scrollPosition > scrollText.length()) {
    scrollPosition = 0;
  }
}

// Функция отправки статуса в терминал телефона
void sendStatusOverBluetooth() {
  // Формируем сообщение
  String message = "[" + String(millis() / 1000) + "s] " + currentStatus;
  
  // Отправляем в HC-05 (на телефон)
  hc05Serial.println(message);
  
  // Дублируем в Serial монитор для контроля
  Serial.print("Sent to BT: ");
  Serial.println(message);
}