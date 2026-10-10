# Курсовий проект

Розумний контролер клімату. 
Зчитує поточні температуру, вологість, освітленність (телеметрію) раз на 60 секунд (за замовчуванням),
телеметрію публікує у хмарі, між циклами опитування  переходе у режим низького енегоспоживання DeepSleep.
Поточний стан: LED та інтервал Deep Sleep контролюється тінню пристрою - Device Shadow.
Один параметр Device Shadow,стан LED, може бути змінено з фронтенду та застосований пристроєм після 
виходу з Deep Sleep. Має функцію негайного виходу з Deep Sleep (пробудження) та публікації телеметрії по натисканню кнопки.
Прошивка пристрою може бути оновлена з хмари із використанням OTA.
Дані телеметрії відображаються у Grafana. 



| Тека      | Роль | Технології |
|---|---|---|
| [`HTML/`](HTML/)       | Frontend -інтерфейс: дві кнопки | статичний HTML + `fetch` |
| [`Backend/`](FastAPI/) | бекенд: читає телеметрію, публікує команди | Python, FastAPI, boto3 |
| [`ESP32/`](ESP32/)     | пристрій: публікує телеметрію, слухає команди | C++, PlatformIO, Wokwi |
| [`Grafana/`](JSON)     | JSON файл для імпорту у Grafana й створення dashboard-у `HomeTask6`
---

## Архітектура

