#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <RTClib.h>
#include <Preferences.h>
#include "webpage.h"

#define PIN_RELAY      26      
#define PIN_LED        2       

#define RELAY_ACTIVE_LOW   true

const char* AP_SSID = "Automatic Watering";
const char* AP_PASS = "12345678";        

AsyncWebServer server(80);
RTC_DS3231 rtc;
Preferences prefs;
bool rtcOk = false;

struct Settings {
  uint8_t  startHour       = 10;
  uint8_t  startMinute     = 0;
  uint8_t  endHour         = 14;
  uint8_t  endMinute       = 0;
  uint16_t sprayDurationSec= 900;   
  uint16_t restDurationSec = 900;   
} cfg;

enum ManualMode { MODE_AUTO, MODE_MAN_ON, MODE_MAN_OFF };
ManualMode manualMode = MODE_AUTO;

enum CyclePhase { PHASE_SPRAY, PHASE_REST };
CyclePhase cyclePhase   = PHASE_SPRAY;
unsigned long phaseStart = 0;
bool inWindowPrev = false;
bool pumpOn = false;
String stateLabel = "DI LUAR JADWAL";

String lastWaterTime = "";   

void loadSettings(){
  prefs.begin("siram", true);
  cfg.startHour        = prefs.getUChar("sH", cfg.startHour);
  cfg.startMinute      = prefs.getUChar("sM", cfg.startMinute);
  cfg.endHour          = prefs.getUChar("eH", cfg.endHour);
  cfg.endMinute        = prefs.getUChar("eM", cfg.endMinute);
  cfg.sprayDurationSec = prefs.getUShort("spr", cfg.sprayDurationSec);
  cfg.restDurationSec  = prefs.getUShort("rst", cfg.restDurationSec);
  prefs.end();
}
void saveSettings(){
  prefs.begin("siram", false);
  prefs.putUChar("sH", cfg.startHour);
  prefs.putUChar("sM", cfg.startMinute);
  prefs.putUChar("eH", cfg.endHour);
  prefs.putUChar("eM", cfg.endMinute);
  prefs.putUShort("spr", cfg.sprayDurationSec);
  prefs.putUShort("rst", cfg.restDurationSec);
  prefs.end();
}

void saveLastWater(){
  prefs.begin("siram", false);
  prefs.putString("last", lastWaterTime);
  prefs.end();
}
void loadLastWater(){
  prefs.begin("siram", true);
  lastWaterTime = prefs.getString("last", "");
  prefs.end();
}

void recordWaterEnd(){
  if (!rtcOk) return;
  DateTime n = rtc.now();
  char b[8];
  snprintf(b, sizeof(b), "%02d.%02d", n.hour(), n.minute());
  lastWaterTime = String(b);     
  saveLastWater();
}

