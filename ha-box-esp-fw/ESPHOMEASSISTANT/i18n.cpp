#include "i18n.h"
#include "lang/fr.h"
#include "lang/en.h"

static const LangAll* s_current = &strings_fr;

void i18n::init(uint8_t langId)
{
  s_current = (langId == 1) ? &strings_en : &strings_fr;
}

const LangHome& i18n::home()
{
  return s_current->home;
}

const LangSettings& i18n::settings()
{
  return s_current->settings;
}

const LangLoading& i18n::loading()
{
  return s_current->loading;
}
