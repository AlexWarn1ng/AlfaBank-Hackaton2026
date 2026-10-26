import importlib.util
import sys
from pathlib import Path
from unittest.mock import Mock

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
spec = importlib.util.spec_from_file_location("profile", ROOT / "profile.py")
profile = importlib.util.module_from_spec(spec)
sys.modules["profile"] = profile
spec.loader.exec_module(profile)
import core
import app as web

USER = {"id": "user-a", "f_uid": "F_64255", "role": "treasurer", "name": "Егор"}
NEWS = {"id": 220, "title": "Новость", "text": "Текст"}

@pytest.fixture(autouse=True)
def isolated_cache(tmp_path, monkeypatch):
    monkeypatch.setattr(core, "ANALYSIS_CACHE_FILE", tmp_path / "cache.sqlite3")
    web.app.config.update(TESTING=True, SECRET_KEY="test-key")


def mock_feed(monkeypatch):
    rank = Mock(return_value=[220])
    load = Mock(return_value=[NEWS])
    monkeypatch.setattr(core, "get_news_ids", rank)
    monkeypatch.setattr(core, "get_news_by_ids", load)
    return rank, load


def test_cached_cards_avoid_ranking_and_article_fetch(monkeypatch):
    rank, load = mock_feed(monkeypatch)
    assert core.get_feed(USER) == {"news": [NEWS]}
    assert core.get_feed(USER) == {"news": [NEWS]}
    assert core.get_article(USER, 220) == [NEWS]
    rank.assert_called_once_with(USER)
    load.assert_called_once_with([220])


def test_scope_isolates_user_role_and_person():
    core.save_cached_feed(USER, {"news": [NEWS]})
    for field, value in [("id", "user-b"), ("role", "ceo"), ("f_uid", "F_10000")]:
        assert core.get_cached_feed({**USER, field: value}) is None


def test_expiry_empty_feed_and_invalidation(monkeypatch):
    monkeypatch.setattr(core.time, "time", lambda: 1000)
    core.save_cached_feed(USER, {"news": []})
    assert core.get_cached_feed(USER) == {"news": []}
    monkeypatch.setattr(core.time, "time", lambda: 1601)
    assert core.get_cached_feed(USER) is None
    core.save_cached_feed(USER, {"news": [NEWS]})
    core.invalidate_feed_cache(USER)
    assert core.get_cached_feed(USER) is None


def test_failure_does_not_cache_error(monkeypatch):
    mock_feed(monkeypatch)
    monkeypatch.setattr(core, "get_news_ids", Mock(side_effect=core.requests.Timeout))
    with pytest.raises(core.requests.Timeout):
        core.get_feed(USER)
    assert core.get_cached_feed(USER) is None


def test_navigation_read_badge_refresh_and_authorization(monkeypatch):
    rank, load = mock_feed(monkeypatch)
    monkeypatch.setattr(web, "get_current_user", lambda: USER)
    with web.app.test_client() as client:
        assert client.get("/feed").status_code == 200
        article = client.get("/news/220")
        assert article.status_code == 200
        text = article.get_data(as_text=True)
        assert text.index("Что можно сделать?") < text.index("Подтверждающие источники и выводы")
        returned = client.get("/feed").get_data(as_text=True)
        assert "news-card-read" in returned
        assert rank.call_count == load.call_count == 1
        assert client.get("/news/999").status_code == 404
        assert client.post("/feed/refresh", follow_redirects=True).status_code == 200
        assert rank.call_count == 2


def test_cached_analysis_still_preserves_no_company_notice(monkeypatch):
    core.save_cached_feed(USER, {"news": [NEWS]})
    monkeypatch.setattr(web, "get_current_user", lambda: USER)
    analyze = Mock(return_value={"analysis": {"relevanceReason": "Компания не найдена"}, "sources": []})
    monkeypatch.setattr(web, "analyze_news", analyze)
    with web.app.test_client() as client:
        with client.session_transaction() as session:
            session["visible_news_scope"] = web._feed_scope(USER)
            session["visible_news_ids"] = [220]
        for _ in range(2):
            response = client.post("/news/220/analysis")
            assert response.status_code == 200
            assert response.json["analysis"]["relevanceReason"] == "Компания не найдена"
    assert analyze.call_count == 1