```
         ↓ КОМАНДИ (вниз)                                            ↑ ТЕЛЕМЕТРІЯ та ПОДІЇ (вгору)
         ↑ Стан LED (вгору) 
┌───────────────────────────┐                                ┌─────────────────┐
│     Браузер               │  HTML/index.html               │    Браузер:     │
│   [Увімк./Вимк.,Стан LED] |                                │   Grafana або   │
|                           |                                │   Swagger UI    │  Swagger UI: http://localhost:8000/docs
└─▲────────────────┬────────┘                                └────────▲────────┘
  │ GET            │ POST /actuators/led  {"value":"on"} або          │ ← GET /sensors/latest
  │ /actuators/    │  HTTP + CORS,        {"value":"off"}             │ ← GET /sensors/history                                   
  │     ledstate   │  QoS 1                                           │ ← GET /events
  │  HTTP + CORS   │                                                  │ ← GET /health 
  │                │                                                  │ 
┌─┴────────────────▼──────────────────────────────────────────────────┴────────────────────────────────────────────────────┐
│  FastAPI  :8000                                                                                                          │  
│  main.py · iot_client.py · db.py                                                                                         │  
└───▲────────────────────────┬────────────────────────────────▲───────────────────────────────────────▲────────────────────┘
    │boto3                   │ boto3                          │ boto3 query :                         │ boto3 query :
    │iot.get_thing_shadow    │ iot.update_thing_shadow        │ IAM user: iam_user1                   │ IAM user  : iam_user1
	│{"state": {"reported":  | {"state": {"desired":          │ IAM policy: iot_telemetry2_read       │ IAM policy: iot_events_read
    │ "indicator":           |    "indicator":                │                                       │
    │   value: "on"/"off"}}}}│     value: "on"/"off" }}})     │                                       │    
    │                        │                                │                                       │ 
    │ IAM user: iam_user1    │ IAM user: iam_user1            │                                       │ 
    │ IAM policy:            │ IAM policy:                    │                                       │ 
	│  iam_publish_command   │  iam_publish_command           │                                       │
    │                        │                                │                                       │
    │                        │  ┌────────────────┐     ┌──────┴──────────┐                   ┌────────┴────────┐
    │                        │  │ AWS CloudWatch │     │  AWS DynamoDB   │                   │ AWS DynamoDB    │
    │                        │  │ Log group:     │     │                 │                   │                 │
    │                        │  │ iot_telemetry2 │     │                 │                   │                 │
    │                        │  │      _alert    │     │ TABLE:          │                   │ TABLE:          │ 
    │                        │  │                │     │  iot_telemetry2 │                   │  iot_events     │
    │                        │  └──▲─────────────┘     └──────▲──────────┘                   └────────▲────────┘
    │                        │     │ Rules Engine             │                                       │ 
    │                        │     │  rule:                   │  Rules Engine                         │ Rules Engine 
    │                        │     │   rule-iot-telemetry2-   │  rule:  rule-iot-telemetry2           │ rule:   rule-iot-events 
    │                        │     │                  alert   │   Action: DynamoDB                    │  Action: DynamoDB
    │                        │     │ IAM role: role_iot_      │   IAM role: role_iot_telemetry2_add   │  IAM role: iot_events_add
    │                        │     │     telemetry2_alert     │   IAM policy: iot_telemetry2_add      │  IAM policy: ..._iot_events_add 
┌───┴────────────────────────│─────┴──────────────────────────┴───────────────────────────────────────┴──────────────────────────────────────────┐                   ┌─────────────────────────┐ 
│  ┌│────────────────────────▼────┐                                                                                                              │                   │          AWS S3         │
│  │                              │                                                                                                              │ IAM ROLE:role_ota │ bucket:                 │
│  │                              │                   AWS IoT Core                                                                               │ IAM POLICY:ota    │  iot_course_test_bucket │ 
│  │      Device Shadow           │             THINGNAME :esp32_yakymovich                                                                      ◀───────────────────|    - job.json           │
│  │                              │                  POLICY  :my_policy1,                                                                        │                   │    - firmware.bin       │
│  │                              │                      MQTT Broker                                                                             │                   │                         │
│  │                              │                                                                                                              │                   │                         │   
│  └─│────────────────────────▲───┘                                                                                                              │                   └────────┬────────────────┘
└────┬────────────────────────│───────────────────────────▲────────────────────────▲───────────────────────▲────────────────────────┬────────────┘                            │
     │ Subscribe              │ Publish                   │  Publish               │ Publish               │ Publish                │Subscribe                                │    HTTPS://presigned URL 
     │ topics:                │ topic:                    │  topic:                │  topic:               │  topic:                │ topics:                                 │    firmware.bin
     │  "$aws/things/         │   "$aws/things/           │     "iot-course/       │  "iot-course/         │   "$aws/things/jobs/   │  "$aws/things/                          │  
	 │	 esp32_yakymovich     │    esp32_yakymovich/      │      yakymovcih/       │   yakymovich/         │    esp32_yakymovich/   │   esp32_yakymovich/                     │ 
	 │	 shadow/get/accepted",│    shadow/get",           │     telemetry"         │      events"          │    $next/get"          │   /jobs/notify-next",                   │    
     │ "$aws/things/          │   "$aws/things/           │  MQTT over TLS :8883   │  MQTT over TLS :8883  │  MQTT over TLS :8883   │  "$aws/things/                          │ 
     │   esp32_yakymovich/    │    esp32_yakymovich/      │                        │                       │                        │   esp32_yakymovich/                     │ 
     │   shadow/update/       │     shadow/update"        │                        │                       │                        │   jobs/$next/get/                       │
     │   accepted",           │   MQTT over TLS :8883     │                        │                       │                        │    accepted"  : jobs.json               │ 
     │ "$aws/things/          │                           │                        │                       │                        │   MQTT over TLS :8883                   │
     │   esp32_yakymovich/    │                           │                        │                       │                        │                                         │
	 │   shadow/update/       │                           │                        │                       │                        │                                         │
	 │   rejected",           │                           │                        │                       │                        │                                         │
     │   "$aws/things/        │                           │                        │                       │                        │                                         │
	 │	 esp32_yakymovich/    │                           │                        │                       │                        │                                         │ 
	 │	 shadow/update/delta" │                           │                        │                       │                        │                                         │ 
	 │	 MQTT over TLS :8883  │                           │                        │                       │                        │                                         │
	 │                        │                           │                        │                       │                        │                                         │
	 │                        │                           │                        │                       │                        │                                         │
	 │                        │                           │                        │                       │                        │                                         │
┌────▼────────────────────────┴───────────────────────────┴────────────────────────┴───────────────────────┴────────────────────────▼─┐                                       │
│                                                ESP32 (Wokwi), ESP32 DevKit v.1                                                      ◀──────────────────────────────────────┘
│                                                                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```
Топік команд, `iot-course/yakymovich/commands/led`, що на нього підписаний пристрій, не зображено на схемі,
бо він не викоритсовується, залишений для сумісності. Пристрій циклічно переходе у Deep Sleep завдяки чому  для керування 
викоритсовується Device Shadow.

