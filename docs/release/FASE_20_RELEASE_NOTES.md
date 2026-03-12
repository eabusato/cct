# CCT — Release Notes: FASE 20

**Date**: March 11, 2026  
**Compiler version**: FASE 20F  
**Tests**: 1181 passed / 0 failed (full suite)

---

## Summary

FASE 20 closes the first application-stack expansion of CCT:

- `cct/json` for canonical JSON values, parsing, stringify/pretty-print, and navigation.
- `cct/socket` / `cct/net` for host-only TCP/UDP transport and text-oriented network helpers.
- `cct/http` for HTTP/1.1 request/response modeling, parsing, client flows, and single-request server primitives.
- `cct/config` for INI configuration parsing, typed access, writing, env overlays, and JSON bridging.
- `cct/db_sqlite` for local persistence through a thin SQLite bridge with cursors, prepared statements, transactions, and scalar helpers.

The phase also closes normative documentation, canonical examples, and handoff material for the new stack.

---

## 1) Delivered Modules

### `cct/json`

What you gain:
- canonical JSON value construction (`json_obj`, `json_arr`, `json_str`, `json_num`, `json_bool`, `json_null`)
- strict parsing (`json_parse`, `json_parse_file`, `json_try_parse*`)
- deterministic stringify (`json_stringify`, `json_stringify_pretty*`)
- navigation and mutation (`json_get_key`, `json_get_index`, `json_set_key`, `json_set_index`)
- typed expect helpers (`json_expect_*`)

### `cct/socket` / `cct/net`

What you gain:
- host-only socket bridge without hiding transport details
- canonical TCP/UDP wrappers (`tcp_connect`, `tcp_listen`, `udp_bind`, `udp_send_to`, `udp_recv_from`)
- textual framing helpers (`net_read_line`, `net_write_line`, `net_read_exact`)
- address parsing and endpoint introspection support

### `cct/http`

What you gain:
- structured `HttpRequest` / `HttpResponse` modeling
- HTTP parse/stringify helpers
- local HTTP client workflows (`http_get`, `http_post`, `http_get_json`)
- single-request server primitives (`http_server_listen`, `http_server_accept`, `http_server_reply`)

### `cct/config`

What you gain:
- INI parse/load/write flows
- typed access (`config_get_int`, `config_get_bool`, `config_get_real`)
- section enumeration and mutation
- environment overlay (`config_apply_env_prefix`)
- JSON/config bridge helpers

### `cct/db_sqlite`

What you gain:
- open/exec/query flows
- row iteration helpers
- prepared statements and typed bind
- transaction helpers
- scalar convenience helpers

SQLite policy:
- host-only in the current subset
- generated host builds link `-lsqlite3` only when SQLite builtins are used
- if the host toolchain lacks SQLite, compilation fails clearly instead of silently downgrading behavior

---

## 2) Examples Added

Canonical examples published in `examples/`:

- `http_simple_server_20f2.cct`
- `http_json_client_20f2.cct`
- `config_sqlite_app_20f2.cct`
- `udp_loopback_demo_20f2.cct`

These examples are project-local and keep generated/runtime artifacts inside the repository tree.

---

## 3) Host vs Freestanding Boundary

Stable host-only application modules in the current subset:

- `cct/socket`
- `cct/net`
- `cct/http`
- `cct/config`
- `cct/db_sqlite`

Freestanding remains focused on the bridge/kernel-oriented subset introduced in FASE 16 and is intentionally not expanded with the FASE 20 application stack.

---

## 4) Test Closure

Test count added in FASE 20:

| Subphase group | Fixtures | Status |
|---|---:|---|
| 20A (`json`) | 18 | PASS |
| 20B (`socket` / `net`) | 13 | PASS |
| 20C (`http`) | 11 | PASS |
| 20D (`config`) | 8 | PASS |
| 20E (`db_sqlite`) | 11 | PASS |
| 20F (`docs/examples/release`) | 0 | PASS |
| **Total FASE 20** | **61** | **PASS** |
| **Global cumulative** | **1181** | **PASS** |

Final gate:
- `make test`
- Result: **1181 passed / 0 failed**

---

## 5) Architectural Decisions

1. High-level JSON/HTTP/config behavior stays in CCT unless a direct host boundary is unavoidable.
2. Socket and SQLite runtime additions stay narrow and are emitted only for generated host programs that actually use them.
3. HTTP in this phase intentionally targets deterministic HTTP/1.1 text workflows only; TLS/HTTPS and HTTP/2+ remain out of scope.
4. Config keeps INI as the canonical textual format and uses JSON only as an integration bridge, not as the primary config syntax.
5. SQLite keeps explicit ownership boundaries: DB handles, rows handles, and statement handles each have explicit close/finalize calls.
6. Example artifacts are kept project-local; no `/tmp` requirement is introduced by the phase examples.

---

## 6) Out of Scope

- TLS / HTTPS
- HTTP/2 or WebSocket
- remote database drivers or ORM layers
- asynchronous/event-loop networking framework
- broader freestanding expansion of the application stack

---

## 7) Forward Look

With FASE 20 closed, the roadmap moves to FASE 21 for richer generic data modeling and collection ergonomics over the now-stabilized application baseline.