void setPump(bool on){
  bool prev = pumpOn;
  pumpOn = on;
  digitalWrite(PIN_RELAY, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  if (!on && prev) recordWaterEnd();      
}

int minutesOfDay(int h,int m){ return h*60+m; }

bool isWithinWindow(const DateTime& now){
  int s = minutesOfDay(cfg.startHour, cfg.startMinute);
  int e = minutesOfDay(cfg.endHour,   cfg.endMinute);
  int n = minutesOfDay(now.hour(),    now.minute());
  if (s == e) return false;              
  if (s <  e) return (n >= s && n < e);  
  return (n >= s || n < e);              
}

long countdownSec = 0;
String countdownLabel = "";

void updateLogic(){
  DateTime now = rtcOk ? rtc.now() : DateTime((uint32_t)(millis()/1000));
  bool inWindow = rtcOk ? isWithinWindow(now) : false;
  bool desired = false;
  countdownLabel = "";
  countdownSec   = 0;

  if (manualMode == MODE_MAN_ON){
    desired = true;  stateLabel = "MANUAL ON";
  }
  else if (manualMode == MODE_MAN_OFF){
    desired = false; stateLabel = "MANUAL OFF";
  }
  else { 
    if (!inWindow){
      desired = false; stateLabel = "DI LUAR JADWAL";
    }
    else {
      
      if (!inWindowPrev){ cyclePhase = PHASE_SPRAY; phaseStart = millis(); }
      unsigned long elapsed = millis() - phaseStart;
      unsigned long sprayMs = (unsigned long)cfg.sprayDurationSec * 1000UL;
      unsigned long restMs  = (unsigned long)cfg.restDurationSec  * 1000UL;

      if (cyclePhase == PHASE_SPRAY){
        if (sprayMs == 0){ cyclePhase = PHASE_REST; phaseStart = millis(); }
        else if (elapsed >= sprayMs){ cyclePhase = PHASE_REST; phaseStart = millis(); }
      } else {
        if (restMs == 0){ cyclePhase = PHASE_SPRAY; phaseStart = millis(); }
        else if (elapsed >= restMs){ cyclePhase = PHASE_SPRAY; phaseStart = millis(); }
      }

      if (cyclePhase == PHASE_SPRAY){
        desired = true;  stateLabel = "MENYIRAM";
        countdownLabel = "Sisa siram";
        countdownSec = (long)((sprayMs - (millis()-phaseStart)) / 1000UL);
      } else {
        desired = false; stateLabel = "ISTIRAHAT";
        countdownLabel = "Sisa istirahat";
        countdownSec = (long)((restMs - (millis()-phaseStart)) / 1000UL);
      }
    }
  }

  inWindowPrev = inWindow;
  if (desired != pumpOn) setPump(desired);
}

String modeStr(){ return manualMode==MODE_MAN_ON?"ON":manualMode==MODE_MAN_OFF?"OFF":"AUTO"; }

String buildStatusJson(){
  DateTime now = rtcOk ? rtc.now() : DateTime((uint32_t)0);
  char tbuf[12], dbuf[12], dtext[40];
  const char* hari[] = {"Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"};
  const char* bln[]  = {"Jan","Feb","Mar","Apr","Mei","Jun","Jul","Agu","Sep","Okt","Nov","Des"};
  snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  snprintf(dbuf, sizeof(dbuf), "%04d-%02d-%02d", now.year(), now.month(), now.day());
  snprintf(dtext,sizeof(dtext),"%s, %d %s %d",
           hari[now.dayOfTheWeek()], now.day(), bln[now.month()-1], now.year());

  String j = "{";
  j += "\"time\":\"" + String(rtcOk?tbuf:"--:--:--") + "\",";
  j += "\"date\":\"" + String(dbuf) + "\",";
  j += "\"dateText\":\"" + String(dtext) + "\",";
  j += "\"rtcOk\":" + String(rtcOk?"true":"false") + ",";
  j += "\"state\":\"" + stateLabel + "\",";
  j += "\"pump\":" + String(pumpOn?"true":"false") + ",";
  j += "\"mode\":\"" + modeStr() + "\",";
  j += "\"countdownLabel\":\"" + countdownLabel + "\",";
  j += "\"countdownSec\":" + String(countdownSec) + ",";
  j += "\"lastWater\":\"" + lastWaterTime + "\",";
  j += "\"settings\":{";
  j += "\"startHour\":"        + String(cfg.startHour) + ",";
  j += "\"startMinute\":"      + String(cfg.startMinute) + ",";
  j += "\"endHour\":"          + String(cfg.endHour) + ",";
  j += "\"endMinute\":"        + String(cfg.endMinute) + ",";
  j += "\"sprayDurationSec\":" + String(cfg.sprayDurationSec) + ",";
  j += "\"restDurationSec\":"  + String(cfg.restDurationSec);
  j += "}}";
  return j;
}

String argv(AsyncWebServerRequest* r, const char* name, const String& def=""){
  if (r->hasParam(name, true))  return r->getParam(name, true)->value();   
  if (r->hasParam(name, false)) return r->getParam(name, false)->value();  
  return def;
}
int argi(AsyncWebServerRequest* r, const char* name, int def){
  String v = argv(r, name);
  return v.length() ? v.toInt() : def;
}
template<typename T> T clampv(T x, T lo, T hi){ return x<lo?lo:(x>hi?hi:x); }

void setupServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/html", index_html);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildStatusJson());
  });

  server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *req){
    String m = argv(req, "mode");
    if (m=="on")   manualMode = MODE_MAN_ON;
    else if (m=="off")  manualMode = MODE_MAN_OFF;
    else if (m=="auto") manualMode = MODE_AUTO;
    updateLogic();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *req){
    cfg.startHour        = clampv(argi(req,"startHour",   cfg.startHour),   0, 23);
    cfg.startMinute      = clampv(argi(req,"startMinute", cfg.startMinute), 0, 59);
    cfg.endHour          = clampv(argi(req,"endHour",     cfg.endHour),     0, 23);
    cfg.endMinute        = clampv(argi(req,"endMinute",   cfg.endMinute),   0, 59);
    cfg.sprayDurationSec = clampv(argi(req,"sprayDurationSec", cfg.sprayDurationSec), 0, 64800);
    cfg.restDurationSec  = clampv(argi(req,"restDurationSec",  cfg.restDurationSec),  0, 64800);
    saveSettings();
    phaseStart = millis();          
    inWindowPrev = false;
    updateLogic();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest *req){
    int Y=argi(req,"Y",2025), Mo=argi(req,"M",1), D=argi(req,"D",1);
    int h=argi(req,"h",0), mi=argi(req,"m",0), s=argi(req,"s",0);
    if (rtcOk) rtc.adjust(DateTime(Y,Mo,D,h,mi,s));
    req->send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound([](AsyncWebServerRequest *req){ req->send(404, "text/plain", "404"); });
  server.begin();
}

void setup(){
  Serial.begin(115200);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED,   OUTPUT);
  setPump(false);                       

  loadSettings();
  loadLastWater();

  if (rtc.begin()){
    rtcOk = true;
    if (rtc.lostPower()){
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  } else {
    Serial.println("RTC DS3231 tidak terdeteksi! Cek wiring I2C.");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP aktif. Buka http://");
  Serial.println(WiFi.softAPIP());      

  setupServer();
}

void loop(){
  static unsigned long t = 0;
  if (millis() - t >= 250){            
    t = millis();
    updateLogic();
  }
}