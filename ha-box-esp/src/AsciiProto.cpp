#include "AsciiProto.h"
#include <string.h>

AsciiProto::AsciiProto(HardwareSerial& serial) : m_ser(serial) {
  memset(m_recentIds, 0, sizeof(m_recentIds));
  memset(m_line, 0, sizeof(m_line));
}

void AsciiProto::begin() {
  m_len = 0;
  m_lastSeen = millis();
}

uint32_t AsciiProto::lastSeenMs() const {
  return m_lastSeen;
}

void AsciiProto::setAuthorizeCallback(AuthorizeFn fn, void* user) {
  m_authFn = fn;
  m_authUser = user;
}

void AsciiProto::setMessageCallback(MessageFn fn, void* user) {
  m_msgFn = fn;
  m_msgUser = user;
}

void AsciiProto::setAckCallback(AckFn fn, void* user) {
  m_ackFn = fn;
  m_ackUser = user;
}

void AsciiProto::setRawLineCallback(RawLineFn fn, void* user) {
  m_rawLineFn = fn;
  m_rawLineUser = user;
}

uint32_t AsciiProto::nextId() {
  return m_nextId++;
}

void AsciiProto::writeLine(const char* s) {
  m_ser.print(s);
  m_ser.print('\n');
}

void AsciiProto::poll() {
  while (m_ser.available()) {
    char c = (char)m_ser.read();
    if (c == '\r') continue;

    if (c == '\n') {
      if (m_len == 0) continue; // ignore empty line
      m_line[m_len] = '\0';
      processLine(m_line);
      m_len = 0;
      continue;
    }

    if (m_len < (MAX_LINE - 1)) {
      m_line[m_len++] = c;
    } else {
      Serial2.println("[proto] RX overflow: line >256 chars, discarded");
      m_len = 0;
    }
  }
}

void AsciiProto::processLine(char* line) {
  if (m_rawLineFn) m_rawLineFn(line, m_rawLineUser);

  // strtok_r modifies the buffer; parseAck must not truncate line before parseMessage
  char lineCopy[MAX_LINE];
  strncpy(lineCopy, line, MAX_LINE - 1);
  lineCopy[MAX_LINE - 1] = '\0';

  // lastSeen is updated only after successful parsing to ignore noise
  Ack ack;
  if (parseAck(lineCopy, ack)) {
    m_lastSeen = millis();

    // sendWithAck: capture si c'est l'ACK attendu
    if (m_waitingAck && ack.id == m_waitAckId) {
      m_lastAck = ack;
      m_waitingAck = false;
    }

    if (m_ackFn) m_ackFn(ack, m_ackUser);
    return;
  }

  Message msg;
  if (parseMessage(line, msg)) {
    m_lastSeen = millis();

    // Auto-ACK before processing; dedup handled below
    autoAckForMessage(msg);

    // Dedup: drop duplicate messages silently
    if (isDuplicateIncoming(msg.id)) return;
    rememberIncoming(msg.id);

    if (m_msgFn) m_msgFn(msg, m_msgUser);
    return;
  }

  // Invalid line: ignored
}

bool AsciiProto::parseAck(char* line, Ack& out) {
  // Format:
  // ACK <id> OK
  // ACK <id> ERR <code>
  char* save = nullptr;
  char* tok = strtok_r(line, " ", &save);
  if (!tok || strcmp(tok, "ACK") != 0) return false;

  char* idTok = strtok_r(nullptr, " ", &save);
  char* statusTok = strtok_r(nullptr, " ", &save);
  if (!idTok || !statusTok) return false;

  out.id = (uint32_t)strtoul(idTok, nullptr, 10);

  if (strcmp(statusTok, "OK") == 0) {
    out.ok = true;
    out.err[0] = '\0';
    return true;
  }

  if (strcmp(statusTok, "ERR") == 0) {
    out.ok = false;
    char* codeTok = strtok_r(nullptr, " ", &save);
    if (!codeTok) codeTok = (char*)"err";
    strncpy(out.err, codeTok, MAX_KEY - 1);
    out.err[MAX_KEY - 1] = '\0';
    return true;
  }

  return false;
}

