var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

function storedUnit() {
  var u = localStorage.getItem('UNIT');
  return (u === null) ? 0 : parseInt(u, 10);
}

// Tell the watch the display-unit preference. The watch stores raw °C and
// converts, so this alone updates the display even with no fresh weather fetch.
function sendUnit(unitInt) {
  Pebble.sendAppMessage({ 'UNIT': unitInt },
    function() { console.log('unit sent: ' + unitInt); },
    function(e) { console.log('unit send failed: ' + JSON.stringify(e)); });
}

// Fox display mode: 0 = idle, 1 = walking. The watch persists it.
function sendFoxMode(mode) {
  Pebble.sendAppMessage({ 'FOX_MODE': mode },
    function() { console.log('fox mode sent: ' + mode); },
    function(e) { console.log('fox mode send failed: ' + JSON.stringify(e)); });
}

// Pull an integer from the raw Clay settings (a select arrives as { value: "x" }).
function settingInt(settings, key, dflt) {
  var raw = settings && settings[key];
  var val = (raw && typeof raw === 'object') ? raw.value : raw;
  var n = parseInt(val, 10);
  return isNaN(n) ? dflt : n;
}

function sendWeather(tempC, code, unitInt) {
  Pebble.sendAppMessage(
    { 'TEMPERATURE': tempC, 'WEATHER_CODE': code, 'UNIT': unitInt },
    function() { console.log('weather sent: ' + tempC + 'C code ' + code); },
    function(e) { console.log('weather send failed: ' + JSON.stringify(e)); }
  );
}

function fetchWeather() {
  var unitInt = storedUnit();
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      // Always fetch Celsius; the watch converts to °F when needed, so a unit
      // change takes effect without a fresh fetch.
      var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + pos.coords.latitude +
                '&longitude=' + pos.coords.longitude +
                '&current=temperature_2m,weather_code&temperature_unit=celsius';
      var xhr = new XMLHttpRequest();
      xhr.timeout = 10000;
      xhr.onload = function() {
        if (xhr.status < 200 || xhr.status >= 300) {
          console.log('weather HTTP error: ' + xhr.status);
          return;
        }
        try {
          var json = JSON.parse(xhr.responseText);
          var cur = json && json.current;
          var temp = cur ? Math.round(cur.temperature_2m) : NaN;
          var code = cur ? cur.weather_code : null;
          if (isNaN(temp) || code === undefined || code === null) {
            console.log('weather: missing fields in response');
            return;
          }
          sendWeather(temp, code, unitInt);
        } catch (err) { console.log('weather parse error: ' + err); }
      };
      xhr.onerror   = function() { console.log('weather network error'); };
      xhr.ontimeout = function() { console.log('weather request timed out'); };
      xhr.open('GET', url);
      xhr.send();
    },
    function(err) { console.log('geo error: ' + err.message); },
    { timeout: 15000, maximumAge: 60000 }
  );
}

Pebble.addEventListener('ready', function() { fetchWeather(); });

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload && typeof e.payload.REQUEST_WEATHER !== 'undefined') { fetchWeather(); }
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) { return; }
  // clay.getSettings(response) returns a dict keyed by NUMERIC AppMessage keys, so
  // dict.UNIT by name is always undefined. Read the raw, messageKey-named settings
  // instead; a select value arrives as { value: "0" | "1" }.
  var settings = clay.getSettings(e.response, false);

  var u = settingInt(settings, 'UNIT', 0);
  localStorage.setItem('UNIT', u);
  sendUnit(u);     // watch re-converts its stored °C immediately - works even offline

  var fox = settingInt(settings, 'FOX_MODE', 0);
  sendFoxMode(fox);

  console.log('config saved: UNIT=' + u + ' FOX_MODE=' + fox);
  fetchWeather();  // also refresh the actual weather data
});
