#pragma once
#include "../../i18n.h"
#include "../page_loading.h"

struct LoadingPageConfig
{
  const GFXfont* fontTitle;
  const GFXfont* fontSubtitle;
  const GFXfont* fontReason;
  const GFXfont* fontWarning;
  const uint8_t* timerIconUp;
  const uint8_t* timerIconMid;
  const uint8_t* timerIconComplete;
};

template<typename DisplayT>
void applyLoadingConfig(
  PageLoading<DisplayT>& page,
  const LoadingPageConfig& cfg)
{
  page.setFonts(cfg.fontTitle, cfg.fontSubtitle, cfg.fontReason, cfg.fontWarning);
  page.setTimerIcons(cfg.timerIconUp, cfg.timerIconMid, cfg.timerIconComplete);
}