**Чотири незалежні канали в одному брокері.** «Вгору» — пристрій публікує
телеметрію та події, Rules Engine кладе їх в DynamoDB, бекенд читає таблиці. «Вниз» —
бекенд публікує команду в топік, пристрій її забирає. Спільного коду в цих
каналів немає — тільки спільний брокер.

**Ніхто нікого не знає.** Браузер знає адресу бекенда. Бекенд знає назви
топіка й таблиці. Пристрій знає назви топіків. Жодна ланка не знає IP-адреси
наступної — тому ESP32 може сидіти за NAT у симуляторі, а браузер на іншому
континенті.

---

## Топіки й контракти

Все, що склеює три теки — це наступні рядки. Помилка в будь-якому з них ламає
ланцюг мовчки, без жодної помилки в логах.

| Контракт         | Значення                                               | Хто визначає              | Хто споживає                                      
| Топік команд     | `iot-course/yakymovich/commands/led`                   | `Backend/iot_client.py`   | `ESP32/src/mqtt/mqtt.cpp`                  |       
| Топік телеметрії | `iot-course/yakymovich/telemetry2`                     | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB                    |       
| Топіки робіт     | `$aws/things/esp32_yakymovich/jobs/notify-next`        | `ESP32/src/mqtt/mqtt.cpp` |`ESP32/src/mqtt/ota.cpp`                    |
|                  | `$aws/things/esp32_yakymovich/jobs/$next/get`          |                           |                                            |
|				   | `$aws/things/esp32_yakymovich/jobs/$next/get/accepted` |                           |                                            |
|  Топіки shadow   | `$aws/things/esp32_yakymovich/shadow/get`              | `ESP32/src/mqtt/shadow.h` | `ESP32/src/mqtt/shadow.cpp`                |  
|                  | `$aws/things/esp32_yakymovich/shadow/get/accepted`     |                           |                                            |
|				   | `$aws/things/esp32_yakymovich/shadow/update`           |                           |                                            | 
|				   | `$aws/things/esp32_yakymovich/shadow/update/delta`     |                           |                                            | 
|				   | `$aws/things/esp32_yakymovich/shadow/update/accepted`  |                           |                                            |
|				   | `$aws/things/esp32_yakymovich/shadow/update/rejected`  |                           |                                            |
|				   |                                                        |                           |                                            | 
| Топік подій      | `iot-course/yakymovich/events`                         | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB                    |       
| Тіло команди     | `{"action":"set","value":"on"}`                        |                           | `ESP32/src/led/led.cpp`                    |       
| Тіло HTTP-запиту |   `http://localhost:8000/actuators/led`                |  `FastAPI/iot_client.py`, |  `HTML/index.html`                         |
| Тіло команди     | `{"state": {"desired": {"indicator": value}}}`         |  `FastAPI/main.py`        |  `HTML/index.html`                         |        
| Тіло HTTP-запиту |   `http://localhost:8000/actuators/ledstate`           |  `FastAPI/iot_client.py`, |  `HTML/index.html`                         |
| Тіло команди     |  `{"state": {"reported": {"indicator": value}}}`       |  `FastAPI/main.py`        |  `HTML/index.html`                         | 
| Тіло HTTP-запиту |  `http://localhost:8000/sensors/history?hours=24   `   |  `Backend/db.py`          |  Grafana, Dashboard `CourseProject`,       |
|                  |                                                        |                           |   TimeSeries "Температура",                | 
|				   |              									        |						    |    Gauge "Поточна вологість"               | 
| Тіло HTTP-запиту | `http://localhost:8000/events?num=60     `             | `Backend/db.py`           |  Grafana, DashBoard `CourseProject`,       |
|                  |                                                        |                           |   Stat "Останні температура та вологість"  |           
| Тіло HTTP-запиту | `http://localhost:8000/sensors/latest`                 | `Backend/db.py`           |  Grafana, DashBoard `CourseProject`,       |
|                  |                                                        |                           |  Table "Події"                             |
| Тіло HTTP-запиту | `http://localhost:8000/health`                         | `Backend/main.py`         |  Swagger UI                                |
     


Пристрій шукає в команді підрядок `"on"` / `"off"` — разом із лапками, щоб
`"on"` не збігся всередині `"off"`. Поле `action` він зараз ігнорує: воно є
на виріст, коли команд стане більше однієї.
---

