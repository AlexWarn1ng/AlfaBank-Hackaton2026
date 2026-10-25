import hashlib
import json
import os
import sqlite3
from pathlib import Path
from threading import Lock
from urllib.parse import quote

import psycopg
import requests
from flask import session
from psycopg.rows import dict_row

DATA_DIR = Path(__file__).resolve().parent / "data"
ANALYSIS_CACHE_FILE = DATA_DIR / "analysis_cache.sqlite3"
analysis_cache_lock = Lock()
DATA_DIR.mkdir(exist_ok=True)

JAVA_ROLES = {
    "ceo": "CEO",
    "cfo": "CFO",
    "accountant": "ChiefAccountant",
    "treasurer": "HeadofTreasury",
}
ROLES = {
    "ceo": "Генеральный директор",
    "cfo": "Финансовый директор",
    "accountant": "Главный бухгалтер",
    "treasurer": "Руководитель казначейства",
}
COMPANY_SERVICE_URL = os.getenv("COMPANY_SERVICE_URL", "http://localhost:8888").rstrip("/")
NEWS_ANALYZER_URL = os.getenv("NEWS_ANALYZER_URL", "http://localhost:5001").rstrip("/")


class ServiceError(Exception):
    pass


def get_db_connection():
    return psycopg.connect(
        host=os.environ["POSTGRES_HOST"],
        port=os.getenv("POSTGRES_PORT", "5432"),
        dbname=os.environ["POSTGRES_DB"],
        user=os.environ["POSTGRES_USER"],
        password=os.environ["POSTGRES_PASSWORD"],
        row_factory=dict_row,
    )


def get_current_user():
    user_id = session.get("user_id")
    if not user_id:
        return None
    with get_db_connection() as connection:
        row = connection.execute(
            "SELECT id, name, login, f_uid, role FROM public.users WHERE id = %s",
            (user_id,),
        ).fetchone()
    if not row:
        return None
    return {
        "id": str(row["id"]), "name": row["name"], "email": row["login"],
        "f_uid": row["f_uid"], "role": row["role"],
    }


def get_news_ids(user):
    if not user.get("f_uid"):
        raise ServiceError("У этого аккаунта нет F_uid. Создайте новый аккаунт с ID клиента.")
    response = requests.get(
        f"{COMPANY_SERVICE_URL}/api/news/{quote(user['f_uid'], safe='')}/{JAVA_ROLES[user['role']]}",
        timeout=15,
    )
    if response.status_code == 400:
        message = response.json().get("error")
        if message == "Can not find your company":
            raise ServiceError("Для этого F_uid не найдена связанная компания. Проверьте ID.")
        if message == "Job Title not Found":
            raise ServiceError("Должность не найдена в сервисе подбора новостей.")
    response.raise_for_status()
    ids = response.json()["ID"]
    if not isinstance(ids, list) or any(type(i) is not int for i in ids):
        raise ValueError("Invalid news IDs")
    return list(dict.fromkeys(ids))


def get_news_by_ids(ids):
    if not ids:
        return []
    response = requests.post(
        f"{COMPANY_SERVICE_URL}/api/news/get", data=" ".join(map(str, ids)),
        headers={"Content-Type": "text/plain"}, timeout=15,
    )
    response.raise_for_status()
    items = response.json()["news"]
    if not isinstance(items, list):
        raise ValueError("Invalid news response")
    by_id = {item["id"]: item for item in items}
    return [by_id[item_id] for item_id in ids if item_id in by_id]


def analyze_news(news, user):
    response = requests.post(
        f"{NEWS_ANALYZER_URL}/api/analysis",
        json={"userId": user["id"], "newsId": news["id"]}, timeout=60,
    )
    response.raise_for_status()
    analysis = response.json()["analysis"]
    if isinstance(analysis, str):
        text = analysis.strip()
        if text.startswith("```") and text.endswith("```"):
            text = text.split("\n", 1)[1].rsplit("```", 1)[0].strip()
        analysis = json.loads(text)
    if not isinstance(analysis, dict):
        raise ValueError("Invalid analysis")
    return analysis


def analysis_cache_key(news, user):
    source = f"{news['id']}\n{user['role']}\n{news['text']}"
    return hashlib.sha256(source.encode("utf-8")).hexdigest()


def get_cached_analysis(cache_key):
    with analysis_cache_lock, sqlite3.connect(ANALYSIS_CACHE_FILE) as connection:
        connection.execute("CREATE TABLE IF NOT EXISTS analysis_cache (cache_key TEXT PRIMARY KEY, analysis TEXT NOT NULL)")
        row = connection.execute("SELECT analysis FROM analysis_cache WHERE cache_key = ?", (cache_key,)).fetchone()
    return json.loads(row[0]) if row else None


def save_cached_analysis(cache_key, analysis):
    with analysis_cache_lock, sqlite3.connect(ANALYSIS_CACHE_FILE) as connection:
        connection.execute("CREATE TABLE IF NOT EXISTS analysis_cache (cache_key TEXT PRIMARY KEY, analysis TEXT NOT NULL)")
        connection.execute("INSERT OR REPLACE INTO analysis_cache (cache_key, analysis) VALUES (?, ?)", (cache_key, json.dumps(analysis, ensure_ascii=False)))
