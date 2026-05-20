# HA Box vs Home Assistant Apps (Add-ons) Best Practices

This document compares the HA Box implementation to the official Home Assistant developer documentation for Apps (formerly add-ons): [Developing an app](https://developers.home-assistant.io/docs/apps/).

References:
- [Home Assistant Developer Docs](https://developers.home-assistant.io/)
- [Add-on configuration](https://developers.home-assistant.io/docs/add-ons/configuration/)
- [Add-on security](https://developers.home-assistant.io/docs/add-ons/security/)
- [Example app repository](https://github.com/home-assistant/addons-example)

---

## 1. Structure and Core Files

| Requirement | Status | Notes |
|-------------|--------|-------|
| `config.yaml` at App root | OK | Present with name, version, slug, description, url, arch |
| `Dockerfile` using `ARG BUILD_FROM` | OK | Uses `FROM $BUILD_FROM`, standard base |
| Startup script | OK | s6-overlay `run` script under `rootfs/etc/s6-overlay/s6-rc.d/ha-box/run` (modern approach) |
| `build.yaml` for builder | OK | Present with `build_from`, labels, args |
| Single `config.yaml` for App config | OK | No other `config.yaml` in repo that could be picked up by Supervisor recursive search |

**Recommendation:** Keep only one `config.yaml` in the App directory; avoid naming other files `config.yaml` elsewhere in the repo.

---

## 2. config.yaml

| Item | Status | Notes |
|------|--------|------|
| `name` | OK | "HA Box" |
| `version` | OK | String, e.g. "0.1.36" |
| `slug` | OK | "ha-box", unique identifier |
| `description` | OK | Present |
| `url` | OK | Points to `https://github.com/LeKaZeD/ha-box` |
| `arch` | OK | `aarch64` (Raspberry Pi); consider adding `amd64` for local dev if supported |
| `options` | OK | Default values for all configuration |
| `schema` | OK | Validation for all options (str?, int(), list(), match(), etc.) |
| `init` | OK | `false` (App does not need to run host init) |
| `startup` | OK | `services` |
| `boot` | OK | `auto` |
| `apparmor` | OK | `true` |
| `hassio_api` | OK | `true` (Supervisor API) |
| `homeassistant_api` | OK | `true` (Core API via Supervisor proxy) |
| `map` | OK | `share:rw` as needed |
| `devices` | OK | UART devices listed; minimal set |
| `gpio` / `kernel_modules` | OK | `false` when not required |

**Gaps:**
- Ensure `version` in config.yaml stays in sync with tagged image and CHANGELOG when publishing.

---

## 3. Security (AppArmor)

| Item | Status | Notes |
|------|--------|------|
| `apparmor.txt` present | OK | Custom profile in App root |
| Profile name matches App | OK | `profile ha-box` |
| Least privilege | OK | Only required paths and devices (serial, network, /data/options.json, /addon/**, etc.) |
| No broad filesystem access | OK | No `/** rw` or host mount |
| Documented in SECURITY.md | OK | SECURITY.md describes rating and permissions |

**Recommendation:** Any new device (e.g. `/dev/spidev0.1`, additional GPIO chips) or path required by the app must be added explicitly to `apparmor.txt` and reflected in SECURITY.md. Current devices: UART (`/dev/ttyAMA0`, `/dev/serial0`), GPIO chardev (`/dev/gpiochip0`), SPI for RF433 (`/dev/spidev0.0`).

---

## 4. Communication with Supervisor and Core

| Item | Status | Notes |
|------|--------|------|
| API access via Supervisor | OK | Uses `SUPERVISOR_TOKEN` and `SUPERVISOR_URL` (with fallback) in Python |
| No hardcoded credentials | OK | Token from environment only |
| Core availability not assumed at startup | OK | Documented in .cursorrules; code checks Core before API calls |
| Graceful degradation | OK | Log and continue when Core is unavailable |

**Recommendation:** Keep using environment variables for Supervisor/Core access; do not add hardcoded tokens or URLs for production.

---

## 5. Lifecycle and Startup

| Item | Status | Notes |
|------|--------|------|
| Startup script shebang | OK | `#!/usr/bin/with-contenv bash` |
| Python unbuffered output | OK | `PYTHONUNBUFFERED=1` and `python3 -u` in run script |
| No assumption that Core is ready at boot | OK | Retries and periodic checks in code |
| Clean exit / finish script | OK | `finish` script logs exit code |

**Note:** The run script does not use `bashio`; it only exec's Python. If future logic is added in shell (e.g. reading options, logging), use `bashio` as per HA examples and .cursorrules.

---

## 6. Internationalization and Presentation

| Item | Status | Notes |
|------|--------|------|
| Translations folder | OK | `translations/` with `en.yaml` and `fr.yaml` |
| Configuration labels/descriptions | OK | Nested structure in translation files for UI |
| Runtime i18n in Python | OK | `core/i18n.py` with language detection (LANG, SUPERVISOR_LANGUAGE) |

**Recommendation:** When adding new options, add corresponding keys to both `config.yaml` schema and translation files (en/fr).

---

## 7. Documentation and Metadata

| Item | Status | Notes |
|------|--------|------|
| README.md | OK | Installation, configuration, hardware, license |
| CHANGELOG | OK | Present with link to HA keeping-a-changelog docs |
| SECURITY.md | OK | Security rating and permissions |
| build.yaml labels | OK | OCI image title, description, source, licenses |
| Commented `image` in config | OK | Optional for pre-built image; fine when building locally |

**Gaps:**
- Terminology aligned with HA: "App" for the product; "add-on" only for config keys and legacy/doc URLs.
- No dedicated DOCS.md; configuration is documented via README and translation labels. Optional: add a short DOCS.md for the store if required by publishing checklist.

---

## 8. Build and Distribution

| Item | Status | Notes |
|------|--------|------|
| build.yaml `build_from` | OK | aarch64 base image specified |
| Single architecture | OK | aarch64 only (Raspberry Pi target) |
| Tempio in Dockerfile | OK | Installed for potential future templating |
| requirements.txt | OK | Pinned dependencies |

**Recommendation:** If the app is ever ported to amd64 (e.g. for dev), add `amd64` to `build_from` in build.yaml and to `arch` in config.yaml, and document any arch-specific behavior.

---

## 9. Summary

- **Respected:** config structure, schema, options, security (AppArmor), Supervisor/Core API usage, startup and lifecycle, translations, and documentation (README, CHANGELOG, SECURITY).
- **To fix before publishing:** Replace placeholder `url` in config.yaml.
- **Optional improvements:** Align terminology with "App" where referring to HA docs; keep version and CHANGELOG in sync; consider DOCS.md and multi-arch if needed for store submission.

HA Box is aligned with Home Assistant App recommendations; the main actionable item is replacing the placeholder URL and maintaining version/changelog consistency.
