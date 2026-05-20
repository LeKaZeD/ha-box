#pragma once
#include <stdint.h>
#include "../model/model.h"
#include "page.h"
#include "page_onboarding.h"
#include "page_loading.h"
#include "page_home.h"
#include "page_settings.h"
#include "page_shutdown.h"

enum class PageId : uint8_t
{
  None = 0,
  Onboarding,
  Loading,
  Home,
  Settings,
  Shutdown
};

template<typename DisplayT>
class UIManager
{
public:
  UIManager(DisplayT& display, Model& model)
    : display_(display), model_(model), currentPageId_(PageId::None), currentPage_(nullptr)
  {}

  // ---- Lifecycle ----
  void begin(PageId initialPage = PageId::Loading)
  {
    navigateTo(initialPage);
  }

  void update()
  {
    if (!currentPage_) return;

    // 1) Tick de la page courante (animations, logique locale, etc.)
    currentPage_->tick(model_);

    // 2) Si le modèle est dirty, appliquer aux widgets
    if (model_.hasDirty())
    {
      uint32_t dirty = model_.takeDirty();
      currentPage_->applyDirty(dirty);
    }

    // 3) Render partial (budget = 2 refreshes max per update)
    currentPage_->renderPartials(display_, model_, 2);
  }

  // ---- Navigation ----
  void navigateTo(PageId pageId)
  {
    if (pageId == currentPageId_) return; // already on this page

    // Exit current page
    if (currentPage_)
    {
      currentPage_->onExit(model_);
      currentPage_ = nullptr;
    }

    currentPageId_ = pageId;

    // Enter new page
    switch (pageId)
    {
      case PageId::Onboarding:
        currentPage_ = &pageOnboarding_;
        break;
      case PageId::Loading:
        currentPage_ = &pageLoading_;
        break;
      case PageId::Home:
        currentPage_ = &pageHome_;
        break;
      case PageId::Settings:
        currentPage_ = &pageSettings_;
        break;
      case PageId::Shutdown:
        currentPage_ = &pageShutdown_;
        break;
      default:
        currentPageId_ = PageId::None;
        return;
    }

    if (currentPage_)
    {
      currentPage_->onEnter(model_);
      currentPage_->drawFull(display_, model_);
    }
  }

  PageId getCurrentPageId() const { return currentPageId_; }

  void redrawCurrentPage()
  {
    if (!currentPage_) return;
    currentPage_->onExit(model_);
    currentPage_->onEnter(model_);
    currentPage_->drawFull(display_, model_);
  }

  // Call when touch is detected (x, y in display coordinates). Returns true if the page consumed it.
  bool handleTouch(int16_t x, int16_t y)
  {
    if (!currentPage_) return false;
    return currentPage_->onTouch(x, y);
  }

  // ---- Access to pages (for setup, font injection, etc.) ----
  auto& pageOnboarding() { return pageOnboarding_; }
  auto& pageLoading()    { return pageLoading_; }
  auto& pageHome()       { return pageHome_; }
  auto& pageSettings()   { return pageSettings_; }
  auto& pageShutdown()   { return pageShutdown_; }

private:
  DisplayT& display_;
  Model& model_;
  
  PageId currentPageId_;
  Page<DisplayT>* currentPage_;

  // Instances des pages (créées à la compilation)
  PageOnboarding<DisplayT> pageOnboarding_;
  PageLoading<DisplayT>    pageLoading_;
  PageHome<DisplayT>       pageHome_;
  PageSettings<DisplayT>   pageSettings_;
  PageShutdown<DisplayT>   pageShutdown_;
};
