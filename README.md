как запустить
1. клонировать репозиторий

```bash
git clone -b integration https://github.com/AlexWarn1ng/AlfaBank-Hackaton2026.git
cd AlfaBank-Hackaton2026
```

2. настроить переменные окружения

В корне проекта создать файл `.env` на основе `.env.example`.

НИКОГДА НИКУДА .env НЕ КИДАТЬ И НЕ КОММИТИТЬ


3. запустить докер
   
4. собрать и запустить сервисы

в корне репозитория выполнить:

```bash
docker compose up --build -d
```
последующие запуски без изменений кода:

```bash
docker compose up -d
```
полезные штуки

проверить доступность Java API:

```text
http://localhost:8888/api/company/test-uid
```

Ответ `404 Not Found` для несуществующей компании означает, что запрос достиг Java-сервиса.

**Просмотр логов всех сервисов:**

```bash
docker compose logs -f
```

**Просмотр логов конкретного сервиса:**

```bash
docker compose logs -f company-service
```
ну логи спокойно в самом приложении докера смотрятся так даж удобнее как будто

**Пересборка после изменения исходного кода:**

```bash
docker compose up --build -d
```

**Пересоздание контейнеров после изменения `.env`:**

```bash
docker compose up -d --force-recreate
```

**Остановка проекта:**

```bash
docker compose down
```

Команда `down` удаляет контейнеры и созданную Compose-сеть, но сохраняет собранные образы.
ну ваще проект и из ui докера можно остановить никто не запрещает
