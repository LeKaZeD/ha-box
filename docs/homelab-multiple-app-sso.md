# Homelab Documentation — Self-Hosted Stack with SSO

## Introduction

This guide documents the setup of a complete homelab on a Raspberry Pi 5 running Home Assistant OS. The goal is to self-host several services (media, photos, recipes, home automation, passwords) accessible from the internet in a secure way, with centralized authentication via SSO (Single Sign-On).

### What we are setting up

- **Cloudflare Tunnel + Zero Trust** — secure exposure of services without opening ports on your router. All traffic goes through Cloudflare, your IP stays hidden.
- **NPM (Nginx Proxy Manager)** — local reverse proxy that routes requests to the right services.
- **LLDAP** — lightweight user directory. Single source of truth for all family accounts.
- **Authelia** — SSO authentication portal. All services authenticate through Authelia, which verifies identities against LLDAP.
- **OIDC (OpenID Connect)** — standard authentication protocol. Each service delegates login to Authelia instead of managing its own accounts.

### Why this architecture?

Without SSO, each service has its own accounts. Adding a family member means creating an account in every app separately. With this setup, you create a user once in LLDAP and they have access to all configured services automatically.

### Important limitations — Cloudflare Zero Trust

WARNING: Cloudflare Zero Trust protects web access but blocks mobile applications and API access.

Mobile apps (Immich, Mealie, Jellyfin) cannot authenticate against Cloudflare Access. To use them:
- Immich mobile — generate an API token from the web interface and use it in the app
- Mealie mobile — same approach, API token from the profile settings
- Jellyfin mobile — use the app on the local network, or create a Cloudflare Access bypass for the Jellyfin subdomain if external access is required

An alternative is to not enable Cloudflare Access on services intended for mobile apps, and rely solely on the service's own OIDC authentication.

### Network architecture

```
Internet (HTTPS)
    -> Cloudflare Tunnel
        -> NPM :80 (internal reverse proxy)
            -> Authelia      :9091
            -> Mealie        :9090
            -> Jellyfin      :8096
            -> Immich        :8181
            -> Vaultwarden   :7277
            -> Home Assistant :80
            -> Uptime Kuma   :3001
```

OIDC authentication flow:
```
User
    -> Service (Mealie, Jellyfin, etc.)
        -> Authelia (identity verification)
            -> LLDAP (user directory)
                -> Access granted
```

---

## Prerequisites

- Raspberry Pi 5 with Home Assistant OS
- Domain managed on Cloudflare (e.g. `example.com`)
- Cloudflare account (free plan is sufficient)
- SSH or terminal access on HA

---

## 1. Cloudflare Tunnel

### Creating the tunnel
1. Cloudflare Zero Trust -> Networks -> Tunnels -> Create a tunnel
2. Install the Cloudflare addon on HA from the addon store
3. The tunnel points everything to NPM: `http://192.168.1.X:80`

### Subdomains to configure in the tunnel
| Subdomain | Local destination |
|-----------|------------------|
| auth.example.com | http://192.168.1.X:80 |
| mealie.example.com | http://192.168.1.X:80 |
| jellyfin.example.com | http://192.168.1.X:80 |
| immich.example.com | http://192.168.1.X:80 |
| bit.example.com | http://192.168.1.X:80 |
| ha.example.com | http://192.168.1.X:80 |
| uptime.example.com | http://192.168.1.X:80 |
| home.example.com | http://192.168.1.X:80 |

### Cloudflare Access — two applications

**Application 1 — Bypass for technical endpoints**

Create a Self-hosted application with a Bypass policy for the following routes (required for OIDC and monitoring):

| Subdomain | Path | Reason |
|-----------|------|--------|
| auth.example.com | `/.well-known` | OIDC discovery |
| auth.example.com | `/api/oidc` | Token exchange |
| auth.example.com | `/jwks.json` | Public keys |
| uptime.example.com | `/api/status-page/heartbeat` | Monitoring dashboard API |

**Application 2 — Service protection**

Create a Self-hosted application for `*.example.com` with an authentication policy (Google SSO, email OTP, etc.). This protects web access.

Note: Do not enable Cloudflare Access on subdomains used by mobile apps, or create a bypass for those subdomains if you still want to protect web access.

---

## 2. NPM (Nginx Proxy Manager)

### HA Addon
Search for "Nginx Proxy Manager" in the HA addon store.

