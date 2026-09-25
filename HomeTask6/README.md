# Домашнє завдання №6


Демонстрація зібрки повного IoT-стеку: від пристрою до інтерфейсу керування та побудування API поверх хмарних даних, 
візуалізація потоків в реальному часі та замикання  петлі — відправки команди назад на пристрій через той самий стек


Три теки — три ланки одного ланцюга. Кожна запускається окремо, кожна має свій
README з деталями.

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

Все, що склеює три теки — це чотири рядки. Помилка в будь-якому з них ламає
ланцюг мовчки, без жодної помилки в логах.

| Контракт         | Значення                                          | Хто визначає              | Хто споживає                                      |
| Топік команд     | `iot-course/yakymovich/commands/led`              | `Backend/iot_client.py`   | `ESP32/src/mqtt/mqtt.cpp`                  |       |
| Топік телеметрії | `iot-course/yakymovich/telemetry`                 | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB                    |       |
| Топік подій      | `iot-course/yakymovich/events`                    | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB                    |       |
| Тіло команди     | `{"action":"set","value":"on"}`                   | `FastAPI/iot_client.py`   | `ESP32/src/led/led.cpp`                    |       |
| Тіло HTTP-запиту | `{"value":"on"\|"off"}`                           | `HTML/index.html`         | `FastAPI/main.py`                          |       | 
| Тіло HTTP-запиту | `http://localhost:8000/sensors/history?minutes=60`| `Backend/db.py`           |  Grafana, Dashboard `HomeTask6`,           |
|                  |                                                   |                           |   TimeSeries "Температура",                | 
|				   |              									   |						   |      Gauge "Поточна вологість"             | 
| Тіло HTTP-запиту | `http://localhost:8000/events?minutes=60`         | `Backend/db.py`           |  Grafana, DashBoard `HomeTask6`,           |
|                  |                                                   |                           |   Stat "Останні температура та вологість"  |           | 
| Тіло HTTP-запиту | `http://localhost:8000/sensors/latest`            | `Backend/db.py`           |  Grafana, DashBoard `HomeTask6`,           |
|                                                                      |                           |   Table "Події"                            |

Пристрій шукає в команді підрядок `"on"` / `"off"` — разом із лапками, щоб
`"on"` не збігся всередині `"off"`. Поле `action` він зараз ігнорує: воно є
на виріст, коли команд стане більше однієї.

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
		   iot_telemetry_read;
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

---

## Де що ламається

Ланцюг довгий, і кожна ланка мовчить по-своєму. Головне правило: **не шукати
причину там, де побачив симптом.**

| Симптом | Ланка-винуватець | Деталі |
|---|---|---|
| Кнопка → «Помилка мережі» | бекенд не запущений | [`HTML/`](HTML/README.md) |
| «CORS policy» у консолі, `200` у логах uvicorn | бекенд: `CORSMiddleware` | [`FastAPI/`](FastAPI/README.md) |
| `422` у відповіді | браузер: не те тіло запиту | [`HTML/`](HTML/README.md) |
| `502` у відповіді | бекенд: немає доступу до AWS IoT | [`FastAPI/`](FastAPI/README.md) |
| `202`, але LED мовчить | пристрій: підписка, Policy або TLS | [`ESP32/`](ESP32/README.md) |
| Телеметрії немає в `/sensors/latest` | пристрій або Rules Engine | Заняття 11 |

**Точка розриву посередині — MQTT test client** в AWS IoT Console. Підпишіться
на `iot-course/demo/commands/led` і натисніть кнопку. Побачили JSON — винні
браузер або бекенд уже ні в чому, шукайте в пристрої. Не побачили — далі
пристрою можна не йти.

---

## Секрети

Дві теки мають файли, які **до git не потрапляють**:

| Файл | Тека | Шаблон |
|---|---|---|
| `.env` | `FastAPI/` | `.env.example` |
| `src/secrets.h` | `ESP32/` | `src/secrets.example.h` |

Обидва — у `.gitignore`. Ключ, що потрапив у git, вважається скомпрометованим,
навіть якщо його звідти видалили наступним комітом.

**Дозволи IAM для бекенда** — рівно два, кожен на конкретний ресурс:
`dynamodb:Query` на таблицю `iot_telemetry` і `iot:Publish` на ARN топіка
команд. Не `AdministratorAccess` «щоб працювало».

---

## Що з чого виросло

| Заняття | Що додало |
|---|---|
| 9 | Thing, сертифікати, Policy |
| 10 | MQTT over TLS з ESP32 → IoT Core |
| 11 | Rules Engine → DynamoDB |
| 12 | FastAPI: читання таблиці (`GET /sensors/*`) |
| 13 | Grafana поверх тих самих даних |
| **14** | **зворотний канал: `POST /actuators/led` → топік → пристрій** |