## Список створених ресурсів AWS
     
	 
	 Thing: 
         THINGNAME: esp32_yakymovich
         POLICY : my_policy1,
		          my_policy2
	
     IAM user: 
	     USENAME: iam_user1
		      ID: 5016-0254-5268
	   
	   - IAM POLICIES:
	       iot_events_read,
		   iot_telemetry_add, 
		   iot_telemetry2_read,
		   iot_telemetry2_alert,
		   iam_publish_command,
		   ota
     	 
     Rules Engine Rules:
	     RULE: rule_iot_telemetry2
		    SQL statement: 
		     SELECT clientid() as client_id, timestamp() as received_at, 
			        device_id, timestamp as transmitted_at, temperature, 
					humidity, light_level, firmware_version, 
					uptime_sec, free_heap 
			 FROM 'iot-course/yakymovich/telemetry2
			 
		   - IAM_ROLE   : role_iot_telemetry2_add
		   - IAM_POLICY : iot_telemetry2_add 
		   
		  RULE: rule_iot_telemetry2_alert
		   SQL statement
             SELECT clientid() as client_id, timestamp() as received_at, 
			        device_id, timestamp as transmitted_at, temperature, 
					humidity,light_level,firmware_version,
					uptime_sec,free_heap 
			FROM 'iot-course/yakymovich/telemetry2' 
			WHERE temperature < 15 or temperature > 28
		
           - IAM_ROLE   : role_iot_telemetry2_alert
		   - IAM_POLICY : iot_telemetry2_alert


		RULE: rule_iot_events
		   SQL statement:
		    SELECT clientid() as client_id, 
			       timestamp() as received_at, 
				   device_id, 
				   timestamp as transmitted_at,
				   event, 
				   value 
		     FROM 'iot-course/yakymovich/events'   
		   
		   - IAM ROLE   : role_iot_events_add
		   - IAM_POLICY : aws-iot-rule-rule_iot_events-action-1-role-role_iot_events_add  
		   
     DynamoDBv2 Tables:           
		   iot_telemetry2,   
           iot_events		
    
     Cloud Watch Log group:
           iot_telemetry2_alert	 		  
		   

     S3 Bucket:
	       iot-course-test-bucket
		   - IAM ROLE    : role_ota
		   - IAM POLICY  : ota

## Опис до архітектури
    
     ESP-32 циклічно опитує сенсори та публікує телеметрію у топік `iot-course/yakymovich/telemetry2`	   
			між вимірами на 60 секунд переходе у стан DeepSleep.
			У хмарі телеметрія пишеться у таблицю DynamoDBv2 `iot_telemetry2`,			
			події у таблицю `iot_events`. Якщо температура виходе за задані межі це фіксується у CloudWatch
			у LogGroup iot_telemetry_alert. Керування LED виконується за допомогою DeviceShadow, фронтенд
            передає стан LED у розділ "desired".Після виходу пристрою з DeepSleep він ортимує дельту та 
            усуває розбіжності: LED вмикається або вимикається. Так само можна міняти інтервал DeepSleep
            з хмари, редагуванням Device Shadow.   
            По натисканню кнопки виконується негайний вихід з DeepSleep по EXT0 та публікація телеметрії, 
			публікація події "manual trigger" у топік `iot-course/yakymovich/events` після чого 
            пристрій знову переходе  у DeepSleep.	
            За наявності завдання на оновлення прошивки відбувається завантаження нової версії прошивки по HTTPS,
            тестове завантаження пристрою з неї та за успішного підключення нової прошивки до WiFi виконується перехід
            на нову версію.  			
	Backend 		
			Підключається до хмари під користувачем iam_user1, що для нього створені політики
			iot_events_read, iot_telemetry2_read для читання таблиць DynamoDB iot_telemetry2, iot_events2 та
			політика  iam_publish_command для публікації команд через AWS MQTT Broker у топіку 
			`iot-course/yakymovich/commands/led` та для отримання й оновлення стану Shadow.
			У даному проєкті топік `/commands/led` збережений для тестів та для керування не використовується.
	Frontend
	        Для видавання команд використовується HTML сторінка із кнопками,
			командою   POST http://localhost:8000/actuators/led
                                Content-Type: application/json
                              {"state": {"desired": {"indicator": value}}}) ->
							   BOTO3 iot.update_thing_shadow()
            Кнопка "Стан LED" запитує Device Shadow про поточний стан LED з розділу "reported"
                       GET 	http://localhost:8000/actuators/ledstate
                                Content-Type: application/json
                              {"state": {"reported": {"indicator": value}}}) <-
							   BOTO3 iot.get_thing_shadow()		
			Для графічного відображення телеметрії та подій використовується веб-додаток Grafana:
			Infinity DataSource `iot-course-infinity-datasource-1`,
			dashboard `HomeTask6`. Також для відображення у вигляді JSON структур із метою отладки
			може бути використаний вбудований у backend веб-додаток Swagger UI,
			що доступний по адресі http://localhost:8000/docs.
			Команди:
			   GET http://localhost:8000/sensors/history?hours=24 <- DynamoDBv2, table `iot_telemetry2`			       
			       Температура та вологість глибиною по часу 24 години:
			       TimeSeries "Температура" - графік, вісь X - час, вісь Y - температура
				   Gauge "Поточна вологість" - поточне значення вологості
			   GET http://localhost:8000/sensors/latest	<- DynamoDBv2, table `iot_telemetry2`
                   Останні у history (найновіші) виміри дані пристрою 			   
			       Stat "Пристрій"
			   GET http://localhost:8000/events?num=60 <-  DynamoDBv2, table `iot_events`                  			   
			       Події пристрою із глибиною 60 останніх подій
				   Table "Події"
			   GET http://localhost:8000/health  <- main.py 
			   