### SSL Certificate
Import the Cloudflare Origin Certificate (wildcard `*.example.com`) in NPM -> SSL Certificates.

### X-Forwarded-Proto header (required on all services)
Without this header, services generate redirect_uris with `http://` instead of `https://`, which breaks OIDC. Add in the Advanced tab of each Proxy Host:

```nginx
location / {
    proxy_pass http://192.168.1.X:PORT;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto https;
    proxy_set_header X-Forwarded-Host $http_host;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
}
```

### Service ports
| Service | Port |
|---------|------|
| Authelia | 9091 |
| Mealie | 9090 |
| Jellyfin | 8096 |
| Immich | 8181 |
| Vaultwarden | 7277 |
| Home Assistant | 80 (via NPM redirect) |
| Uptime Kuma | 3001 |

### Static dashboard (home.example.com)
Place `index.html` in `/addon_configs/[npm_addon_id]/www/index.html`

NPM Advanced config for `home.example.com`:
```nginx
location / {
    root /config/www;
    index index.html;
    try_files $uri $uri/ /index.html;
}
```

### CORS for Uptime Kuma
Add in the Advanced tab of the `uptime.example.com` Proxy Host:
```nginx
location /api/status-page/heartbeat {
    proxy_pass http://192.168.1.X:3001/api/status-page/heartbeat;
    add_header Access-Control-Allow-Origin "https://home.example.com";
    add_header Access-Control-Allow-Methods "GET";
}
```

---

## 3. LLDAP

### HA Addon
Repo: https://github.com/lldap/lldap  
Community addon — search `LLDAP` in the store or add the repo `https://github.com/alexbelgium/hassio-addons`.

### Access
- Web interface: `http://192.168.1.X:17170`
- LDAP port: `3890`

### Groups to create
From the LLDAP web interface -> Groups:
- `mealie-admins`
- `mealie-users`
- `jellyfin-admins`
- `jellyfin-users`

Note: In LLDAP, members are added from the group page, not from the user page.

---

## 4. Authelia

### HA Addon
Repo: https://github.com/alexbelgium/hassio-addons

### Generating secrets

### Generating secrets for each OIDC service

For each OIDC service, two distinct values are needed:
- The **plaintext secret** — to place in the service configuration (Mealie, Jellyfin, etc.) and store in a password manager.
- The **hashed secret** — to place in the corresponding Authelia client.

```bash
# Step 1 — Generate the plaintext secret
openssl rand -hex 32
# Example output: a3f8c2d1e4b7...
# Save this value in your password manager

# Step 2 — Generate the hash from the plaintext secret
docker exec -it $(docker ps | grep authelia | awk '{print $1}') \
  authelia crypto hash generate pbkdf2 --password "YOUR_PLAINTEXT_SECRET"
# Example output: $pbkdf2-sha512$310000$...
# This hash goes in configuration.yml for Authelia
```

Repeat this operation for each service:

| Service | Plaintext secret -> | Hash -> |
|---------|-------------------|---------|
| Mealie | `OIDC_CLIENT_SECRET` variable in the Mealie addon | `client_secret` of the `mealie` client in Authelia |
| Jellyfin | "OID Secret" field in the SSO-Auth plugin | `client_secret` of the `jellyfin` client in Authelia |
| Immich | "Client Secret" field in Immich OAuth settings | `client_secret` of the `immich` client in Authelia |
| Vaultwarden | SSO variable on the Vaultwarden side | `client_secret` of the `vaultwarden` client in Authelia |

> Home Assistant does not have a client secret — the client is configured with `public: true` in Authelia.


```bash
# JWKS key — PKCS8 format required (BEGIN PRIVATE KEY, not BEGIN RSA PRIVATE KEY)
openssl genrsa -out /homeassistant/oidc.key 4096
openssl pkcs8 -topk8 -inform PEM -outform PEM -nocrypt \
  -in /homeassistant/oidc.key -out /homeassistant/oidc_pkcs8.key

# Verify the format
head -1 /homeassistant/oidc_pkcs8.key
# Must show: -----BEGIN PRIVATE KEY-----

# HMAC secret
openssl rand -hex 32

# Client secret for each service
openssl rand -hex 32

# Hash the client secret (required in Authelia config)
docker exec -it $(docker ps | grep authelia | awk '{print $1}') \
  authelia crypto hash generate pbkdf2 --password "YOUR_SECRET"
```

### configuration.yml
File to place at `/homeassistant/configuration.yml`:

