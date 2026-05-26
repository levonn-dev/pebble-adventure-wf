#include "../src/c/weather.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
  // scene mapping
  assert(weather_scene_for_code(0)  == SCENE_CLEAR);
  assert(weather_scene_for_code(1)  == SCENE_CLEAR);
  assert(weather_scene_for_code(2)  == SCENE_CLOUDY);
  assert(weather_scene_for_code(3)  == SCENE_CLOUDY);
  assert(weather_scene_for_code(45) == SCENE_CLOUDY);
  assert(weather_scene_for_code(48) == SCENE_CLOUDY);
  assert(weather_scene_for_code(51) == SCENE_RAIN);
  assert(weather_scene_for_code(65) == SCENE_RAIN);
  assert(weather_scene_for_code(80) == SCENE_RAIN);
  assert(weather_scene_for_code(82) == SCENE_RAIN);
  assert(weather_scene_for_code(95) == SCENE_RAIN);
  assert(weather_scene_for_code(99) == SCENE_RAIN);
  assert(weather_scene_for_code(71) == SCENE_SNOW);
  assert(weather_scene_for_code(77) == SCENE_SNOW);
  assert(weather_scene_for_code(85) == SCENE_SNOW);
  assert(weather_scene_for_code(86) == SCENE_SNOW);

  // lightning
  assert(weather_has_lightning(95) == true);
  assert(weather_has_lightning(96) == true);
  assert(weather_has_lightning(99) == true);
  assert(weather_has_lightning(65) == false);
  assert(weather_has_lightning(0)  == false);

  // labels
  assert(strcmp(weather_label_for_code(0),  "Clear")  == 0);
  assert(strcmp(weather_label_for_code(2),  "Cloudy") == 0);
  assert(strcmp(weather_label_for_code(45), "Fog")    == 0);
  assert(strcmp(weather_label_for_code(61), "Rain")   == 0);
  assert(strcmp(weather_label_for_code(95), "Storm")  == 0);
  assert(strcmp(weather_label_for_code(71), "Snow")   == 0);

  printf("all weather tests passed\n");
  return 0;
}
