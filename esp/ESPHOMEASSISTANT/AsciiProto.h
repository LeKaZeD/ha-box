#pragma once
#include <Arduino.h>

class AsciiProto {
public:
  static constexpr size_t MAX_LINE      = 256;
  static constexpr size_t MAX_VERB      = 24;
  static constexpr size_t MAX_KV        = 12;
  static constexpr size_t MAX_KEY       = 24;
  static constexpr size_t MAX_VAL       = 96;
  static constexpr size_t DEDUPE_WINDOW = 8;

  struct KV {
    char key[MAX_KEY];
    char val[MAX_VAL];
  };

  struct Message {
    uint32_t id = 0;
    char verb[MAX_VERB];   // Ex: "READY", "STATUS", "SHUTDOWN_REQUEST"
    KV kv[MAX_KV];
    uint8_t kvCount = 0;
  };

  struct Ack {
    uint32_t id = 0;
    bool ok = false;
    char err[MAX_KEY];     // Ex: "unauthorized", "bad_args", "unknown_cmd"
  };

  using AuthorizeFn = bool (*)(const Message& msg, void* user, const char** errCode);
  using MessageFn   = void (*)(const Message& msg, void* user);
  using AckFn       = void (*)(const Ack& ack, void* user);
  using RawLineFn   = void (*)(const char* line, void* user);

  explicit AsciiProto(HardwareSerial& serial);

  void begin();
  void poll();

  // Heartbeat implicite : mis à jour à chaque ligne valide reçue (ACK ou message)
  uint32_t lastSeenMs() const;

  // Callbacks
  void setAuthorizeCallback(AuthorizeFn fn, void* user);
  void setMessageCallback(MessageFn fn, void* user);
  void setAckCallback(AckFn fn, void* user);
  void setRawLineCallback(RawLineFn fn, void* user);

  // Envoi
  uint32_t nextId();
  uint32_t sendCommand(const char* verb, const KV* kv = nullptr, uint8_t kvCount = 0);

  // Version simple synchrone (bloquante) pour attendre l'ACK d'une commande envoyée
  bool sendWithAck(const char* verb,
                   const KV* kv,
                   uint8_t kvCount,
                   uint32_t ackTimeoutMs,
                   uint8_t retryCount,
                   Ack* outAck = nullptr);

  // ACK manuel (si tu veux l'utiliser toi-même)
  void sendAckOk(uint32_t id);
  void sendAckErr(uint32_t id, const char* errCode);

private:
  void processLine(char* line);

  bool parseAck(char* line, Ack& out);
  bool parseMessage(char* line, Message& out);

  void autoAckForMessage(const Message& msg);

  bool isDuplicateIncoming(uint32_t id);
  void rememberIncoming(uint32_t id);

  void writeLine(const char* s);

private:
  HardwareSerial& m_ser;

  // RX line buffer
  char m_line[MAX_LINE];
  size_t m_len = 0;

  // last activity
  uint32_t m_lastSeen = 0;

  // id generator
  uint32_t m_nextId = 1;

  // dedupe ring
  uint32_t m_recentIds[DEDUPE_WINDOW];
  uint8_t m_recentPos = 0;

  // callbacks
  AuthorizeFn m_authFn = nullptr;
  void* m_authUser = nullptr;

  MessageFn m_msgFn = nullptr;
  void* m_msgUser = nullptr;

  AckFn m_ackFn = nullptr;
  void* m_ackUser = nullptr;

  RawLineFn m_rawLineFn = nullptr;
  void* m_rawLineUser = nullptr;

  // waiting ACK (for sendWithAck)
  volatile bool m_waitingAck = false;
  uint32_t m_waitAckId = 0;
  Ack m_lastAck;
};