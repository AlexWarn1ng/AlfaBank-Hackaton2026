# THIRD_PARTY_NOTICES — Alfa Insight

This document inventories third-party software, data categories, model APIs, and mocked/demo integrations. It is **not** a substitute for upstream license verification; the team must retain required license notices before final publication.

| Component / service | Use | Provenance / notes |
| --- | --- | --- |
| Python 3.12 | Frontend runtime | Official Python container image; license PSF |
| Flask, Werkzeug, Jinja2, MarkupSafe, click, itsdangerous, blinker | Web application, templates, sessions | Python package sources; review package licenses at pinned versions |
| Requests, psycopg (binary) | HTTP and PostgreSQL | Python package dependencies |
| SQLite | Frontend local analysis/feed cache | Python standard library SQLite bindings |
| Java 21 Eclipse Temurin, Maven 3.9 | Java service runtime/build | Eclipse Adoptium and Apache Maven distributions |
| Spring Boot 4.1.1, Spring Data JPA/Hibernate, Jackson | Java REST and persistence | Resolved by Maven; transitive dependencies must be checked |
| PostgreSQL JDBC driver | Java database access | Maven runtime dependency |
| Apache PDFBox 3.0.6 | PDF processing | Apache License 2.0 |
| .NET 9, ASP.NET Core, Microsoft.AspNetCore.OpenApi 9.0.9 | C# agent API | Microsoft .NET / NuGet packages |
| PostgreSQL 16 | Application data storage | Official PostgreSQL Docker image |
| Docker / Docker Compose | Container orchestration | External runtime prerequisite |
| Changellenge competition LLM gateway | External OpenAI-compatible inference endpoint | `https://changellenge.up.railway.app/v1/`; external access and an approved API key required. Not emulated offline. |
| `google/gemma-4-26b-a4b-it` | Java RSS enrichment | Case-permitted LLM (if RSS import is invoked) |
| `deepseek/deepseek-v4.1-flash` and other model IDs in `Agents/` | Agent planning/analysis/validation | Confirm exact IDs against deployed configuration and case whitelist before submission |
| Kommersant RSS | Optional live news import | `https://www.kommersant.ru/rss/section-economics.xml` and `/section-business.xml`; copyright retained by original publisher. Not needed for seeded demo. |
| Alfa-Bank business and analytical datasets | Case-provided anonymized sources | **Not included** in this demo pack; redistribution and source notices subject to organizer's permissions. |
| `db/init/02_demo_seed.sql` | Local deterministic demonstration | Completely synthetic examples authored for this packaging step, explicitly marked `ДЕМО`; **not** real company, market, or bank research. |
| Alfa-Bank internet banking UI | Case-inspired product interface | Prototype only; **not integrated with a real bank login, account, or transaction service**. |

Before deadline: verify exact licenses in the dependency tree; verify allowed LLM model IDs in every service; obtain permission before redistributing any external/bank source data. No payment, identity, or bank API is connected.
