# FASE 37 - Operational Mail - Release Notes

**Date:** 2026-03-26
**Version:** 0.37.0
**Status:** Completed

## Executive Summary

FASE 37 delivered the transactional email layer of the CCT standard library. Three modules cover email composition and delivery via SMTP, persistent queue management with retry semantics, and webhook parsing for delivery event normalization across mail service providers.

These modules provide a complete operational email pipeline — compose, send, queue, retry, and observe delivery outcomes — without depending on external SDKs.

## Scope Closed

Delivered subphases:

- **37A**: `cct/mail` — email composition, SMTP backends, and dev/test backends
- **37B**: `cct/mail_spool` — persistent mail queue with retry and dead-letter
- **37C**: `cct/mail_webhook` — delivery event normalization from provider webhooks

## Key Deliverables

### 37A — `cct/mail`

Email composition:

```cct
ADVOCARE "cct/mail.cct"

EVOCA SPECULUM NIHIL msg AD CONIURA mail_new()
CONIURA mail_set_from(msg, "sender@example.com")
CONIURA mail_set_to(msg, "recipient@example.com")
CONIURA mail_set_subject(msg, "Hello")
CONIURA mail_set_body_text(msg, "Plain text body")
CONIURA mail_set_body_html(msg, "<p>HTML body</p>")
CONIURA mail_add_attachment(msg, "/path/to/file.pdf", "report.pdf")

EVOCA SPECULUM NIHIL smtp AD CONIURA mail_smtp_open(config)
EVOCA SPECULUM NIHIL res AD CONIURA mail_send(smtp, msg)
CONIURA mail_smtp_close(smtp)
CONIURA mail_free(msg)
```

SMTP authentication modes: PLAIN, LOGIN, STARTTLS, SMTPS (SSL/TLS).

Configuration fields: `host`, `port`, `username`, `password`, `auth_mode`, `tls_verify`.

Development and test backends:

- **File backend** — writes emails as `.eml` files to a directory; no SMTP server needed
- **Memory backend** — holds emails in memory for inspection in test suites; `mail_memory_drain() REDDE FLUXUS GENUS(MailMessage)`

MIME construction:
- Multipart/mixed for attachments
- Multipart/alternative for text+HTML
- Inline attachments with Content-ID
- RFC 5322 header compliance

### 37B — `cct/mail_spool`

Persistent mail queue with state machine:

```
PENDING → SENT
        → FAILED → (retry) → SENT
                            → DEAD
```

API:

```cct
ADVOCARE "cct/mail_spool.cct"

EVOCA SPECULUM NIHIL spool AD CONIURA mail_spool_open(spool_dir)
EVOCA VERBUM id AD CONIURA mail_spool_enqueue(spool, msg)
EVOCA SPECULUM NIHIL entry AD CONIURA mail_spool_get(spool, id)
CONIURA mail_spool_mark_sent(spool, id)
CONIURA mail_spool_mark_failed(spool, id, "reason")
EVOCA FLUXUS GENUS(SpoolEntry) pending AD CONIURA mail_spool_list_pending(spool)
CONIURA mail_spool_drain_memory(spool, smtp)  -- attempt all PENDING
CONIURA mail_spool_retry_dead(spool)          -- move DEAD back to PENDING
CONIURA mail_spool_delete(spool, id)
CONIURA mail_spool_close(spool)
```

- Persistence format: one JSON file per message in `spool_dir/`
- Retry: exponential backoff, configurable max attempts before DEAD
- `mail_spool_list_pending` returns entries sorted by enqueue time
- Each `SpoolEntry` carries: `id`, `state`, `attempt_count`, `last_error`, `enqueued_at`, `last_attempt_at`

### 37C — `cct/mail_webhook`

Delivery event normalization from mail service provider webhooks:

```cct
ADVOCARE "cct/mail_webhook.cct"

EVOCA SPECULUM NIHIL event AD CONIURA mail_webhook_parse(provider, raw_body)
EVOCA VERBUM kind AD CONIURA mail_webhook_event_kind(event)  -- "delivered" | "bounce" | "complaint" | "open" | "click"
EVOCA VERBUM recipient AD CONIURA mail_webhook_recipient(event)
EVOCA VERBUM message_id AD CONIURA mail_webhook_message_id(event)
```

Supported providers: Mailgun, SendGrid (extensible via raw access).

Event kinds:
- `delivered` — message accepted by receiving MTA
- `bounce` — permanent delivery failure; includes bounce reason
- `complaint` — spam report from recipient
- `open` — message opened (pixel tracking)
- `click` — link clicked

MIME utilities included in 37C:
- `mail_mime_scan(VERBUM raw) REDDE SPECULUM NIHIL` — lightweight MIME scanner (no full parser)
- `mail_headers_parse(VERBUM raw) REDDE SPECULUM NIHIL` — RFC 5322 header block parser
- `mail_header_get(SPECULUM NIHIL headers, VERBUM name) REDDE VERBUM`

## Design Decisions

### File and Memory Backends for Testability

Production SMTP requires a live server. The file and memory backends allow test suites to verify email composition and delivery logic without network dependencies. The memory backend's `mail_memory_drain` returns the full message list for assertion in integration tests.

### Spool as Files, Not Database Rows

The spool stores each message as an individual JSON file. This avoids requiring a database for mail queue functionality, keeps the module self-contained, and allows manual inspection and intervention by operators without tooling.

### Webhook Normalization, Not Provider SDKs

FASE 37C normalizes the minimum semantically meaningful subset of webhook events across providers. Provider-specific raw access is available for fields outside the normalized model. This avoids the maintenance burden of tracking full provider API schemas.

## Host-Only Contract

All FASE 37 modules are host-only. SMTP tests that require a live server use the file or memory backend and skip with exit code 0 when `SMTP_HOST` is absent.

## Validation Summary

- ✅ `mail`: MIME multipart construction (text+HTML, with attachment)
- ✅ `mail`: RFC 5322 header compliance
- ✅ `mail`: file backend write and read-back
- ✅ `mail`: memory backend drain and inspect
- ✅ `mail`: SMTP LOGIN handshake test (requires `SMTP_HOST`)
- ✅ `mail_spool`: enqueue, get, mark-sent, mark-failed, list-pending, retry-dead, delete
- ✅ `mail_spool`: drain-memory sends all PENDING via backend
- ✅ `mail_spool`: state machine correctness
- ✅ `mail_webhook`: Mailgun delivery/bounce parse
- ✅ `mail_webhook`: SendGrid delivered/complaint parse
- ✅ `mail_webhook`: invalid payload rejection
- ✅ `mail_headers_parse`: RFC 5322 multi-value headers
- ✅ `mail_mime_scan`: multipart boundary detection
- ✅ Freestanding rejection tests for each module

## Breaking Changes

None. FASE 37 is purely additive.

## Runtime Dependencies

- **37A**: OpenSSL or equivalent TLS library for STARTTLS/SMTPS
- **37B**: No external dependencies (JSON Lines file I/O)
- **37C**: No external dependencies

---

**Next:** FASE 38 — Runtime Instrumentation (`instrument`, `context_local`)
