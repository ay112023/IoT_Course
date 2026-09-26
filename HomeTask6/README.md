# Домашнє завдання №6


Демонстрація зібрки повного IoT-стеку: від пристрою до інтерфейсу керування та побудування API поверх хмарних даних, 
візуалізація потоків в реальному часі та замикання  петлі — відправки команди назад на пристрій через той самий стек


Три теки — три ланки одного ланцюга. Кожна запускається окремо, кожна має свій
README.

| Тека | Роль | Технології |
|---|---|---|
| [`HTML/`](HTML/) | Frontend -інтерфейс: дві кнопки | статичний HTML + `fetch` |
| [`Backend/`](FastAPI/) | бекенд: читає телеметрію, публікує команди | Python, FastAPI, boto3 |
| [`ESP32/`](ESP32/) | пристрій: публікує телеметрію, слухає команди | C++, PlatformIO, Wokwi |

---

## Архітектура

```
         ↓ КОМАНДИ (вниз)                              ↑ ТЕЛЕМЕТРІЯ та ПОДІЇ (вгору)

┌─────────────────┐                            ┌─────────────────┐
│     Браузер     │  HTML/index.html           │    Браузер      │
│  [Увімк./Вимк.] |                            │   [Grafana]     │
|                 |                            │ [Відображення]  │
└────────┬────────┘                            └────────▲────────┘
         │ POST /actuators/led  {"value":"on"}          │ ← GET /sensors/latest
         │ HTTP + CORS          {"value":"off"}         │ ← GET /sensors/history
         │                                              │ ← GET /events 
         │                                              │ ← GET /health
┌────────▼──────────────────────────────────────────────┴────────────────────────────────────┐
│  FastAPI  :8000                                                                            │  
│  main.py · iot_client.py · db.py                                                           │  
└────────┬────────────────────────────▲─────────────────────────────────────▲────────────────┘
         │ boto3 iot-data             │ boto3 query :                       │ boto3 query :
         │ publish  QoS 1             │ IAM user: iam_user1                 │ IAM user  : iam_user1
		 │ topic:                     │ IAM policy: iot_telemetry_read      │ IAM policy: iot_events_read
         │ iot-course/yakymovich/     │                                     │    
         │ commands/led               │                                     │ 
         │ IAM user: iam_user1        │                                     │ 
         │ IAM:iam_publish_command    │                                     │
         │                            │                                     │
         │                   ┌────────┴────────┐                   ┌────────┴────────┐
         │                   │ AWS DynamoDB    │                   │ AWS DynamoDB    │
         │                   │                 │                   │                 │
         │                   │                 │                   │                 │
         │                   │ TABLE:          │                   │ TABLE:          │ 
         │                   │  iot_telemetry  │                   │  iot_events     │
         │                   └────────▲────────┘                   └────────▲────────┘
         │                            │                                     │ 
         │                            │  Rules Engine                       │ Rules Engine 
         │                            │  rule:  rule-iot-telemetry          │ rule:   rule-iot-events
		 │                            │   Action: DynamoDB                  │  Action: DynamoDB
         │                            │   IAM role: iot_telemetry_add       │  IAM role: iot_events_add
         │                            │   IAM policy: ...-iot_telemetry_add │  IAM policy: ..._iot_events_add 
   ┌─────▼────────────────────────────┴─────────────────────────────────────┴─────────────────┐
   │                                                                                          │
   │                                                                                          │
   │                                     AWS IoT Core                                         │
   │                            THINGNAME    :esp32_yakymovich                                │
   │                                 POLICY  :my_policy1,                                     │
   │                                     MQTT Broker                                          │
   │                                                                                          │
   │                                                                                          │
   └─────┬─────────────────────────▲─────────────────────────────────────▲────────────────────┘
         │ Subscribe               │                                     │
         │ topic:                  │                                     │   
         │ iot-course/yakymovich/  │                                     │   
         │ commands/led            │                                     │   
         │                         │ Publish                             │ Publish
         │ MQTT over TLS :8883     │ topic:  iot-course/yakymovcih/      │ topic:  iot-course/yakymovcih/ 
		 │                         │         telemetry                   │          events 
         │                         │ MQTT over TLS :8883                 │ MQTT over TLS :8883
┌────────▼─────────────────────────┴────────┐                            │
│  ESP32 (Wokwi)                            │────────────────────────────┘
│  subscribe → LED D2   publish → 10 сек    │
└───────────────────────────────────────────┘
```

**Два незалежні канали в одному брокері.** «Вгору» — пристрій публікує
телеметрію та події, Rules Engine кладе її в DynamoDB, бекенд читає таблицю. «Вниз» —
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

| Контракт         | Значення                                            | Хто визначає              | Хто споживає                                      
| Топік команд     | `iot-course/yakymovich/commands/led`                | `Backend/iot_client.py`   | `ESP32/src/mqtt/mqtt.cpp`                  |       
| Топік телеметрії | `iot-course/yakymovich/telemetry`                   | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB                    |       
| Топік подій      | `iot-course/yakymovich/events`                      | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB                    |       
| Тіло команди     | `{"action":"set","value":"on"}`                     | `FastAPI/iot_client.py`   | `ESP32/src/led/led.cpp`                    |       
| Тіло HTTP-запиту | `{"value":"on"\|"off"}`                             | `HTML/index.html`         | `FastAPI/main.py`                          |        
| Тіло HTTP-запиту | `http://localhost:8000/sensors/history?minutes=60`  | `Backend/db.py`           |  Grafana, Dashboard `HomeTask6`,           |
|                  |                                                     |                           |   TimeSeries "Температура",                | 
|				   |              									     |						     |    Gauge "Поточна вологість"             | 
| Тіло HTTP-запиту | `http://localhost:8000/events?minutes=200`          | `Backend/db.py`           |  Grafana, DashBoard `HomeTask6`,           |
|                  |                                                     |                           |   Stat "Останні температура та вологість"  |           
| Тіло HTTP-запиту | `http://localhost:8000/sensors/latest`              | `Backend/db.py`           |  Grafana, DashBoard `HomeTask6`,           |
|                  |                                                     |                           |  Table "Події" 
| Тіло HTTP-запиту | `http://localhost:8000/health`                      |  `Backend/db.py`          |                              |

