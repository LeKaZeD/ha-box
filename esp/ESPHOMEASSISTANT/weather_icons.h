#pragma once
#include "ui/assets/icons_min.h"

/**
 * Weather icon mapping for Home Assistant conditions.
 * Maps numeric weather codes to 45x45 weather icons.
 * 
 * Weather codes:
 *  0 = unknown/N/A
 *  1 = clear-night
 *  2 = cloudy
 *  3 = fog
 *  4 = hail
 *  5 = lightning
 *  6 = lightning-rainy
 *  7 = partlycloudy
 *  8 = pouring
 *  9 = rainy
 * 10 = snowy
 * 11 = snowy-rainy
 * 12 = sunny
 * 13 = windy
 * 14 = windy-variant
 * 15 = exceptional/alert
 */

const uint8_t* getWeatherIcon(uint8_t code, int& w, int& h)
{
  w = 45;
  h = 45;
  
  switch (code)
  {
    case 1:  // clear-night
      return WEATHER_MOON_45x45;
    
    case 2:  // cloudy
      return WEATHER_CLOUD_45x45;
    
    case 3:  // fog
      return WEATHER_FOG_45x45;
    
    case 4:  // hail
      return WEATHER_HAIL_45x45;
    
    case 5:  // lightning
      return WEATHER_THUNDER_45x45;
    
    case 6:  // lightning-rainy
      return WEATHER_THUNDER_RAIN_45x45;
    
    case 7:  // partlycloudy
      return WEATHER_DAY_CLOUDY_45x45;
    
    case 8:  // pouring
      return WEATHER_POURING_45x45;
    
    case 9:  // rainy
      return WEATHER_RAIN_45x45;
    
    case 10: // snowy
      return WEATHER_SNOWY_45x45;
    
    case 11: // snowy-rainy
      return WEATHER_SNOWY_RAINY_45x45;
    
    case 12: // sunny
      return WEATHER_SUNNY_45x45;
    
    case 13: // windy
      return WEATHER_WIND_45x45;
    
    case 14: // windy-variant
      return WEATHER_WIND_CLOUDY_45x45;
    
    case 15: // exceptional/alert
      return WEATHER_ALERT_45x45;
    
    default: // unknown (0)
      return WEATHER_NA_45x45;
  }
}
