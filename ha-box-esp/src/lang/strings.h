#pragma once

struct LangHome
{
  const char* buttonAllOff;
  const char* buttonSettings;
  const char* alertClear;     // "all clear" state
  const char* alertWarning;   // word prefixed by count: "2 Alerte"
  const char* alertCritical;  // word prefixed by count: "1 Critique"
};

struct LangSettings
{
  const char* title;
  const char* labelBrightness;
  const char* labelSound;
};

struct LangLoading
{
  const char* title;
  const char* subtitleLine1;
  const char* subtitleLine2;
  const char* reasonDefault;
  const char* reasonShutdown;
  const char* reasonFirmwareUpdate;
  const char* reasonSuffixInProgress;
  const char* warningLine1;
  const char* warningLine2;
  const char* warningLine3;
};

struct LangShutdown
{
  const char* title;
  const char* subtitleLine1;
  const char* subtitleLine2;
  const char* hintLine1;
  const char* hintLine2;
};

struct LangAll
{
  LangHome home;
  LangSettings settings;
  LangLoading loading;
  LangShutdown shutdown;
};
