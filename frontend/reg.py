from uuid import uuid4

import psycopg
import requests
from flask import Blueprint, redirect, render_template, request, session
from psycopg.errors import UniqueViolation
from werkzeug.security import check_password_hash, generate_password_hash

from core import ROLES, ServiceError, get_current_user, get_db_connection, get_news_ids

auth_bp = Blueprint("auth", __name__)


def show_auth(error=None, status=200):
    return render_template("index.html", roles=ROLES, error=error), status


@auth_bp.get("/")
def home():
    return redirect("/feed") if get_current_user() else show_auth()


@auth_bp.post("/register")
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
        return show_auth("Не удалось проверить ID клиента. Попробуйте позже.", 503)
    user_id = str(uuid4())
    try:
        with get_db_connection() as connection:
            connection.execute("INSERT INTO public.users (id, name, login, password_hash, f_uid, role) VALUES (%s, %s, %s, %s, %s, %s)", (user_id, name, email, generate_password_hash(password), f_uid, role))
    except UniqueViolation:
        return show_auth("Этот email уже зарегистрирован.", 409)
    except psycopg.Error:
        return show_auth("Не удалось сохранить пользователя. Попробуйте позже.", 503)
    session.clear()
    session["user_id"] = user_id
    return redirect("/feed")


@auth_bp.post("/login")
def login():
    email = request.form.get("email", "").strip().lower()
    password = request.form.get("password", "")
    try:
        with get_db_connection() as connection:
            user = connection.execute("SELECT id, password_hash FROM public.users WHERE login = %s", (email,)).fetchone()
    except psycopg.Error:
        return show_auth("Не удалось выполнить вход. Попробуйте позже.", 503)
    if user and check_password_hash(user["password_hash"], password):
        session.clear()
        session["user_id"] = str(user["id"])
        return redirect("/feed")
    return show_auth("Неверный email или пароль.", 401)


@auth_bp.post("/logout")
def logout():
    session.clear()
    return redirect("/")