```yaml
log:
  level: info
theme: auto

authentication_backend:
  ldap:
    address: 'ldap://192.168.1.X:3890'
    base_dn: 'dc=example,dc=com'
    additional_users_dn: 'ou=people'
    users_filter: '(&({username_attribute}={input})(objectclass=person))'
    additional_groups_dn: 'ou=groups'
    groups_filter: '(member={dn})'
    user: 'uid=admin,ou=people,dc=example,dc=com'
    password: 'LLDAP_ADMIN_PASSWORD'
    attributes:
      username: 'uid'
      group_name: 'cn'
      mail: 'mail'
      display_name: 'displayName'

session:
  cookies:
    - domain: example.com
      authelia_url: "https://auth.example.com"
      default_redirection_url: "https://mealie.example.com"

notifier:
  filesystem:
    filename: /config/emails.txt

access_control:
  default_policy: deny
  rules:
    - domain: "mealie.example.com"
      policy: one_factor
    - domain: "jellyfin.example.com"
      policy: one_factor
    - domain: "immich.example.com"
      policy: one_factor
    - domain: "ha.example.com"
      policy: one_factor
    - domain: "bit.example.com"
      policy: one_factor

regulation:
  max_retries: 3
  find_time: "2 minutes"
  ban_time: "5 minutes"

server:
  buffers:
    read: 16384
    write: 16384
  headers:
    csp_template: "frame-ancestors 'self' https://ha.example.com"

definitions:
  user_attributes:
    vaultwarden_roles:
      expression: '"vaultwarden_admins" in groups ? ["admin"] : "vaultwarden_users" in groups ? ["user"] : [""]'

identity_providers:
  oidc:
    hmac_secret: "YOUR_HMAC_SECRET"
    jwks:
      - key: |
          -----BEGIN PRIVATE KEY-----
          CONTENT_OF_oidc_pkcs8.key
          -----END PRIVATE KEY-----
    claims_policies:
      mealie_policy:
        id_token:
          - 'email'
          - 'name'
          - 'groups'
      vaultwarden_policy:
        id_token:
          - 'vaultwarden_roles'
        custom_claims:
          vaultwarden_roles: {}
    scopes:
      vaultwarden:
        claims:
          - 'vaultwarden_roles'
    clients:
      - client_id: 'mealie'
        client_name: 'Mealie'
        client_secret: 'PBKDF2_HASH'
        claims_policy: 'mealie_policy'
        consent_mode: 'implicit'
        public: false
        authorization_policy: 'one_factor'
        require_pkce: true
        pkce_challenge_method: 'S256'
        redirect_uris:
          - 'https://mealie.example.com/login'
        scopes: ['openid','email','profile','groups']
        response_types: ['code']
        grant_types: ['authorization_code']
        access_token_signed_response_alg: 'none'
        userinfo_signed_response_alg: 'none'
        token_endpoint_auth_method: 'client_secret_basic'

      - client_id: 'jellyfin'
        client_name: 'Jellyfin'
        client_secret: 'PBKDF2_HASH'
        claims_policy: 'mealie_policy'
        consent_mode: 'implicit'
        public: false
        authorization_policy: 'one_factor'
        require_pkce: true
        pkce_challenge_method: 'S256'
        redirect_uris:
          - 'https://jellyfin.example.com/sso/OID/redirect/authelia'
        scopes: ['openid','profile','groups']
        response_types: ['code']
        grant_types: ['authorization_code']
        access_token_signed_response_alg: 'none'
        userinfo_signed_response_alg: 'none'
        token_endpoint_auth_method: 'client_secret_post'

      - client_id: 'immich'
        client_name: 'Immich'
        client_secret: 'PBKDF2_HASH'
        claims_policy: 'mealie_policy'
        consent_mode: 'implicit'
        public: false
        authorization_policy: 'one_factor'
        require_pkce: false
        redirect_uris:
          - 'https://immich.example.com/auth/login'
          - 'https://immich.example.com/user-settings'
          - 'app.immich:///oauth-callback'
        scopes: ['openid','profile','email']
        response_types: ['code']
        grant_types: ['authorization_code']
        access_token_signed_response_alg: 'none'
        userinfo_signed_response_alg: 'none'
        token_endpoint_auth_method: 'client_secret_post'

      - client_id: 'vaultwarden'
        client_name: 'Vaultwarden'
        client_secret: 'PBKDF2_HASH'
        claims_policy: 'vaultwarden_policy'
        consent_mode: 'implicit'
        public: false
        authorization_policy: 'one_factor'
        require_pkce: true
        pkce_challenge_method: 'S256'
        redirect_uris:
          - 'https://bit.example.com/identity/connect/oidc-signin'
        scopes: ['openid','offline_access','profile','email','vaultwarden']
        response_types: ['code']
        grant_types: ['authorization_code','refresh_token']
        access_token_signed_response_alg: 'none'
        userinfo_signed_response_alg: 'none'
        token_endpoint_auth_method: 'client_secret_basic'

      - client_id: 'homeassistant'
        client_name: 'Home Assistant'
        public: true
        require_pkce: true
        pkce_challenge_method: 'S256'
        consent_mode: 'implicit'
        authorization_policy: 'one_factor'
        redirect_uris:
          - 'https://ha.example.com/auth/oidc/callback'
          - 'http://ha.example.com/auth/oidc/callback'
        scopes: ['openid','profile','groups']
        id_token_signed_response_alg: 'RS256'
```

