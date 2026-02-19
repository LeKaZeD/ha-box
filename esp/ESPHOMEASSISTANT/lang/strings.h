#pragma once

struct LangHome
{
  const char* buttonAdd;
  const char* buttonAllOff;
  const char* buttonSettings;
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
  const char* reasonSuffixInProgress;
  const char* warningLine1;
  const char* warningLine2;
  const char* warningLine3;
};

struct LangAll
{
  LangHome home;
  LangSettings settings;
  LangLoading loading;
};
