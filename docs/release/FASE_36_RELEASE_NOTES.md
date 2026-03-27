# FASE 36 - Operational Database - Release Notes

**Date:** 2026-03-25
**Version:** 0.36.0
**Status:** Completed

## Executive Summary

FASE 36 delivered the operational database layer of the CCT standard library. Four modules add PostgreSQL client support, full-text search helpers, a Redis client, and PostgreSQL advisory locks for distributed coordination.

These modules follow the same host-only bridge pattern established by the SQLite client, providing a unified and consistent database API surface across different backends.

## Scope Closed

Delivered subphases:

- **36A**: `cct/db_postgres` — PostgreSQL client with prepared statements and transactions
- **36B**: `cct/db_postgres_search` — PostgreSQL full-text search helpers
- **36C**: `cct/redis` — Redis client with core command support
- **36D**: `cct/db_postgres_lock` — PostgreSQL advisory locks for distributed coordination

## Key Deliverables

### 36A — `cct/db_postgres`

```cct
ADVOCARE "cct/db_postgres.cct"

EVOCA SPECULUM NIHIL db AD CONIURA postgres_open(dsn)
EVOCA SPECULUM NIHIL stmt AD CONIURA postgres_prepare(db, sql)
CONIURA postgres_bind_text(stmt, 1, value)
EVOCA SPECULUM NIHIL rows AD CONIURA postgres_step(stmt)
EVOCA VERBUM col AD CONIURA postgres_column_text(rows, 0)
CONIURA postgres_finalize(stmt)
CONIURA postgres_close(db)
```

Key capabilities:
- `postgres_open(VERBUM dsn) REDDE Result GENUS(PostgresDb, VERBUM)`
- Prepared statements with typed bind: `bind_text`, `bind_int`, `bind_real`, `bind_bool`, `bind_null`
- Type-specific column accessors: `column_text`, `column_int`, `column_real`, `column_bool`, `column_json`
- `postgres_exec(db, sql)` — statement without result set
- Transaction control: `postgres_begin`, `postgres_commit`, `postgres_rollback`
- LISTEN/NOTIFY: `postgres_listen(db, channel)`, `postgres_notify_wait(db)`
- PostgreSQL-specific types: JSONB, ARRAY, UUID via text protocol

### 36B — `cct/db_postgres_search`

Full-text search helpers on top of `db_postgres`:

- `postgres_search_config(VERBUM dictionary) REDDE SPECULUM NIHIL` — stable FTS configuration handle
- `postgres_search_query(SPECULUM NIHIL cfg, VERBUM query) REDDE VERBUM` — generates `to_tsquery` expression
- `postgres_search_rank(SPECULUM NIHIL cfg, VERBUM tsvector_col, VERBUM query) REDDE VERBUM` — generates `ts_rank` SQL fragment
- `postgres_search_headline(SPECULUM NIHIL cfg, VERBUM text_col, VERBUM query) REDDE VERBUM` — generates `ts_headline` SQL fragment
- `postgres_search_document(VERBUM col1, VERBUM col2) REDDE VERBUM` — concatenated `to_tsvector` source
- GIN index management helpers: `postgres_search_ensure_index(db, table, col)`

### 36C — `cct/redis`

```cct
ADVOCARE "cct/redis.cct"

EVOCA SPECULUM NIHIL r AD CONIURA redis_open(dsn)
CONIURA redis_set(r, "key", "value", 300)
EVOCA VERBUM v AD CONIURA redis_get(r, "key")
CONIURA redis_close(r)
```

Key capabilities:
- `redis_open(VERBUM dsn) REDDE Result GENUS(Redis, VERBUM)`
- Strings: `redis_get`, `redis_set(key, value, ttl_seconds)`, `redis_del`, `redis_exists`, `redis_incr`
- Hashes: `redis_hget`, `redis_hset`, `redis_hdel`, `redis_hgetall`
- Lists: `redis_lpush`, `redis_rpush`, `redis_lpop`, `redis_rpop`, `redis_llen`
- Sets: `redis_sadd`, `redis_srem`, `redis_smembers`, `redis_sismember`
- Pub/Sub: `redis_publish`, `redis_subscribe`, `redis_receive`
- Escape hatch: `redis_raw(r, FLUXUS GENUS(VERBUM) args) REDDE VERBUM` — raw RESP command

DSN format: `redis://[:password@]host:port[/db]`

### 36D — `cct/db_postgres_lock`

PostgreSQL advisory locks for distributed coordination:

```cct
ADVOCARE "cct/db_postgres_lock.cct"

EVOCA SPECULUM NIHIL lock AD CONIURA postgres_lock_open(db, "job-processor")
CONIURA postgres_lock_acquire(lock)    -- blocks until acquired
-- critical section
CONIURA postgres_lock_release(lock)
CONIURA postgres_lock_close(lock)
```

Key capabilities:
- `postgres_lock_open(db, VERBUM name) REDDE SPECULUM NIHIL` — named lock handle
- `postgres_lock_acquire(lock)` — blocking acquisition
- `postgres_lock_try(lock) REDDE VERUM` — non-blocking try
- `postgres_lock_release(lock)` — explicit release
- Lock scopes: session-level (survives transaction) and transaction-level (auto-released on COMMIT/ROLLBACK)
- `WITH` pattern support: `postgres_lock_with(db, name, RITUALE callback)` — acquire, run, release

## Design Decisions

### API Symmetry with SQLite

The `db_postgres` API surface deliberately mirrors the existing `db_sqlite` module where semantically equivalent. Programs can switch backends with minimal API surface changes. Differences exist only where PostgreSQL semantics differ fundamentally (LISTEN/NOTIFY, JSONB, advisory locks).

### FTS as Builders, Not String Templates

`db_postgres_search` generates SQL fragments rather than executing queries directly. This allows callers to compose FTS conditions into larger queries without string concatenation, keeping the generated SQL correct and injection-free.

### Redis Raw Escape Hatch

The `redis_raw` function is intentional and documented. It ensures the module never becomes a bottleneck when applications need commands not yet covered by the CCT-level API. The wrapper functions are thin and consistent; `redis_raw` handles everything else.

## Host-Only Contract

All FASE 36 modules are host-only. A live database or Redis instance is required at test time. Tests that cannot connect (missing `DATABASE_URL` or `REDIS_URL` environment variables) skip with exit code 0 rather than failing.

## Validation Summary

- ✅ `db_postgres`: open, prepare, bind, step, column, finalize, close
- ✅ `db_postgres`: transaction commit/rollback correctness
- ✅ `db_postgres`: closed-handle rejection tests
- ✅ `db_postgres_search`: query/rank/headline fragment generation
- ✅ `db_postgres_search`: GIN index creation idempotency
- ✅ `redis`: string get/set/del/incr with TTL
- ✅ `redis`: RESP array and scalar parsing
- ✅ `redis`: closed-handle rejection
- ✅ `db_postgres_lock`: acquire/release, try-lock false on contention
- ✅ `db_postgres_lock`: lock-with pattern correctness
- ✅ Freestanding rejection tests for each module

## Breaking Changes

None. FASE 36 is purely additive.

## Runtime Dependencies

- **36A, 36B, 36D**: libpq (PostgreSQL client library) linked at build time
- **36C**: RESP protocol implemented in C, no external library required
- Live PostgreSQL / Redis instances required for integration tests; tests skip gracefully when unavailable

---

**Next:** FASE 37 — Operational Mail (`mail`, `mail_spool`, `mail_webhook`)
