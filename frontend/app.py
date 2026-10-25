import secrets
import sqlite3

import requests
from flask import Flask, abort, jsonify, redirect, render_template, session

from core import (
    ROLES, ServiceError, analysis_cache_key, analyze_news, get_cached_analysis,
    get_current_user, get_news_by_ids, get_news_ids, save_cached_analysis,
)
from profile import profile_bp
from reg import auth_bp

app = Flask(__name__)
# A stable key is required if several Flask workers/containers are used.
# In production set FLASK_SECRET_KEY in the environment.
import os
app.secret_key = os.environ.get("FLASK_SECRET_KEY") or secrets.token_hex(32)
app.register_blueprint(auth_bp)
app.register_blueprint(profile_bp)


# The Java feed endpoint runs an LLM-based ranking. Its result may change
# between the feed request and a click. Keep the IDs actually shown to
# this authenticated user, instead of re-ranking on every article visit.
def _feed_scope(user):
    return [str(user["id"]), user.get("f_uid"), user.get("role")]


def remember_visible_news(user, ids):
    scope = _feed_scope(user)
    previous = session.get("visible_news_ids", []) if session.get("visible_news_scope") == scope else []
    valid_ids = [value for value in [*previous, *ids] if type(value) is int and value > 0]
    session["visible_news_ids"] = list(dict.fromkeys(valid_ids))[-200:]
    session["visible_news_scope"] = scope


def may_open_news(user, news_id):
    if session.get("visible_news_scope") == _feed_scope(user):
        return news_id in session.get("visible_news_ids", [])
    # Support a direct URL in a fresh session. Do not accept arbitrary IDs.
    ids = get_news_ids(user)
    remember_visible_news(user, ids)
    return news_id in ids


@app.get("/feed")
def feed():
    user = get_current_user()
    if not user:
        session.clear()
        return redirect("/")
    error, news, status = None, [], 200
    try:
        news = get_news_by_ids(get_news_ids(user))
        remember_visible_news(user, [item["id"] for item in news])
    except ServiceError as exc:
        error, status = str(exc), 400
    except (requests.RequestException, ValueError, KeyError, TypeError):
        app.logger.exception("Could not load feed")
        error, status = "Не удалось загрузить новости. Попробуйте обновить страницу позже.", 503
    read_news_ids = {
        news_id for news_id in session.get("read_news_ids", [])
        if isinstance(news_id, int)
    }
    return render_template(
        "feed.html", user=user, role=ROLES.get(user["role"], user["role"]),
        news=news, read_news_ids=read_news_ids, error=error,
    ), status


@app.get("/news/<int:news_id>")
def news_detail(news_id):
    user = get_current_user()
    if not user:
        return redirect("/")
    try:
        if not may_open_news(user, news_id):
            abort(404)
        items = get_news_by_ids([news_id])
    except ServiceError as exc:
        return render_template("feed.html", user=user, role=ROLES[user["role"]], news=[], error=str(exc)), 400
    except (requests.RequestException, ValueError, KeyError, TypeError):
        app.logger.exception("Could not load article")
        return render_template("feed.html", user=user, role=ROLES[user["role"]], news=[], error="Не удалось загрузить новость. Попробуйте позже."), 503
    if not items:
        abort(404)
    read_news_ids = session.get("read_news_ids", [])
    if news_id not in read_news_ids:
        session["read_news_ids"] = [*read_news_ids, news_id]
    return render_template("news.html", user=user, news=items[0], role=ROLES[user["role"]])


@app.post("/news/<int:news_id>/analysis")
def news_analysis(news_id):
    user = get_current_user()
    if not user:
        return jsonify(error="Сессия завершена. Войдите снова."), 401
    try:
        if not may_open_news(user, news_id):
            abort(404)
        items = get_news_by_ids([news_id])
        if not items:
            abort(404)
        news = items[0]
        cache_key = analysis_cache_key(news, user)
        result = get_cached_analysis(cache_key)
        if result is None:
            result = analyze_news(news, user)
            save_cached_analysis(cache_key, result)
        return jsonify(analysis=result["analysis"], sources=result.get("sources", []))
    except ServiceError as exc:
        return jsonify(error=str(exc)), 400
    except (requests.RequestException, ValueError, KeyError, TypeError, IndexError, sqlite3.Error):
        app.logger.exception("Could not analyze article")
        return jsonify(error="Не удалось выполнить анализ. Попробуйте ещё раз."), 503


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
