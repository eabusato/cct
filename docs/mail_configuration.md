# CCT Mail Configuration Guide

## Overview

`cct/mail` provides the canonical mail transport types and send path, but application configuration must stay outside the library.

This is intentional.

A compiled CCT program must be able to:

- read SMTP settings from environment variables
- read mail settings from a config file
- overlay environment values on top of file values when needed
- build `MailBackendOptions` explicitly in application code

Nothing in the application should need to be hardcoded except the mapping logic you choose for your own config contract.

## What `cct/mail` Owns

`cct/mail` owns:

- `MailMessage`
- `MailBackendOptions`
- `mail_backend_smtp(...)`
- `mail_backend_file(...)`
- `mail_backend_memory()`
- MIME rendering
- SMTP delivery

`cct/mail` does not force:

- a fixed env prefix
- a fixed config file path
- a fixed config file format
- a fixed secrets strategy

Those decisions belong to the application.

## Recommended Configuration Contract

Recommended environment variable names:

- `APP_MAIL_BACKEND`
- `APP_MAIL_SMTP_HOST`
- `APP_MAIL_SMTP_PORT`
- `APP_MAIL_SMTP_USERNAME`
- `APP_MAIL_SMTP_PASSWORD`
- `APP_MAIL_SMTP_AUTH`
- `APP_MAIL_SMTP_STARTTLS`
- `APP_MAIL_SMTP_SMTPS`
- `APP_MAIL_FILE_OUT_DIR`

Recommended `ini` section:

```ini
[mail]
backend=smtp
host=smtp.example.com
port=587
username=mailer@example.com
password=secret
auth=login
starttls=true
smtps=false
file_out_dir=var/mail_out
```

Recommended precedence:

1. application defaults
2. config file
3. environment overlay
4. explicit runtime override in code

## Supported Backend Modes

`backend=smtp`
- real SMTP transport

`backend=file`
- writes rendered messages to disk
- useful for development and inspection

`backend=memory`
- stores messages in memory
- useful for tests

## Supported SMTP Auth Values

Accepted values for `auth`:

- `none`
- `plain`
- `login`
- `xoauth2`

For most transactional providers, `login` or `plain` is enough.

## Environment-Driven Example

This pattern is appropriate when the process is configured entirely by env.

```cct
ADVOCARE "cct/env.cct"
ADVOCARE "cct/mail.cct"
ADVOCARE "cct/option.cct"
ADVOCARE "cct/parse.cct"
ADVOCARE "cct/verbum.cct"

RITUALE app_mail_auth(VERBUM raw) REDDE MailAuthKind
  EVOCA VERBUM auth AD CONIURA to_lower(CONIURA trim(raw))
  SI CONIURA compare(auth, "plain") == 0
    REDDE MAIL_AUTH_PLAIN
  FIN SI
  SI CONIURA compare(auth, "none") == 0
    REDDE MAIL_AUTH_NONE
  FIN SI
  SI CONIURA compare(auth, "xoauth2") == 0
    REDDE MAIL_AUTH_XOAUTH2
  FIN SI
  REDDE MAIL_AUTH_LOGIN
EXPLICIT RITUALE

RITUALE app_mail_backend_from_env() REDDE MailBackendOptions
  EVOCA VERBUM backend AD CONIURA to_lower(CONIURA trim(CONIURA getenv("APP_MAIL_BACKEND")))
  SI CONIURA compare(backend, "file") == 0
    REDDE CONIURA mail_backend_file(CONIURA getenv("APP_MAIL_FILE_OUT_DIR"))
  FIN SI
  SI CONIURA compare(backend, "memory") == 0
    REDDE CONIURA mail_backend_memory()
  FIN SI

  EVOCA SPECULUM NIHIL port_opt AD CONIURA try_int(CONIURA getenv("APP_MAIL_SMTP_PORT"))
  EVOCA SPECULUM NIHIL tls_opt AD CONIURA try_bool(CONIURA to_lower(CONIURA trim(CONIURA getenv("APP_MAIL_SMTP_STARTTLS"))))

  REDDE CONIURA mail_backend_smtp(
    CONIURA getenv("APP_MAIL_SMTP_HOST"),
    CONIURA option_unwrap GENUS(REX)(port_opt),
    CONIURA getenv("APP_MAIL_SMTP_USERNAME"),
    CONIURA getenv("APP_MAIL_SMTP_PASSWORD"),
    CONIURA app_mail_auth(CONIURA getenv("APP_MAIL_SMTP_AUTH")),
    CONIURA option_unwrap GENUS(VERUM)(tls_opt)
  )
EXPLICIT RITUALE
```

