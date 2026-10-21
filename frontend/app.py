import json
import secrets
from pathlib import Path
from threading import Lock
from uuid import uuid4
import requests

from flask import Flask, render_template, request, redirect, session, abort
from werkzeug.security import generate_password_hash, check_password_hash

app = Flask(__name__)
app.secret_key = secrets.token_hex(32)

DATA_DIR = Path(__file__).resolve().parent / "data"
USERS_FILE = DATA_DIR / "users.json"
NEWS_FILE = DATA_DIR / "news.json"

# Акцент подачи для разных должностей.
ROLE_FOCUS = {
    "ceo": "Влияние на работу компании и сроки реализации проектов.",
    "cfo": "Влияние на расходы, финансовые обязательства и планирование.",
    "accountant": "Сроки расчётов и документы, которые необходимо проверить.",
    "treasurer": "Платёжный календарь и своевременное исполнение платежей.",
}
users_lock = Lock()

DATA_DIR.mkdir(exist_ok=True)

COMPANIES = {
    "company_1": "Компания 1",
    "company_2": "Компания 2",
}

ROLES = {
    "ceo": "Генеральный директор",
    "cfo": "Финансовый директор",
    "accountant": "Главный бухгалтер",
    "treasurer": "Руководитель казначейства",
}

def load_users():
    # До первой регистрации файла может не быть.
    if not USERS_FILE.exists():
        return []

    # Загружаем пользователей для входа и регистрации.
    with USERS_FILE.open("r", encoding="utf-8") as file:
        return json.load(file)

def load_news():
    # Читаем тестовые новости из файла.
    with NEWS_FILE.open("r", encoding="utf-8") as file:
        return json.load(file)

def analyze_news(news, user):
    response = requests.post(
        "http://localhost:5001/api/analysis/relevance",
        json={
            "newsTitle": news["title"],
            "newsText": news.get("summary", ""),
            "userRole": ROLES[user["role"]],
            "companyName": COMPANIES[user["company_id"]],
            "industry": "",
            "businessGoals": "",
            "interests": ""
        },
        timeout=60
    )

    response.raise_for_status()
    return response.json()

def get_personal_news(user):
    # Собираем ленту для компании и должности пользователя.
    result = []

    for news in load_news():
        company_data = news["companies"].get(user["company_id"])

        if not company_data:
            continue

        # Пока релевантность задаётся тестовыми числами из JSON.
        role_bonus = news["role_bonus"].get(user["role"], 0)
        score = company_data["score"] + role_bonus

        result.append({
            "id": news["id"],
            "title": news["title"],
            "summary": news["summary"],
            "source": news["source"],
            "date": news["date"],
            "reason": company_data["reason"],
            "score": score,
        })

    # Сначала показываем наиболее релевантные новости.
    result.sort(key=lambda news: news["score"], reverse=True)

    return result


def save_users(users):
    temporary_file = USERS_FILE.with_suffix(".tmp")

    with temporary_file.open("w", encoding="utf-8") as file:
        json.dump(users, file, ensure_ascii=False, indent=4)

    temporary_file.replace(USERS_FILE)


def get_current_user():
    user_id = session.get("user_id")

    if not user_id:
        return None

    with users_lock:
        users = load_users()

    for user in users:
        if user["id"] == user_id:
            return user

    return None


def show_auth(error=None, status=200):
    return render_template(
        "index.html",
        companies=COMPANIES,
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
    company_id = request.form.get("company_id", "")
    role = request.form.get("role", "")

    if not all([name, email, password, company_id, role]):
        return show_auth("Заполните все поля.", 400)

    if "@" not in email:
        return show_auth("Введите корректный email.", 400)

    if len(password) < 8:
        return show_auth("Пароль должен содержать минимум 8 символов.", 400)

    if company_id not in COMPANIES:
        return show_auth("Выберите компанию из списка.", 400)

    if role not in ROLES:
        return show_auth("Выберите должность из списка.", 400)

    password_hash = generate_password_hash(password)

    with users_lock:
        users = load_users()

        for user in users:
            if user["email"] == email:
                return show_auth("Этот email уже зарегистрирован.", 409)

        new_user = {
            "id": str(uuid4()),
            "name": name,
            "email": email,
            "password_hash": password_hash,
            "company_id": company_id,
            "role": role,
        }

        users.append(new_user)
        save_users(users)

    session.clear()
    session["user_id"] = new_user["id"]

    return redirect("/feed")


@app.route("/login", methods=["POST"])
def login():
    email = request.form.get("email", "").strip().lower()
    password = request.form.get("password", "")

    with users_lock:
        users = load_users()

    for user in users:
        if user["email"] == email:
            if check_password_hash(user["password_hash"], password):
                session.clear()
                session["user_id"] = user["id"]
                return redirect("/feed")

    return show_auth("Неверный email или пароль.", 401)


@app.route("/feed")
def feed():
    user = get_current_user()

    if not user:
        session.clear()
        return redirect("/")

    news = get_personal_news(user)

    return render_template(
        "feed.html",
        user=user,
        company=COMPANIES.get(user["company_id"], user["company_id"]),
        role=ROLES.get(user["role"], user["role"]),
        news=news,
    )

@app.route("/logout", methods=["POST"])
def logout():
    session.clear()
    return redirect("/")

@app.route("/news/<news_id>")
def news_detail(news_id):
    user = get_current_user()

    if not user:
        return redirect("/")

    for news in load_news():
        if news["id"] != news_id:
            continue

        company_data = news["companies"].get(user["company_id"])

        if not company_data:
            abort(404)

        # Запрашиваем AI-анализ именно выбранной новости.
        try:
            result = analyze_news(news, user)

            # Пока твой сервис возвращает analysis
            # как строку, содержащую JSON.
            analysis = json.loads(result["analysis"])

            print("АНАЛИЗ НОВОСТИ:", analysis)

        except (requests.RequestException, ValueError, KeyError) as e:
            print("Ошибка анализа:", e)
            analysis = None

        return render_template(
            "news.html",
            user=user,
            news=news,
            analysis=company_data["analysis"],
            ai_analysis=analysis,
            reason=company_data["reason"],
            company=COMPANIES.get(
                user["company_id"],
                user["company_id"]
            ),
            role=ROLES.get(
                user["role"],
                user["role"]
            ),
            role_focus=ROLE_FOCUS.get(user["role"], ""),
        )

    abort(404)


if __name__ == "__main__":
    app.run(debug=True)