## Структура проекту

 ── HomeTask6    
    ├── Backend  Fast API, Python - Бекенд
    ├── ESP32    Прошивка ESP32, C++        
    ├── Grafana  JSON, дашбоард HomeTask6 
    ├── HTML 	 HTML + javascript, фронтенд
	├── screenshots скріншоти роботи
   README.md    	

## Скріншоти
    1  Thing esp32_yakymovich
	2  Policy my_policy1
	3  Policy my_policy2
	4  Policy iot_events_read
	5  Policy iot_telemetry2_add
	6  Policy iot_telemetry2_read
	7  Policy iot_telemetry2_alert
	8  Policy iam_publish_command
	9  Policy ota
	10 Rule rule_iot_telemetry2
	11 Rule rule_iot_telemetry2_alert
	12 Rule rule_iot_events
	13 Policy  ...iot_events_add
	14 DynamoDB table iot_telemetry2
	15 DynamoDB table iot_events
	16 CloudWatch log group iot_telemetry2_alert
	17 OTA: S3 bucket iot-course-test-bucket
	18 OTA: Job OTA_14 - оновлення
	19 OTA: Запуск оновлення у Wokwi
	20 OTA: Завантаження пристрою з нової прошивки версії 1.1.0
	21 OTA: Нова прошивка перевірена, Jab OTA_14 - Статус SUCCEDED
	22 OTA: Job OTA_14 - Details
	23 Shadow: State, Delta
	24 Shadow: Запит та отримання документу, зміна стану пристрою, відправлення звіту у хмару
	25 Shadow: Стан після репорту
	26 Shadow: LED світиться відповідно стану
	
   

## Порядок запуску

Ланки залежні: бекенд без AWS не віддасть команду, пристрій без брокера її не
почує. Тому по черзі.

**1. ESP32** (тека `ESP32/`) — Wokwi, чекаємо в Serial Monitor:

```
[MQTT] Підключаємось до AWS IoT Core... OK
[MQTT] Підписані на commands/led
```

**2. FastAPI** (тека `FastAPI/`):

```powershell
.venv\Scripts\activate
fastapi dev main.py          # або: uvicorn main:app --reload
```

`fastapi dev` — це той самий uvicorn з `--reload` під капотом, тільки сам
знаходить об'єкт `app` у файлі й друкує посилання на `/docs`.

**3. HTML** (тека `HTML/`) — відкрити `index.html` подвійним кліком і натиснути
кнопку. 


**4. Grafana: **  встановити плагін Infinity, створити datasource  `iot-course-infinity-datasource-1` 
     створити dashboard HomeTask6 у Dashboards імпортом файлу Grafana/HomeTask6.json,
	 відкрити створений dashboard. Якщо dashboard HomeTask6 вже створений - просто зайти на нього.
	 
---



