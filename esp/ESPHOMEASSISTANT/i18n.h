#pragma once
#include <stdint.h>
#include "lang/strings.h"

namespace i18n
{
  void init(uint8_t langId);
  const LangHome& home();
  const LangSettings& settings();
  const LangLoading& loading();
}
