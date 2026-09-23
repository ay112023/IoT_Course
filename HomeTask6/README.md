# Лекція 14 — Двостороння комунікація: від кнопки в браузері до світлодіода

До цього заняття дані рухались тільки в один бік: пристрій → хмара → бекенд.
Тепер замикаємо коло. Кнопка в браузері вмикає світлодіод на ESP32 — через
FastAPI і AWS IoT Core, без жодного прямого з'єднання між браузером і
пристроєм.

Три теки — три ланки одного ланцюга. Кожна запускається окремо, кожна має свій
README з деталями.

| Тека | Роль | Технології |
|---|---|---|
| [`HTML/`](HTML/) | інтерфейс: дві кнопки | статичний HTML + `fetch` |
| [`FastAPI/`](FastAPI/) | бекенд: читає телеметрію, публікує команди | Python, FastAPI, boto3 |
| [`ESP32/`](ESP32/) | пристрій: публікує телеметрію, слухає команди | C++, PlatformIO, Wokwi |

---

## Архітектура

```
                        ↓ КОМАНДИ (вниз)                ↑ ТЕЛЕМЕТРІЯ (вгору)

┌─────────────────┐
│  Браузер        │  HTML/index.html
│  [Увімкнути]    │
└────────┬────────┘
         │ POST /actuators/led  {"value":"on"}
         │ HTTP + CORS
         ▼
┌─────────────────────────────────────────┐
│  FastAPI  :8000                         │  ← GET /sensors/latest
│  main.py · iot_client.py · db.py        │  ← GET /sensors/history
└────────┬───────────────────────▲────────┘
         │ boto3 iot-data        │ boto3 query
         │ publish  QoS 1        │
         ▼                       │
┌─────────────────┐     ┌────────┴────────┐
│  AWS IoT Core   │     │  DynamoDB       │
│                 │     │  iot_telemetry  │
└────────┬────────┘     └────────▲────────┘
         │                       │
         │ topic:                │ Rules Engine
         │ .../commands/led      │ (Заняття 11)
         │                       │
         │ MQTT over TLS :8883   │ topic: .../telemetry
         ▼                       │
┌─────────────────────────────────┴───────┐
│  ESP32 (Wokwi)                          │
│  subscribe → LED D2   publish → 10 сек  │
└─────────────────────────────────────────┘
```

**Два незалежні канали в одному брокері.** «Вгору» — пристрій публікує
телеметрію, Rules Engine кладе її в DynamoDB, бекенд читає таблицю. «Вниз» —
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

| Контракт | Значення | Хто визначає | Хто споживає |
|---|---|---|---|
| Топік команд | `iot-course/demo/commands/led` | `FastAPI/iot_client.py` | `ESP32/src/mqtt/mqtt.cpp` |
| Топік телеметрії | `iot-course/demo/telemetry` | `ESP32/src/mqtt/mqtt.cpp` | Rules Engine → DynamoDB |
| Тіло команди | `{"action":"set","value":"on"}` | `FastAPI/iot_client.py` | `ESP32/src/led/led.cpp` |
| Тіло HTTP-запиту | `{"value":"on"\|"off"}` | `HTML/index.html` | `FastAPI/main.py` |

Пристрій шукає в команді підрядок `"on"` / `"off"` — разом із лапками, щоб
`"on"` не збігся всередині `"off"`. Поле `action` він зараз ігнорує: воно є
на виріст, коли команд стане більше однієї.

---

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
