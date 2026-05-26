#include "weather.h"

WeatherScene weather_scene_for_code(int code) {
  if (code <= 1)                   return SCENE_CLEAR;   // 0,1 clear
  if (code <= 3)                   return SCENE_CLOUDY;  // 2,3 cloudy/overcast
  if (code == 45 || code == 48)    return SCENE_CLOUDY;  // fog
  if (code >= 71 && code <= 77)    return SCENE_SNOW;    // snow, snow grains
  if (code == 85 || code == 86)    return SCENE_SNOW;    // snow showers
  return SCENE_RAIN;                                     // 51-67 drizzle/rain, 80-82 showers, 95-99 thunder
}

bool weather_has_lightning(int code) {
  return code == 95 || code == 96 || code == 99;  // WMO thunderstorm codes
}

const char *weather_label_for_code(int code) {
  if (code <= 1)                            return "Clear";
  if (code <= 3)                            return "Cloudy";
  if (code == 45 || code == 48)             return "Fog";
  if ((code >= 71 && code <= 77) ||
      code == 85 || code == 86)             return "Snow";
  if (code >= 95)                           return "Storm";
  return "Rain";
}
