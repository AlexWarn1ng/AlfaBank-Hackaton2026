import json
import hashlib
import secrets
import sqlite3
import os
from pathlib import Path
from threading import Lock
from uuid import uuid4
from urllib.parse import quote
import requests
import psycopg
from psycopg.rows import dict_row
from psycopg.errors import UniqueViolation

from flask import Flask, render_template, request, redirect, session, abort, jsonify
from werkzeug.security import generate_password_hash, check_password_hash

app = Flask(__name__)
app.secret_key = secrets.token_hex(32)

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
COMPANY_SERVICE_URL = os.getenv("COMPANY_SERVICE_URL", "http://localhost:8888").rstrip("/")

class ServiceError(Exception):
    pass

ROLES = {
    "ceo": "Генеральный директор",
    "cfo": "Финансовый директор",
    "accountant": "Главный бухгалтер",
    "treasurer": "Руководитель казначейства",
}

NEWS_ANALYZER_URL = os.getenv("NEWS_ANALYZER_URL", "http://localhost:5001").rstrip("/")


def get_db_connection():
    return psycopg.connect(
        host=os.environ["POSTGRES_HOST"],
        port=os.getenv("POSTGRES_PORT", "5432"),
        dbname=os.environ["POSTGRES_DB"],
        user=os.environ["POSTGRES_USER"],
        password=os.environ["POSTGRES_PASSWORD"],
        row_factory=dict_row,
    )


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
        f"{COMPANY_SERVICE_URL}/api/news/get",
        data=" ".join(map(str, ids)),
        headers={"Content-Type": "text/plain"},
        timeout=15,
    )
    response.raise_for_status()
    payload = response.json()
    items = payload["news"]
    if not isinstance(items, list):
        raise ValueError("Invalid news response")
    by_id = {}
    for item in items:
        if (not isinstance(item, dict) or type(item.get("id")) is not int
                or not isinstance(item.get("title"), str)
                or not isinstance(item.get("text"), str)):
            raise ValueError("Invalid news item")
        by_id[item["id"]] = item
    return [by_id[i] for i in ids if i in by_id]