Пристрій шукає в команді підрядок `"on"` / `"off"` — разом із лапками, щоб
`"on"` не збігся всередині `"off"`. Поле `action` він зараз ігнорує: воно є
на виріст, коли команд стане більше однієї.

*Параметр `minutes` у телеметрії та подій за замовчуванням у бекенді == 30, для
 відображення у Grafana взяв minutes=60 для телеметрії та minutes=200 для подій.
---

## Список створених ресурсів AWS
     
	 
	 Thing: 
         THINGNAME: esp32_yakymovich
         POLICY : my_policy1
	
     IAM user: 
	     USENAME: iam_user1
		      ID: 5016-0254-5268
	   
	   - IAM POLICIES:
	       iot_events_read,
		   iot_telemetry_read,
		   iam_publish_command
     	 
     Rules Engine Rules:
	     RULE: rule_iot_telemetry
		    SQL statement: 
		     SELECT  clientid() as client_id, 
			          timestamp() as received_at, 
		     	      device_id, 
					  timestamp as transmitted_at, 
			          topic(2) as student, 
					  temperature, humidity 
			 FROM 'iot-course/yakymovich/telemetry'
			 
		   - IAM_ROLE   : iot_telemetry_add
		   - IAM_POLICY : aws-iot-rule-rule_iot_telemetry-action-1-role-iot_telemetry_add 
		   
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
		   iot_telemetry,   
           iot_events		   



## Опис до архітектури
    
     ESP-32 опитує сенсори раз у 10 секунд та публікує значення параметрів у топік `iot-course/yakymovich/telemetry`
	        для прийому команд з хмари підписується на топік `iot-course/yakymovich/commands/led`та вмикає або вимикає LED 
			у залежності від `value`, після чого публікує статус LED у топік `iot-course/yakymovich/events`
			У хмарі телеметрія пишеться у таблицю DynamoDBv2 `iot_telemetry`,
			події у таблицю `iot_events`. 
	Backend 		
			Підключається до хмари під користувачем iam_user1, що для нього створені політики
			iot_events_read, iot_telemetry_read для читання таблиць DynamoDB iot_telemetry, iot_events та
			політика  iam_publish_command для публікації команд через AWS MQTT Broker у топіку 
			`iot-course/yakymovich/commands/led`. 
	Frontend
	        Для видавання команд використовується HTML сторінка із кнопками,
			командою   POST http://localhost:8000/actuators/led
               │                Content-Type: application/json
                             │  {"value":"on"} або {"value":"off"} ->
							topic `iot-course/yakymovich/commands/led`
			Для відображення використовується Grafana,
			Infinity DataSource iot-course-infinity-datasource-1,
			dashboard "HomeTask6"
			команди:
			   GET http://localhost:8000/sensors/history?minutes=60 <- DynamoDBv2, table `iot_telemetry`			       
			       температура та вологість глибиною по часу 60 хвилин:
			       TimeSeries "Температура" - графік, вісь X - час, вісь Y - температура
				   Gauge "Поточна вологість" - поточне значення вологості
			   GET http://localhost:8000/sensors/latest	<- DynamoDBv2, table `iot_telemetry`
                   Останні у history (найновіші) виміри температури та вологосі 			   
			       Stat "Останні температура та вологість"
			   GET http://127.0.0.1:8000/events?minutes=200 <-  DynamoDBv2, table `iot_events`                  			   
			       Події присторю глибиною по часу 200 хвилин
				   Table "Події"
			   
## Структура проекту

 ── HomeTask6    
    ├── Backend  Fast API, Python - Бекенд
    ├── ESP32    Прошивка ESP32, C++        
    ├── Grafana  JSON, дашбоард HomeTask6 
    ├── HTML 	 HTML + javascript, фронтенд
	├── screenshots скріншоти роботи
   README.md    	

## Скріншоти
    1  Запуск ESP-32, синхронізація, коннект с AWS, підписка на ../commands/led, публікація сенсорів
    2  Відображення вимірів у Grafana Dashboard (Table подій поки що пустий) 
	3  Фронтенд -> видавання команди + робота callback-у ESP32, публікація подій із статусом led
	4  Відображення подій у Grafana, table "Події"
	5  Thing 
	6  Policy my_policy1
    7  IAM user
    8  Policy iam_publish_command
    9  Policy iam_events_read
   10  Policy iot_telemtry_read
   11  Rule rule_iot_telemetry
   12  Rule rule_iot_events
   13  IAM role iot_telemetry_add
   14  IAM policy aws-iot-rule-rule_iot_telemetry-action-1-role-iot_telemetry_add 
   15  IAM role iot_events_add
   16  IAM policy  aws-iot-rule-rule_iot_events-action-1-role-role_iot_events_add  
   17  DynamoDBv2 iot_events
   18  DynamoDbv2 iot_telemetry

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


**4. Grafana->Dashboards->HomeTask6
---



