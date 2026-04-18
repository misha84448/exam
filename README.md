Билет №2
# Система мониторинга на базе ESP32 с ЖК-дисплеем и Bluetooth

## Цель работы

Разработать устройство мониторинга на микроконтроллере ESP32, отображающее на символьном ЖК-дисплее бегущую строку «Система мониторинга v1.0» и передающее текущий статус системы по Bluetooth через модуль HC-05 в терминал смартфона.

---

## Используемые компоненты

| Компонент               | Количество | Назначение                          |
|------------------------|------------|-------------------------------------|
| ESP32 Dev Board        | 1          | Главный контроллер                  |
| LCD Keypad Shield      | 1          | Отображение информации и кнопки     |
| Bluetooth модуль HC-05 | 1          | Беспроводная передача данных        |
| Резистор 10 кОм        | 1          | Подтяжка пина GPIO15 (для загрузки) |
| Провода Dupont         | набор      | Соединение компонентов              |
| Макетная плата         | 1         | Монтаж схемы                        |

---

## Схема подключения

### Таблица соединений

| ESP32 Pin | Назначение          | Куда подключено           |
|-----------|---------------------|---------------------------|
| 3.3V      | Питание             | LCD Shield (VCC), HC-05 (VCC) |
| GND       | Земля               | LCD Shield (GND), HC-05 (GND), резистор 10к |
| GPIO15    | LCD RS (D8 шилда)   | D8 шилда + резистор 10к → GND |
| GPIO3     | LCD Enable (D9)     | D9 шилда                  |
| GPIO16    | LCD D4              | D4 шилда                  |
| GPIO17    | LCD D5              | D5 шилда                  |
| GPIO18    | LCD D6              | D6 шилда                  |
| GPIO19    | LCD D7              | D7 шилда                  |
| GPIO36    | Кнопки (A0 шилда)   | A0 шилда                  |
| GPIO1     | Подсветка (D10)     | D10 шилда (опционально)   |
| GPIO16    | UART2 RX (HC-05 TX) | TX модуля HC-05           |
| GPIO17    | UART2 TX (HC-05 RX) | RX модуля HC-05           |

> ⚠️ **Важно**: Между GPIO15 и GND обязательно установлен резистор 10 кОм, иначе ESP32 не загрузится.

### Схема подключения (текстовое описание)


### Фотография собранной установки

*(Вставьте сюда реальное фото вашего устройства)*

---

## Настройка программного обеспечения

### 1. Установка библиотек (Arduino IDE)

- **LiquidCrystal** — встроенная (для работы с LCD Keypad Shield)
- **HardwareSerial** — встроенная (для UART связи с HC-05)

### 2. Конфигурация HC-05

| Параметр        | Значение |
|----------------|----------|
| Скорость UART  | 9600     |
| Режим          | Slave    |
| Имя устройства | HC-05    |
| Пароль         | 1234     |

(Настройки по умолчанию, изменение не требуется)

### 3. Подключение к смартфону

1. Включить Bluetooth на телефоне
2. Найти устройство `HC-05`
3. Ввести пароль `1234`
4. Запустить приложение *Serial Bluetooth Terminal*
5. Подключиться к HC-05

---

## Программный код

Полный код представлен в листинге ниже.

