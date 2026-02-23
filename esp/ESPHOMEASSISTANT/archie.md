/src
  main.ino

  /model
    model.h
    model.cpp
    dirty_keys.h
    state_onboarding.h
    state_loading.h
    state_home.h

  /ui
    ui_manager.h
    ui_manager.cpp
    page.h
    page_onboarding.h/.cpp
    page_loading.h/.cpp
    page_home.h/.cpp

    /widgets
      widget.h
      widget_status_icons.h/.cpp
      widget_weather.h/.cpp
      widget_temp.h/.cpp
      widget_clock.h/.cpp
      widget_progress.h/.cpp

  /drivers
    display_driver.h/.cpp      // helpers drawIcon, draw degree, etc.
    uart_rx.h/.cpp             // parse UART => model.setXxx()

  /assets
    icons_min.h
    /fonts
      MyFont18pt7b.h