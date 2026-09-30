import os
import time
import boto3
from boto3.dynamodb.conditions import Key




# ═══════════════ Підключення до DynamoDB ═══════════════
# Регіон задаємо явно — щоб не залежати від налаштувань aws config
dynamodb = boto3.resource("dynamodb", region_name=os.getenv("AWS_DEFAULT_REGION"))

TABLE_NAME = os.getenv("TABLE_NAME")
TABLE_NAME_EVENTS = os.getenv("TABLE_NAME_EVENTS")
DEVICE_ID = os.getenv("DEVICE_ID")

table = dynamodb.Table(TABLE_NAME)
table_events = dynamodb.Table(TABLE_NAME_EVENTS)



# ═══════════════ Запити до таблиці ═══════════════
def get_latest():
    """Останній замір пристрою: у його партицію, згори, один."""
    response = table.query(
        KeyConditionExpression=Key("client_id").eq(DEVICE_ID),
        ScanIndexForward=False,   # найновіші згори
        Limit=1,
    )
    items = response["Items"]
    return items[0] if items else None


def get_history(minutes):
    """Записи пристрою за останні N хвилин."""
    now_ms = int(time.time() * 1000)            # зараз — у мілісекундах
    cutoff_ms = now_ms - minutes * 60 * 1000    # межа: N хвилин тому

    response = table.query(
        KeyConditionExpression=Key("client_id").eq(DEVICE_ID)
                               & Key("received_at").gte(cutoff_ms),
    )
    return response["Items"]

def get_events(minutes):
    """Записи подій пристрою за останні N хвилин."""
    now_ms = int(time.time() * 1000)            # зараз — у мілісекундах
    cutoff_ms = now_ms - minutes * 60 * 1000    # межа: N хвилин тому

    response = table_events.query(
        KeyConditionExpression=Key("client_id").eq(DEVICE_ID)
                               & Key("received_at").gte(cutoff_ms),
    )
    return response["Items"]