```cpp
#include <LiquidCrystal.h>
#include <HardwareSerial.h>

// Пины для LCD Keypad Shield
LiquidCrystal lcd(15, 3, 16, 17, 18, 19);

// Пин для кнопок
#define BTN_ADC_PIN 36

// Пины UART для HC-05
#define HC05_RX_PIN 16
#define HC05_TX_PIN 17

HardwareSerial hc05Serial(2);

String scrollText = "   Sistema monitoringa v1.0   ";
int scrollPosition = 0;
unsigned long lastScrollTime = 0;
unsigned long lastBTSendTime = 0;
int currentStatusIndex = 0;

int readButtons() {
  int val = analogRead(BTN_ADC_PIN);
  if (val > 4000) return 0; // нет нажатия
  if (val < 500) return 1;  // Right
  if (val < 1100) return 2; // Up
  if (val < 1800) return 3; // Down
  if (val < 2700) return 4; // Left
  if (val < 3500) return 5; // Select
  return 0;
}

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.print("ESP32 System");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  
  hc05Serial.begin(9600, SERIAL_8N1, HC05_RX_PIN, HC05_TX_PIN);
  
  delay(2000);
  lcd.clear();
}

void loop() {
  // Бегущая строка
  if (millis() - lastScrollTime > 350) {
    lastScrollTime = millis();
    String displayText = scrollText.substring(scrollPosition, scrollPosition + 16);
    while (displayText.length() < 16) displayText += " ";
    lcd.setCursor(0, 0);
    lcd.print(displayText);
    scrollPosition++;
    if (scrollPosition > scrollText.length()) scrollPosition = 0;
  }

  // Отправка статуса по Bluetooth
  if (millis() - lastBTSendTime > 5000) {
    lastBTSendTime = millis();
    String statusMsg;
    if (currentStatusIndex % 2 == 0) statusMsg = "System: OK";
    else statusMsg = "Temp: 24.5C  Humidity: 60%";
    currentStatusIndex++;
    
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(statusMsg);
    
    hc05Serial.println("[" + String(millis() / 1000) + "s] " + statusMsg);
    Serial.println("Sent to BT: " + statusMsg);
  }

  // Обработка кнопок
  int btn = readButtons();
  if (btn != 0) {
    String btnName;
    switch(btn) {
      case 1: btnName = "RIGHT"; break;
      case 2: btnName = "UP"; break;
      case 3: btnName = "DOWN"; break;
      case 4: btnName = "LEFT"; break;
      case 5: btnName = "SELECT"; break;
    }
    hc05Serial.println("Button pressed: " + btnName);
    delay(200);
  }

  // Приём команд через Bluetooth
  if (hc05Serial.available()) {
    String cmd = hc05Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      Serial.print("Command from phone: ");
      Serial.println(cmd);
      if (cmd == "STATUS") {
        hc05Serial.println("System running, v1.0");
      } else if (cmd == "HELLO") {
        hc05Serial.println("Hello from ESP32!");
      }
    }
  }
}

# Отчёт: Операции с дисками в виртуальной машине Windows

## Цель работы
Научиться управлять виртуальными дисками в среде Windows: создавать, инициализировать, разбивать на разделы, форматировать в разные файловые системы и настраивать сетевой доступ с разграничением прав.

## Выполненные шаги

### 1. Создание виртуального диска
- **Размер**: 10 ГБ
- **Тип**: динамически расширяемый
- **Инструмент**: оснастка «Управление дисками» (`diskmgmt.msc`)
- **Результат**: в системе появился новый диск без инициализации

### 2. Инициализация диска
- **Стиль разделов**: MBR (Master Boot Record)
- **Состояние после инициализации**: диск готов к созданию томов

### 3. Создание двух разделов по 5 ГБ
| Раздел | Размер | Буква диска |
|--------|--------|--------------|
| Раздел 1 | 5 ГБ (5120 МБ) | G: |
| Раздел 2 | 5 ГБ (оставшееся место) | H: |

*Разделы созданы на нераспределённой области с помощью мастера простых томов.*

### 4. Форматирование разделов
| Раздел | Файловая система | Метка тома | Быстрое форматирование |
|--------|------------------|------------|------------------------|
| G:     | NTFS             | Data_NTFS  | Да                     |
| H:     | FAT32            | Data_FAT32 | Да                     |

### 5. Настройка общей папки на разделе NTFS (G:)
1. **Включение учётной записи Guest**
   - Отключённая учётная запись `Guest` активирована через «Локальные пользователи и группы»
2. **Создание общей папки**
   - Папка: `G:\SharedData`
3. **Настройка сетевого доступа (Sharing)**
   - Добавлен пользователь `Guest`
   - Уровень разрешений: **Чтение и запись**
4. **Настройка разрешений NTFS (Security)**
   - Пользователю `Guest` явно разрешены операции **Чтение** и **Запись**

## Результаты проверки
- ✅ Виртуальный диск 10 ГБ отображается в «Управлении дисками»
- ✅ На диске присутствуют два раздела по 5 ГБ с буквами G: и H:
- ✅ Раздел G: отформатирован в NTFS, раздел H: — в FAT32
- ✅ Папка `G:\SharedData` доступна по сети для пользователя `Guest`
- ✅ Пользователь `Guest` может создавать, изменять и удалять файлы в общей папке

## Использованные инструменты
- `diskmgmt.msc` — управление дисками (графический интерфейс)
- `lusrmgr.msc` — локальные пользователи и группы (активация Guest)
- Проводник Windows — создание папки, проверка прав

## Примечания
- Для применения разрешений может потребоваться перезапуск службы «Сервер» или переподключение сетевой папки.
- Файловая система FAT32 имеет ограничение на размер отдельного файла не более 4 ГБ, что следует учитывать при практическом использовании раздела H:.

## Вывод
Работа выполнена в полном объёме: создан и настроен виртуальный диск, проведено разбиение на разделы, выполнено форматирование в NTFS и FAT32, организован общий доступ к папке на NTFS-разделе с правами «чтение и запись» для гостевого пользователя.