---

## 5. Mealie

### HA Addon
Repo: https://github.com/alexbelgium/hassio-addons

### Environment variables
```
OIDC_AUTH_ENABLED=true
OIDC_SIGNUP_ENABLED=true
OIDC_CONFIGURATION_URL=https://auth.example.com/.well-known/openid-configuration
OIDC_CLIENT_ID=mealie
OIDC_CLIENT_SECRET=YOUR_PLAINTEXT_SECRET
OIDC_AUTO_REDIRECT=false
OIDC_ADMIN_GROUP=mealie-admins
OIDC_USER_GROUP=mealie-users
BASE_URL=https://mealie.example.com
ALLOW_PASSWORD_LOGIN=false
FORWARDED_ALLOW_IPS=*
```

Mobile app note: Mealie uses session cookies that are incompatible with Cloudflare Access in mobile contexts. Generate an API token from Profile -> API Tokens and use it in the mobile app instead.

---

## 6. Jellyfin

### SSO Plugin
Plugin repo: https://github.com/9p4/jellyfin-plugin-sso

1. In Jellyfin -> Administration -> Plugins -> Repositories, add:
   `https://raw.githubusercontent.com/9p4/jellyfin-plugin-sso/manifest-release/manifest.json`
2. Install SSO-Auth from the Catalog and restart Jellyfin

### SSO-Auth configuration
Administration -> Plugins -> SSO-Auth -> Settings:
- **Provider name**: `authelia` (exact, case-sensitive)
- **OID Endpoint**: `https://auth.example.com`
- **OID Client ID**: `jellyfin`
- **OID Secret**: plaintext secret
- **Enable Authorization**: enabled
- **Enable All Folders**: enabled
- **Roles**: `jellyfin-users`, `jellyfin-admins`
- **Admin Roles**: `jellyfin-admins`
- **Role Claim**: `groups`
- **OID Scopes**: `groups`
- **Disable Pushed Authorization**: enabled
- **Scheme Override**: `https`

### Login button
Administration -> General -> Branding -> Login disclaimer:
```html
<form action="https://jellyfin.example.com/sso/OID/start/authelia">
  <button class="raised block emby-button button-submit">
    Sign in with Authelia
  </button>
</form>
```

Mobile app note: The Jellyfin mobile app does not go through Cloudflare Access. Either disable Cloudflare Access on `jellyfin.example.com`, or use the app on the local network only.

---

## 7. Immich

### HA Addon
Repo: https://github.com/alexbelgium/hassio-addons

### Sentinel files (.immich) — required
The Immich addon checks for `.immich` files in its folders at startup. Without them, Immich refuses to start. The addon may delete them on restart, so this script recreates them automatically.

Create `/addon_configs/[immich_addon_id]/immich.sh`:
```bash
#!/command/with-contenv bashio
# shellcheck shell=bash

mkdir -p /media/Media/Immich/encoded-video
mkdir -p /media/Media/Immich/thumbs
mkdir -p /media/Media/Immich/upload
mkdir -p /media/Media/Immich/backups
mkdir -p /media/Media/Immich/profile
mkdir -p /media/Media/Immich/library
touch /media/Media/Immich/encoded-video/.immich
touch /media/Media/Immich/thumbs/.immich
touch /media/Media/Immich/upload/.immich
touch /media/Media/Immich/backups/.immich
touch /media/Media/Immich/profile/.immich
touch /media/Media/Immich/library/.immich
```
```bash
chmod +x /addon_configs/[immich_addon_id]/immich.sh
```