## Config-File Example

This pattern is appropriate when the application owns a stable config file.

```cct
ADVOCARE "cct/config.cct"
ADVOCARE "cct/mail.cct"
ADVOCARE "cct/verbum.cct"

RITUALE app_mail_auth(VERBUM raw) REDDE MailAuthKind
  EVOCA VERBUM auth AD CONIURA to_lower(CONIURA trim(raw))
  SI CONIURA compare(auth, "plain") == 0
    REDDE MAIL_AUTH_PLAIN
  FIN SI
  SI CONIURA compare(auth, "none") == 0
    REDDE MAIL_AUTH_NONE
  FIN SI
  SI CONIURA compare(auth, "xoauth2") == 0
    REDDE MAIL_AUTH_XOAUTH2
  FIN SI
  REDDE MAIL_AUTH_LOGIN
EXPLICIT RITUALE

RITUALE app_mail_backend_from_cfg(SPECULUM NIHIL cfg) REDDE MailBackendOptions
  EVOCA VERBUM backend AD CONIURA to_lower(CONIURA trim(CONIURA config_get_or(cfg, "mail", "backend", "smtp")))

  SI CONIURA compare(backend, "file") == 0
    REDDE CONIURA mail_backend_file(CONIURA config_get_or(cfg, "mail", "file_out_dir", "var/mail_out"))
  FIN SI
  SI CONIURA compare(backend, "memory") == 0
    REDDE CONIURA mail_backend_memory()
  FIN SI

  REDDE CONIURA mail_backend_smtp(
    CONIURA config_get_or(cfg, "mail", "host", ""),
    CONIURA config_get_int(cfg, "mail", "port"),
    CONIURA config_get_or(cfg, "mail", "username", ""),
    CONIURA config_get_or(cfg, "mail", "password", ""),
    CONIURA app_mail_auth(CONIURA config_get_or(cfg, "mail", "auth", "login")),
    CONIURA config_get_bool(cfg, "mail", "starttls")
  )
EXPLICIT RITUALE
```

## Config File With Environment Overlay

If you want file-based configuration with environment overrides:

```cct
ADVOCARE "cct/config.cct"

RITUALE app_load_cfg(VERBUM path) REDDE SPECULUM NIHIL
  EVOCA SPECULUM NIHIL cfg AD CONIURA config_read_ini(path)
  CONIURA config_apply_env_prefix(cfg, "APP")
  REDDE cfg
EXPLICIT RITUALE
```

With this pattern, these env vars override `[mail]` keys:

- `APP_MAIL_BACKEND`
- `APP_MAIL_HOST`
- `APP_MAIL_PORT`
- `APP_MAIL_USERNAME`
- `APP_MAIL_PASSWORD`
- `APP_MAIL_AUTH`
- `APP_MAIL_STARTTLS`
- `APP_MAIL_SMTPS`
- `APP_MAIL_FILE_OUT_DIR`

This naming is derived from `config_apply_env_prefix(...)`, which expands `APP + section + key`.

## Practical Example: Zoho SMTP

Typical setup:

```bash
export APP_MAIL_BACKEND=smtp
export APP_MAIL_SMTP_HOST=smtp.zoho.com
export APP_MAIL_SMTP_PORT=587
export APP_MAIL_SMTP_USERNAME=noreply@yourdomain.com
export APP_MAIL_SMTP_PASSWORD=your-app-password
export APP_MAIL_SMTP_AUTH=login
export APP_MAIL_SMTP_STARTTLS=true
```

Then your CCT app builds the backend with `mail_backend_smtp(...)` and sends normally.

For Zoho production usage, prefer:

- app password instead of an account password
- a real sender domain
- SPF/DKIM configured in Zoho
- a sender address that matches your domain

## Validation Advice

Before using real production credentials, validate in three layers:

1. `memory` backend for pure application tests
2. `file` backend for rendered MIME inspection
3. real SMTP in staging

This repository already contains integration coverage for these configuration patterns:

- `tests/integration/mail_backend_env_smtp_37a.cct`
- `tests/integration/mail_backend_env_file_37a.cct`
- `tests/integration/mail_backend_config_smtp_37a.cct`
- `tests/integration/mail_backend_config_env_overlay_37a.cct`
- `tests/integration/mail_backend_config_invalid_37a.cct`

## Design Rule

The stable rule is:

- transport belongs in `cct/mail`
- configuration belongs in the application

That separation keeps the language runtime reusable while still allowing each compiled CCT program to define its own operational contract.
