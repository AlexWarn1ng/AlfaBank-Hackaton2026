# Alfa News

Умный помощник по бизнес-новостям для хакатона Альфа-Банка.

Сервис позволяет пользователю зарегистрироваться по `F_uid`, выбрать должность и получить персональную ленту новостей. Для каждой новости доступен AI-анализ с рисками, возможностями и рекомендуемыми действиями.

## Возможности

- Регистрация и вход в аккаунт.
- Проверка `F_uid` через Java-сервис.
- Четыре поддерживаемые должности:
  - CEO
  - CFO
  - ChiefAccountant
  - HeadofTreasury
- Персональная новостная лента из PostgreSQL.
- Страница новости открывается сразу.
- AI-анализ подгружается отдельно с анимацией загрузки.
- При ошибке анализа доступна кнопка повторной попытки.
- Успешные AI-анализы кешируются в SQLite, поэтому при повторном открытии новости токены AI не расходуются.
- Данные frontend и кеш сохраняются в Docker volume.

## Стек

- Frontend: Flask, Jinja2, HTML/CSS
- Сервис подбора новостей: Java
- AI-анализатор: C# / ASP.NET
- База данных: PostgreSQL
- Контейнеризация: Docker Compose

## Запуск

### 1. Клонировать репозиторий

```bash
git clone -b integration https://github.com/AlexWarn1ng/AlfaBank-Hackaton2026.git
cd AlfaBank-Hackaton2026
```

### 2. Создать `.env`

В корне проекта создай файл `.env`:

```env
AI_API_KEY=
COMPANY_AI_API_KEY=
POSTGRES_HOST=
POSTGRES_PORT=5432
POSTGRES_DB=
POSTGRES_USER=
POSTGRES_PASSWORD=
```

`.env` нельзя коммитить или отправлять в репозиторий.

### 3. Собрать и запустить проект

```bash
docker compose up --build -d
```

После запуска:

- Frontend: http://localhost:5050
- Java-сервис: http://localhost:8888
- AI-анализатор: http://localhost:5001

### Тестовый пользователь

Для проверки регистрации можно использовать:

```text
F_uid: F_64255
Должность: CEO
```

## Проверка обновления frontend

```bash
docker compose exec frontend python -c "import requests; t=requests.get('http://127.0.0.1:5000/').text; print('f_uid:', 'name=\"f_uid\"' in t); print('company_id:', 'name=\"company_id\"' in t)"
```

Ожидаемый результат:

```text
f_uid: True
company_id: False
```

## Полезные команды

Пересобрать только frontend после изменения Flask или шаблонов:

```bash
docker compose up --build -d --no-deps frontend
```

Посмотреть логи:

```bash
docker compose logs -f
```

Логи конкретного сервиса:

```bash
docker compose logs -f frontend
docker compose logs -f company-service
docker compose logs -f news-analyzer
```

Остановить проект:

```bash
docker compose down
```

Полностью очистить контейнеры и данные frontend, включая SQLite-кеш и зарегистрированных пользователей:

```bash
docker compose down -v
```

Использовать `docker compose down -v` нужно только при необходимости.