def analyze_news(news_id, user):
    response = requests.post(
        f"{NEWS_ANALYZER_URL}/api/analysis/relevance",
        json={
            "userId": user["id"],
            "newsId": news_id,
        },
        timeout=60,
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

    for field in ("summary", "relevanceReason", "recommendedAction"):
        if not isinstance(analysis.get(field), str):
            raise ValueError("Invalid analysis text")

    for field in ("potentialRisks", "potentialOpportunities"):
        if not isinstance(analysis.get(field), list) or any(
                not isinstance(item, str) for item in analysis[field]):
            raise ValueError("Invalid analysis list")

    return analysis


def analysis_cache_key(news_id, user):
    source = f"{news_id}\n{user['id']}\n{user['role']}\n{user['f_uid']}"
    return hashlib.sha256(source.encode("utf-8")).hexdigest()


def get_cached_analysis(cache_key):
    with analysis_cache_lock, sqlite3.connect(ANALYSIS_CACHE_FILE) as connection:
        connection.execute("""
            CREATE TABLE IF NOT EXISTS analysis_cache (
                cache_key TEXT PRIMARY KEY,
                analysis TEXT NOT NULL
            )
        """)
        row = connection.execute(
            "SELECT analysis FROM analysis_cache WHERE cache_key = ?", (cache_key,)
        ).fetchone()
    return json.loads(row[0]) if row else None


def save_cached_analysis(cache_key, analysis):
    with analysis_cache_lock, sqlite3.connect(ANALYSIS_CACHE_FILE) as connection:
        connection.execute("""
            CREATE TABLE IF NOT EXISTS analysis_cache (
                cache_key TEXT PRIMARY KEY,
                analysis TEXT NOT NULL
            )
        """)
        connection.execute(
            "INSERT OR REPLACE INTO analysis_cache (cache_key, analysis) VALUES (?, ?)",
            (cache_key, json.dumps(analysis, ensure_ascii=False)),
        )


def get_current_user():
    user_id = session.get("user_id")
    if not user_id:
        return None
    with get_db_connection() as connection:
        row = connection.execute(
            "SELECT id, login, f_uid, role FROM public.users WHERE id = %s",
            (user_id,),
        ).fetchone()
    if not row:
        return None
    return {
        "id": str(row["id"]),
        "name": row["login"],
        "email": row["login"],
        "f_uid": row["f_uid"],
        "role": row["role"],
    }


def show_auth(error=None, status=200):
    return render_template(
        "index.html",
        roles=ROLES,
        error=error,
    ), status


@app.route("/")
def home():
    if get_current_user():
        return redirect("/feed")

    return show_auth()


@app.route("/register", methods=["POST"])
def register():
    name = request.form.get("name", "").strip()
    email = request.form.get("email", "").strip().lower()
    password = request.form.get("password", "")
    f_uid = request.form.get("f_uid", "").strip()
    role = request.form.get("role", "")

    if not all([name, email, password, f_uid, role]):
        return show_auth("Заполните все поля.", 400)

    if "@" not in email:
        return show_auth("Введите корректный email.", 400)

    if len(password) < 8:
        return show_auth("Пароль должен содержать минимум 8 символов.", 400)

    if role not in ROLES:
        return show_auth("Выберите должность из списка.", 400)

    try:
        get_news_ids({"f_uid": f_uid, "role": role})
    except ServiceError as error:
        return show_auth(str(error), 400)
    except (requests.RequestException, ValueError, KeyError, TypeError):
        app.logger.exception("Could not validate F_uid")
        return show_auth("Не удалось проверить ID клиента. Попробуйте позже.", 503)

    new_user_id = str(uuid4())
    try:
        with get_db_connection() as connection:
            connection.execute(
                """
                INSERT INTO public.users (id, login, password_hash, f_uid, role)
                VALUES (%s, %s, %s, %s, %s)
                """,
                (new_user_id, email, generate_password_hash(password), f_uid, role),
            )
    except UniqueViolation:
        return show_auth("Этот email уже зарегистрирован.", 409)
    except psycopg.Error:
        app.logger.exception("Could not register user")
        return show_auth("Не удалось сохранить пользователя. Попробуйте позже.", 503)

    session.clear()
    session["user_id"] = new_user_id

    return redirect("/feed")


@app.route("/login", methods=["POST"])
def login():
    email = request.form.get("email", "").strip().lower()
    password = request.form.get("password", "")

    try:
        with get_db_connection() as connection:
            user = connection.execute(
                "SELECT id, password_hash FROM public.users WHERE login = %s",
                (email,),
            ).fetchone()
    except psycopg.Error:
        app.logger.exception("Could not log in user")
        return show_auth("Не удалось выполнить вход. Попробуйте позже.", 503)

    if user and check_password_hash(user["password_hash"], password):
        session.clear()
        session["user_id"] = str(user["id"])
        return redirect("/feed")

    return show_auth("Неверный email или пароль.", 401)


@app.route("/feed")
def feed():
    user = get_current_user()

    if not user:
        session.clear()
        return redirect("/")

    error = None
    news = []
    status = 200
    try:
        news = get_news_by_ids(get_news_ids(user))
    except ServiceError as exc:
        error, status = str(exc), 400
    except (requests.RequestException, ValueError, KeyError, TypeError):
        app.logger.exception("Could not load feed")
        error, status = "Не удалось загрузить новости. Попробуйте обновить страницу позже.", 503
    return render_template(
        "feed.html", user=user, role=ROLES.get(user["role"], user["role"]),
        news=news, error=error,
    ), status

@app.route("/logout", methods=["POST"])
def logout():
    session.clear()
    return redirect("/")

@app.route("/news/<int:news_id>")
def news_detail(news_id):
    user = get_current_user()
    if not user:
        return redirect("/")
    try:
        if news_id not in get_news_ids(user):
            abort(404)
        items = get_news_by_ids([news_id])
    except ServiceError as exc:
        return render_template("feed.html", user=user, role=ROLES[user["role"]],
                               news=[], error=str(exc)), 400
    except (requests.RequestException, ValueError, KeyError, TypeError):
        app.logger.exception("Could not load article")
        return render_template("feed.html", user=user, role=ROLES[user["role"]],
                               news=[], error="Не удалось загрузить новость. Попробуйте позже."), 503
    if not items:
        abort(404)
    news = items[0]
    return render_template("news.html", user=user, news=news,
                           role=ROLES[user["role"]])


@app.route("/news/<int:news_id>/analysis", methods=["POST"])
def news_analysis(news_id):
    user = get_current_user()
    if not user:
        return jsonify(error="Сессия завершена. Войдите снова."), 401
    try:
        if news_id not in get_news_ids(user):
            abort(404)
        cache_key = analysis_cache_key(news_id, user)
        analysis = get_cached_analysis(cache_key)

        if analysis is None:
            analysis = analyze_news(news_id, user)
            save_cached_analysis(cache_key, analysis)

        return jsonify(analysis=analysis)
    except ServiceError as exc:
        return jsonify(error=str(exc)), 400
    except (requests.RequestException, ValueError, KeyError, TypeError, IndexError, sqlite3.Error):
        app.logger.exception("Could not analyze article")
        return jsonify(error="Не удалось выполнить анализ. Попробуйте ещё раз."), 503


if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=False
    )
