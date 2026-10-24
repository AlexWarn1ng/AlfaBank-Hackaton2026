import psycopg
import requests
from psycopg.errors import UniqueViolation
from flask import Blueprint, redirect, render_template, request, session, url_for
from werkzeug.security import check_password_hash, generate_password_hash

from core import ROLES, ServiceError, get_current_user, get_db_connection, get_news_ids

profile_bp = Blueprint("profile", __name__)


def profile_redirect():
    return redirect(url_for("profile.profile_page"))


@profile_bp.get("/profile")
def profile_page():
    user = get_current_user()
    if not user:
        session.clear()
        return redirect("/")
    return render_template("profile.html", user=user, roles=ROLES,
                           role=ROLES[user["role"]],
                           error=session.pop("profile_error", None),
                           success=session.pop("profile_success", None))


@profile_bp.post("/profile")
def update_profile():
    user = get_current_user()
    if not user:
        session.clear()
        return redirect("/")
    name = request.form.get("name", "").strip()
    email = request.form.get("email", "").strip().lower()
    f_uid = request.form.get("f_uid", "").strip()
    role = request.form.get("role", "")
    if not all([name, email, f_uid, role]) or role not in ROLES:
        session["profile_error"] = "Заполните все поля и выберите должность."
        return profile_redirect()
    if "@" not in email:
        session["profile_error"] = "Введите корректный email."
        return profile_redirect()
    if len(name) > 255 or len(email) > 255 or len(f_uid) > 100:
        session["profile_error"] = "Слишком длинное значение в профиле."
        return profile_redirect()
    feed_settings_changed = f_uid != user["f_uid"] or role != user["role"]
    if feed_settings_changed:
        try:
            get_news_ids({"f_uid": f_uid, "role": role})
        except ServiceError as error:
            session["profile_error"] = str(error)
            return profile_redirect()
        except (requests.RequestException, ValueError, KeyError, TypeError):
            session["profile_error"] = "Не удалось проверить профиль. Попробуйте позже."
            return profile_redirect()
    try:
        with get_db_connection() as connection:
            connection.execute(
                "UPDATE public.users SET name = %s, login = %s, f_uid = %s, role = %s WHERE id = %s",
                (name, email, f_uid, role, user["id"]),
            )
    except UniqueViolation:
        session["profile_error"] = "Этот email уже зарегистрирован."
        return profile_redirect()
    except psycopg.Error:
        session["profile_error"] = "Не удалось сохранить профиль. Попробуйте позже."
        return profile_redirect()
    session["profile_success"] = "Профиль обновлён."
    return profile_redirect()


@profile_bp.post("/profile/password")
def update_password():
    user = get_current_user()
    if not user:
        session.clear()
        return redirect("/")

    current_password = request.form.get("current_password", "")
    new_password = request.form.get("new_password", "")
    password_repeat = request.form.get("password_repeat", "")
    if not all([current_password, new_password, password_repeat]):
        session["profile_error"] = "Заполните все поля для смены пароля."
        return profile_redirect()
    if new_password != password_repeat:
        session["profile_error"] = "Новый пароль и повтор не совпадают."
        return profile_redirect()
    if len(new_password) < 8:
        session["profile_error"] = "Новый пароль должен содержать минимум 8 символов."
        return profile_redirect()

    try:
        with get_db_connection() as connection:
            row = connection.execute(
                "SELECT password_hash FROM public.users WHERE id = %s", (user["id"],)
            ).fetchone()
            if not row or not check_password_hash(row["password_hash"], current_password):
                session["profile_error"] = "Текущий пароль указан неверно."
                return profile_redirect()
            connection.execute(
                "UPDATE public.users SET password_hash = %s WHERE id = %s",
                (generate_password_hash(new_password), user["id"]),
            )
    except psycopg.Error:
        session["profile_error"] = "Не удалось изменить пароль. Попробуйте позже."
        return profile_redirect()
    session["profile_success"] = "Пароль успешно изменён."
    return profile_redirect()
