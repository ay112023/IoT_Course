# Заняття 12 — FastAPI: читаємо телеметрію з хмари

Продовження Заняття 11. Дані з ESP32 вже лежать у DynamoDB — їх туди кладе
Rules Engine без жодного рядка бекенду. Тепер пишемо цей бекенд: маленький
REST API на FastAPI, який читає таблицю і віддає JSON. Це та сама «труба»,
тільки тепер із краном на кінці.

---

## Архітектура

```
┌──────────────┐
│  ESP32       │  Wokwi
│  (DHT22 sim) │
└──────┬───────┘
       │ MQTT over TLS → AWS IoT Core → Rules Engine (Заняття 10-11)
       ▼
┌───────────────────┐
│  DynamoDB         │
│  iot_telemetry    │
│  pk: device_id    │
│  sk: received_at  │
└──────┬────────────┘
       │ boto3 query (читання)
       ▼
┌───────────────────┐        GET /health
│  FastAPI          │  ←──   GET /sensors/latest
│  uvicorn :8000    │        GET /sensors/history?minutes=30
└───────────────────┘
       │
       ▼
 Заняття 13: Grafana
```

**Ключова ідея:** пристрій і бекенд не знають одне про одного. Пристрій пише
в топік, правило кладе в таблицю, бекенд читає таблицю. Кожна ланка замінюється
окремо — це і є розв'язана (decoupled) архітектура.

---

## Параметри цього проєкту

| Що | Значення |
|---|---|
| Регіон | `eu-north-1` |
| Таблиця DynamoDB | `iot_telemetry` (створена на Занятті 11) |
| `device_id` | `esp32lecture10` |
| Порт API | `8000` (uvicorn за замовчуванням) |

---

## Структура проєкту

```
main.py           — FastAPI-застосунок і ендпоінти
db.py             — робота з DynamoDB (boto3)
requirements.txt  — залежності Python
.env              — AWS-ключі та конфігурація (У .gitignore!)
.env.example      — шаблон .env для копіювання
```

---

## Налаштування .env

`.env` містить AWS-ключі і **до git не потрапляє** (у `.gitignore`).

1. Скопіювати `.env.example` → `.env`.
2. Вписати свої значення:

| Змінна | Що це |
|---|---|
| `AWS_ACCESS_KEY_ID` | ключ IAM-користувача з правом читати таблицю |
| `AWS_SECRET_ACCESS_KEY` | секретна частина ключа |
| `AWS_DEFAULT_REGION` | регіон таблиці — `eu-north-1` |
| `TABLE_NAME` | `iot_telemetry` |
| `DEVICE_ID` | `esp32lecture10` — partition key, за яким робимо query |

Принцип найменших привілеїв — той самий, що на Заняттях 9 і 11: цьому
користувачу достатньо `dynamodb:Query` на одну таблицю. Не давайте йому
AdministratorAccess «щоб працювало».

> `load_dotenv()` у `main.py` викликається **до** `import db` — бо `db.py`
> читає змінні середовища прямо при імпорті. Поміняєте порядок — отримаєте
> підключення до регіону `None`.

---

## Залежності

```
fastapi
uvicorn
boto3
python-dotenv
```

Повний список із версіями — у `requirements.txt`.

---

## Як запустити

```powershell
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
uvicorn main:app --reload
```

Або через FastAPI CLI (той самий uvicorn з `--reload` під капотом):

```powershell
fastapi dev main.py
```

Відкрити http://127.0.0.1:8000/docs — FastAPI сам генерує інтерактивну
документацію (Swagger UI). Кожен ендпоінт можна викликати прямо звідти.

---

## Ендпоінти

| Метод | Шлях | Що повертає |
|---|---|---|
| GET | `/health` | `{"status": "ok"}` — живий чи ні |
| GET | `/sensors/latest` | останній запис пристрою; `404`, якщо даних ще немає |
| GET | `/sensors/history?minutes=30` | записи за останні N хвилин (за замовчуванням 30) |

### Як влаштовані запити (db.py)

**`get_latest()`** — query у партицію пристрою, `ScanIndexForward=False`
(найновіші згори), `Limit=1`. DynamoDB читає рівно один item — це і швидко,
і дешево.

**`get_history(minutes)`** — query по діапазону sort key:
`device_id = ... AND received_at >= cutoff`. Межа рахується в **мілісекундах**
(`time.time() * 1000`), бо `received_at` записаний правилом через
`timestamp()` — а він у мілісекундах (Заняття 11).

**Чому query, а не scan:** query йде точно в партицію по ключу і читає тільки
потрібне. Scan перечитує всю таблицю і фільтрує потім — на великій таблиці це
повільно і дорого. Ключ таблиці будувався саме під ці запити ще на Занятті 11.

---

## Troubleshooting

| Симптом | Куди дивитись |
|---|---|
| `Unable to locate credentials` | `.env` не скопійований або `load_dotenv()` після `import db` |
| `ResourceNotFoundException` | не той регіон у `AWS_DEFAULT_REGION` або не та назва таблиці |
| `AccessDeniedException` | IAM-користувач не має `dynamodb:Query` на таблицю |
| `/sensors/latest` → 404 | таблиця порожня — запустіть пристрій (Заняття 11) |
| `/sensors/history` → `[]` | дані є, але старіші за N хвилин — збільшіть `minutes` |
| Сервер не стартує | активуйте `.venv` і перевірте `pip install -r requirements.txt` |

---

## Ключові факти заняття

- Бекенд **читає** ту саму таблицю, куди **пише** Rules Engine — і вони
  нічого не знають одне про одного
- Query — точковий удар по ключу. Scan — перечитати все. Ключ будувався під query
- `received_at` — мілісекунди, `timestamp` із пристрою — секунди. Не плутати
- `.env` до git не потрапляє. Ключі, що потрапили в git, вважаються скомпрометованими
- FastAPI генерує `/docs` сам — це безкоштовний інструмент перевірки API
- **Пристрій дає дані — хмара дає гарантії — бекенд дає доступ**

---

## Рекомендована література

### FastAPI

- Офіційна документація — [fastapi.tiangolo.com](https://fastapi.tiangolo.com/)
- Tutorial (User Guide) — [fastapi.tiangolo.com/tutorial](https://fastapi.tiangolo.com/tutorial/)

### AWS IAM

- Керування access keys — [docs.aws.amazon.com/IAM/…/id_credentials_access-keys](https://docs.aws.amazon.com/IAM/latest/UserGuide/id_credentials_access-keys.html)
- Політики й дозволи (least privilege) — [docs.aws.amazon.com/IAM/…/access_policies](https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies.html)

### boto3 / DynamoDB

- boto3 Table.query — [boto3.amazonaws.com/…/dynamodb/table/query](https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/dynamodb/table/query.html)
- Query vs Scan (key conditions) — [docs.aws.amazon.com/…/Query.KeyConditionExpressions](https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/Query.KeyConditionExpressions.html)

### Копни глибше (по бажанню)

- **Pydantic response models** — типізована відповідь endpoint замість голого dict
- **def vs async def** — чому з синхронним boto3 беремо `def` (FastAPI винесе в threadpool)
- **boto3 credential chain** — у якому порядку шукаються ключі: env → `~/.aws` → IAM-роль
- **Пагінація DynamoDB** (`LastEvaluatedKey`) — коли історія не влазить у 1 MB
- **/redoc** — друга авто-документація поряд зі Swagger `/docs`

---