Adjust the path `/media/Media/Immich/` according to the `data_location` configured in the addon.

### OIDC configuration
Administration -> Settings -> OAuth:
- **Issuer URL**: `https://auth.example.com/.well-known/openid-configuration`
- **Client ID**: `immich`
- **Client Secret**: plaintext secret
- **Scope**: `openid profile email`
- **Button Text**: `Login with Authelia`
- **Auto Register**: enabled
- **Mobile Redirect URI Override**: enabled

Mobile app note: The Immich app natively supports OIDC via `app.immich:///oauth-callback`, which is already configured in Authelia. However Cloudflare Access may block the flow. If issues arise, use an API token from Account -> API Keys.

---

## 8. Home Assistant

### OIDC integration
HACS repo: https://github.com/christiaangoossens/hass-oidc-auth

Install `hass-oidc-auth` via HACS, then in `configuration.yaml`:
```yaml
auth_oidc:
  client_id: "homeassistant"
  discovery_url: "https://auth.example.com/.well-known/openid-configuration"

http:
  use_x_forwarded_for: true
  trusted_proxies:
    - 127.0.0.1
    - 172.30.32.0/24
    - 172.30.33.0/24
```

The username in LLDAP must match the username of the existing HA account for automatic account linking.

---

## 9. Uptime Kuma

### HA Addon
Available in the HA community addon store.  
Repo: https://github.com/hassio-addons/addon-uptime-kuma

### Recommended monitors
**Services group (HTTP type):**
- Home Assistant -> `https://ha.example.com`
- Mealie -> `https://mealie.example.com`
- Jellyfin -> `https://jellyfin.example.com`
- Immich -> `https://immich.example.com`
- Vaultwarden -> `https://bit.example.com`
- Dashboard -> `https://home.example.com`

**Administration group (HTTP type):**
- Authelia -> `https://auth.example.com`

**Administration group (TCP Port type):**
- LLDAP -> `192.168.1.X:3890`
- NPM -> `192.168.1.X:81`

**Infrastructure group (TCP Port type):**
- SSH -> `192.168.1.X:22`
- Mosquitto -> `192.168.1.X:1883`
- PostgreSQL -> `192.168.1.X:5432`
- Samba -> `192.168.1.X:445`
- Matter Server -> `192.168.1.X:5580`

### Status Page
Create a public status page with slug `home`. Enable Cloudflare Access bypass on `/api/status-page/heartbeat` so the dashboard can query the API.

---

## Troubleshooting

### Authelia won't start — Invalid Credentials LDAP
The LLDAP admin password in `configuration.yml` does not match. Verify by logging into `http://192.168.1.X:17170` with that password.

### Authelia won't start — invalid JWKS key
The key must be in PKCS8 format:
```bash
head -1 /homeassistant/oidc_pkcs8.key
# Must show: -----BEGIN PRIVATE KEY-----
# And NOT: -----BEGIN RSA PRIVATE KEY-----
```
If not, reconvert:
```bash
openssl pkcs8 -topk8 -inform PEM -outform PEM -nocrypt \
  -in /homeassistant/oidc.key -out /homeassistant/oidc_pkcs8.key
```

### redirect_uri uses http:// instead of https://
The service is not receiving the `X-Forwarded-Proto` header. Verify that the NPM Advanced config contains `proxy_set_header X-Forwarded-Proto https;` and that trusted_proxies are correctly set in the service config.

### Cloudflare Access blocks OIDC endpoints
Verify that Bypass applications are correctly configured for `/.well-known`, `/api/oidc` and `/jwks.json` on `auth.example.com`.

### Immich won't start — missing .immich file
```bash
# Check data location
ls /media/Media/Immich/
# Recreate manually if needed
touch /media/Media/Immich/library/.immich
touch /media/Media/Immich/encoded-video/.immich
```

### CORS blocked on dashboard
The Uptime Kuma API is queried from a different domain. Verify the CORS header in NPM and that the Cloudflare Access bypass is correctly set on `/api/status-page/heartbeat`.

---

## To do

- [ ] Enable TOTP 2FA on Authelia for important accounts
- [ ] Set up automatic backups (Authelia config, LLDAP database, PostgreSQL)
- [ ] Configure Uptime Kuma alerts (email or push notification)
- [ ] Evaluate mobile access for each service (API token vs OIDC)