static bool copyToken(char* dst, size_t dstSize, const char* src) {
  if (!src || !src[0]) return false;
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
  return true;
}

bool AsciiProto::parseMessage(char* line, Message& out) {
  // Format:
  // <id> <VERB> [key=value]...
  char* save = nullptr;
  char* idTok = strtok_r(line, " ", &save);
  char* verbTok = strtok_r(nullptr, " ", &save);
  if (!idTok || !verbTok) return false;

  out.id = (uint32_t)strtoul(idTok, nullptr, 10);
  if (out.id == 0) return false;

  if (!copyToken(out.verb, MAX_VERB, verbTok)) return false;

  out.kvCount = 0;

  for (;;) {
    char* kvTok = strtok_r(nullptr, " ", &save);
    if (!kvTok) break;
    if (out.kvCount >= MAX_KV) {
      Serial2.println("[proto] KV overflow: >12 pairs, extra ignored");
      break;
    }

    char* eq = strchr(kvTok, '=');
    if (!eq) continue;

    *eq = '\0';
    const char* k = kvTok;
    const char* v = eq + 1;

    if (!k[0]) continue;

    strncpy(out.kv[out.kvCount].key, k, MAX_KEY - 1);
    out.kv[out.kvCount].key[MAX_KEY - 1] = '\0';

    strncpy(out.kv[out.kvCount].val, v, MAX_VAL - 1);
    out.kv[out.kvCount].val[MAX_VAL - 1] = '\0';

    out.kvCount++;
  }

  return true;
}

void AsciiProto::autoAckForMessage(const Message& msg) {
  // Immediate ACK: run allowlist check if callback is set
  if (m_authFn) {
    const char* err = nullptr;
    bool ok = m_authFn(msg, m_authUser, &err);
    if (ok) sendAckOk(msg.id);
    else sendAckErr(msg.id, err ? err : "unauthorized");
  } else {
    sendAckOk(msg.id);
  }
}

void AsciiProto::sendAckOk(uint32_t id) {
  char buf[64];
  snprintf(buf, sizeof(buf), "ACK %lu OK", (unsigned long)id);
  writeLine(buf);
}

void AsciiProto::sendAckErr(uint32_t id, const char* errCode) {
  char buf[96];
  snprintf(buf, sizeof(buf), "ACK %lu ERR %s", (unsigned long)id, errCode ? errCode : "err");
  writeLine(buf);
}

bool AsciiProto::isDuplicateIncoming(uint32_t id) {
  for (uint8_t i = 0; i < DEDUPE_WINDOW; i++) {
    if (m_recentIds[i] == id) return true;
  }
  return false;
}

void AsciiProto::rememberIncoming(uint32_t id) {
  m_recentIds[m_recentPos] = id;
  m_recentPos = (uint8_t)((m_recentPos + 1) % DEDUPE_WINDOW);
}

uint32_t AsciiProto::sendCommand(const char* verb, const KV* kv, uint8_t kvCount) {
  uint32_t id = nextId();

  char line[MAX_LINE];
  size_t used = 0;

  used += (size_t)snprintf(line + used, sizeof(line) - used, "%lu %s",
                           (unsigned long)id, verb ? verb : "CMD");

  for (uint8_t i = 0; i < kvCount && kv; i++) {
    if (!kv[i].key[0]) continue;
    used += (size_t)snprintf(line + used, sizeof(line) - used, " %s=%s", kv[i].key, kv[i].val);
    if (used >= sizeof(line)) break;
  }

  writeLine(line);
  return id;
}

bool AsciiProto::sendWithAck(const char* verb,
                            const KV* kv,
                            uint8_t kvCount,
                            uint32_t ackTimeoutMs,
                            uint8_t retryCount,
                            Ack* outAck) {
  for (uint8_t attempt = 0; attempt <= retryCount; attempt++) {
    uint32_t id = sendCommand(verb, kv, kvCount);

    m_waitAckId = id;
    m_waitingAck = true;

    uint32_t start = millis();
    while (m_waitingAck && (millis() - start) < ackTimeoutMs) {
      poll();
      delay(1);
    }

    if (!m_waitingAck) {
      if (outAck) *outAck = m_lastAck;
      return m_lastAck.ok;
    }
  }

  return false;
}