Кожна ланка додавалась окремо і жодного разу не переписувалась — це і є
розв'язана (decoupled) архітектура на практиці.

---

## Ключові факти заняття

- Браузер і пристрій ніколи не бачать одне одного. Між ними два посередники,
  і кожен можна замінити окремо
- `202 Accepted` — чесна відповідь на команду: прийняв і передав далі,
  за виконання не ручаюсь
- Два канали, два QoS: команда — QoS 1 (повторити нікому), телеметрія — QoS 0
  (наступний пакет перекриє)
- Три ланки — три різні протоколи: HTTP, boto3/HTTPS, MQTT over TLS
- Спільне в них — не код, а чотири рядки контрактів: два топіки і два JSON
- **Пристрій дає дані — хмара дає гарантії — бекенд дає доступ і керування**

---

## Домашнє завдання

Демо в цих трьох теках покриває більшу частину ДЗ, але **не все**: топіки тут
`iot-course/demo/...`, а у вас має бути своє ім'я замість `demo`. І головного —
підтвердження від пристрою (`events`) — у демо немає взагалі. Це ваша робота.

### Частина 1: Backend API

Створити FastAPI з ендпоінтами:

| Метод | Шлях | Що повертає |
|---|---|---|
| GET | `/sensors/latest` | останні дані з DynamoDB |
| GET | `/sensors/history?minutes=30` | історія за N хвилин |
| POST | `/actuators/led` | команда на ESP32 через AWS IoT |

Додати `iot:Publish` до IAM-політики користувача, під яким працює бекенд — без
цього публікація поверне `ForbiddenException`.

Запустити локально й протестувати всі три через `/docs`, Postman або curl.

### Частина 2: Grafana Dashboard

Підключити Grafana до FastAPI через **Infinity datasource** і зібрати дашборд
із трьох панелей:

| Панель | Дані |
|---|---|
| Time series | температура за останні 30 хвилин |
| Gauge | поточна вологість |
| Stat | останній timestamp |

Експортувати дашборд у JSON (Dashboard settings → JSON Model) і покласти в
репозиторій.

### Частина 3: Зворотний зв'язок

1. ESP32 підписується на топік `iot-course/<name>/commands/led`.
2. Через FastAPI відправити команду `{"action": "set", "value": "on"}` — LED
   має засвітитись.
3. Створити HTML-сторінку з кнопками «увімкнути» / «вимкнути», яка викликає
   ендпоінт FastAPI. (У Grafana OSS кнопки з POST немає без додаткових плагінів
   — тому окрема сторінка.)
4. Після виконання команди ESP32 публікує підтвердження в топік
   `iot-course/<name>/events` у форматі `{"event": "led_changed", "value": "on"}`.

**Опційно (на плюс):** IoT Rule, що складає події в окрему таблицю DynamoDB, і
панель Table в Grafana з останніми подіями.

### Здача

Репозиторій із прошивкою ESP32, бекендом, HTML-сторінкою, JSON-дашбордом і
README. У README:

- архітектурна діаграма (Device → AWS IoT → DynamoDB → FastAPI → Grafana) з
  **обома** напрямками руху даних;
- список створених ресурсів AWS: Thing, Policy, Rules, таблиці;
- інструкція запуску: змінні середовища, команди, порти.

### Критерії

- FastAPI працює, ендпоінти повертають реальні дані з DynamoDB
- Grafana відображає актуальні дані з автооновленням
- Команди з хмари коректно доходять до ESP32 і виконуються
- Пристрій підтверджує виконання команди у власному топіку
- README дозволяє відтворити налаштування з нуля

---

## Додаткові матеріали

### ESP32 / MQTT

- PubSubClient API — [pubsubclient.knolleary.net/api](https://pubsubclient.knolleary.net/api)
- PubSubClient на GitHub — [github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)

### AWS

- MQTT в AWS IoT Core — [docs.aws.amazon.com/iot/…/mqtt.html](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html)
- Дії в IoT Policy — [docs.aws.amazon.com/iot/…/iot-policy-actions.html](https://docs.aws.amazon.com/iot/latest/developerguide/iot-policy-actions.html)
- Приклади Pub/Sub-політик — [docs.aws.amazon.com/iot/…/pub-sub-policy.html](https://docs.aws.amazon.com/iot/latest/developerguide/pub-sub-policy.html)

### Бекенд

- boto3 `iot-data.publish` — [boto3.amazonaws.com/…/iot-data/client/publish](https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/iot-data/client/publish.html)
- CORS у FastAPI — [fastapi.tiangolo.com/tutorial/cors](https://fastapi.tiangolo.com/tutorial/cors/)
- Чому `*` несумісна з credentials — [developer.mozilla.org/…/CORSNotSupportingCredentials](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS/Errors/CORSNotSupportingCredentials)

---
