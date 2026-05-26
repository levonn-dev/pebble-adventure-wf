#pragma once
#include <stdbool.h>

// Weather scene = which background + effect to show. Backed by the four
// adventure biomes: CLEAR=plains, CLOUDY=forest, RAIN=storm, SNOW=mountain.
typedef enum {
  SCENE_CLEAR  = 0,
  SCENE_CLOUDY = 1,
  SCENE_RAIN   = 2,
  SCENE_SNOW   = 3,
} WeatherScene;

// Map an Open-Meteo WMO weather code to a scene.
WeatherScene weather_scene_for_code(int code);

// True only for thunderstorm codes (lightning overlay).
bool weather_has_lightning(int code);

// Short human label for the status row ("Clear","Cloudy","Fog","Rain","Storm","Snow").
const char *weather_label_for_code(int code);
