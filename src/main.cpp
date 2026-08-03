#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <RTClib.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include "webpage.h"

#define PIN_RELAY      26
#define PIN_LED        2

#define RELAY_ACTIVE_LOW   true
#define WDT_TIMEOUT_SEC    30
#define CYCLE_SAVE_MS      5000UL
#define WIFI_CHECK_MS      10000UL
#define LOG_PATH           "/siram_log.csv"
#define LOG_MIN_FREE       8192UL

const char* AP_SSID = "Automatic Spraying";
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
bool cycleRestored = false;

String lastWaterTime = "";
uint32_t bootCount = 0;
bool fsOk = false;

String modeStr();
bool initFS();
size_t logFileSize();
void appendLog(const char* event);
void appendLog(const char* event, const char* status);
void clearLogFile();
void getStorageJson(String& out);
void savePumpFlag(bool on);
bool loadPumpFlag();
void logBootEvents(bool wasPumpOn);
void relaySafeOff();

// Matikan relay secepat mungkin (active-low: HIGH = OFF)
void relaySafeOff(){
  gpio_config_t io = {};
  io.pin_bit_mask = (1ULL << PIN_RELAY);
  io.mode = GPIO_MODE_OUTPUT;
  io.pull_up_en = GPIO_PULLUP_ENABLE;
  io.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io);
  gpio_set_level((gpio_num_t)PIN_RELAY, RELAY_ACTIVE_LOW ? 1 : 0);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
}

const char* resetReasonStr(){
  switch (esp_reset_reason()){
    case ESP_RST_POWERON:   return "Power On";
    case ESP_RST_EXT:       return "External Reset";
    case ESP_RST_SW:        return "Software Reset";
    case ESP_RST_PANIC:     return "Panic";
    case ESP_RST_INT_WDT:   return "Int Watchdog";
    case ESP_RST_TASK_WDT:  return "Task Watchdog";
    case ESP_RST_WDT:       return "Watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep Sleep";
    case ESP_RST_BROWNOUT:  return "Brownout";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "Unknown";
  }
}

void initBootCount(){
  prefs.begin("sysinfo", false);
  bootCount = prefs.getUInt("bootCount", 0) + 1;
  prefs.putUInt("bootCount", bootCount);
  prefs.end();
}

String formatUptime(){
  unsigned long sec = millis() / 1000UL;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
           sec / 3600UL, (sec % 3600UL) / 60UL, sec % 60UL);
  return String(buf);
}

void printSystemStatus(){
  Serial.printf("Reset Terakhir : %s\n", resetReasonStr());
  Serial.printf("Boot Count     : %lu\n", (unsigned long)bootCount);
  Serial.printf("Uptime         : %s\n", formatUptime().c_str());
  Serial.printf("Free Heap      : %u KB\n", ESP.getFreeHeap() / 1024);
}

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

bool initFS(){
  if (!LittleFS.begin(true)){
    Serial.println("LittleFS gagal dimuat.");
    return false;
  }
  if (!LittleFS.exists(LOG_PATH)){
    File f = LittleFS.open(LOG_PATH, "w");
    if (f){
      f.println("tanggal,waktu,event,mode,status");
      f.close();
    }
  }
  return true;
}

size_t logFileSize(){
  if (!fsOk || !LittleFS.exists(LOG_PATH)) return 0;
  File f = LittleFS.open(LOG_PATH, "r");
  if (!f) return 0;
  size_t sz = f.size();
  f.close();
  return sz;
}

void clearLogFile(){
  if (!fsOk) return;
  File f = LittleFS.open(LOG_PATH, "w");
  if (f){
    f.println("tanggal,waktu,event,mode,status");
    f.close();
  }
}

void appendLog(const char* event){
  appendLog(event, stateLabel.c_str());
}

void appendLog(const char* event, const char* status){
  if (!fsOk) return;
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  if (total < used || (total - used) < LOG_MIN_FREE) return;

  char date[12], tim[12];
  if (rtcOk){
    DateTime n = rtc.now();
    if (n.month() < 1 || n.month() > 12){
      snprintf(date, sizeof(date), "----/--/--");
      snprintf(tim, sizeof(tim), "--:--:--");
    } else {
      snprintf(date, sizeof(date), "%04d-%02d-%02d", n.year(), n.month(), n.day());
      snprintf(tim, sizeof(tim), "%02d:%02d:%02d", n.hour(), n.minute(), n.second());
    }
  } else {
    unsigned long sec = millis() / 1000UL;
    snprintf(date, sizeof(date), "uptime");
    snprintf(tim, sizeof(tim), "%02lu:%02lu:%02lu",
             sec / 3600UL, (sec % 3600UL) / 60UL, sec % 60UL);
  }

  File f = LittleFS.open(LOG_PATH, "a");
  if (!f) return;
  f.printf("%s,%s,%s,%s,%s\n", date, tim, event, modeStr().c_str(), status);
  f.close();
}

void savePumpFlag(bool on){
  prefs.begin("siram", false);
  prefs.putBool("pumpOn", on);
  prefs.end();
}

bool loadPumpFlag(){
  prefs.begin("siram", true);
  bool v = prefs.getBool("pumpOn", false);
  prefs.end();
  return v;
}

void logBootEvents(bool wasPumpOn){
  if (!fsOk) return;
  const char* reason = resetReasonStr();
  if (wasPumpOn){
    appendLog("INTERRUPT", reason);
    savePumpFlag(false);
  }
  appendLog("BOOT", reason);
}

void getStorageJson(String& out){
  size_t total = 0, used = 0, freeb = 0, logSz = 0;
  if (fsOk){
    total = LittleFS.totalBytes();
    used  = LittleFS.usedBytes();
    freeb = (total > used) ? (total - used) : 0;
    logSz = logFileSize();
  }
  out = "{";
  out += "\"fsOk\":" + String(fsOk ? "true" : "false") + ",";
  out += "\"totalKB\":" + String(total / 1024) + ",";
  out += "\"usedKB\":" + String(used / 1024) + ",";
  out += "\"freeKB\":" + String(freeb / 1024) + ",";
  out += "\"freeBytes\":" + String((unsigned long)freeb) + ",";
  out += "\"logBytes\":" + String((unsigned long)logSz) + ",";
  out += "\"logKB\":" + String(logSz / 1024);
  out += "}";
}

void saveManualMode(){
  prefs.begin("siram", false);
  prefs.putUChar("mode", (uint8_t)manualMode);
  prefs.end();
}
void loadManualMode(){
  prefs.begin("siram", true);
  manualMode = (ManualMode)prefs.getUChar("mode", MODE_AUTO);
  prefs.end();
}

// Simpan fase + sisa detik agar setelah brownout/restart bisa dilanjutkan
void saveCycleState(uint32_t remainSec){
  prefs.begin("siram", false);
  prefs.putBool("cycOk", true);
  prefs.putUChar("cycPh", (uint8_t)cyclePhase);
  prefs.putUInt("cycRem", remainSec);
  prefs.end();
}
void clearCycleState(){
  prefs.begin("siram", true);
  bool ok = prefs.getBool("cycOk", false);
  prefs.end();
  if (!ok) return;
  prefs.begin("siram", false);
  prefs.putBool("cycOk", false);
  prefs.putUInt("cycRem", 0);
  prefs.end();
}
void loadCycleState(){
  prefs.begin("siram", true);
  bool ok = prefs.getBool("cycOk", false);
  uint8_t ph = prefs.getUChar("cycPh", PHASE_SPRAY);
  uint32_t rem = prefs.getUInt("cycRem", 0);
  prefs.end();

  if (!ok || rem == 0) return;

  cyclePhase = (ph == PHASE_REST) ? PHASE_REST : PHASE_SPRAY;
  uint32_t dur = (cyclePhase == PHASE_SPRAY) ? cfg.sprayDurationSec : cfg.restDurationSec;
  if (rem > dur) rem = dur;

  // phaseStart diset seolah fase sudah berjalan (dur - rem) detik
  unsigned long elapsedMs = (unsigned long)(dur - rem) * 1000UL;
  phaseStart = millis() - elapsedMs;
  inWindowPrev = true;
  cycleRestored = true;
  Serial.printf("Siklus dilanjutkan: fase=%s sisa=%lus\n",
                cyclePhase == PHASE_SPRAY ? "SIRAM" : "ISTIRAHAT",
                (unsigned long)rem);
}

void setPump(bool on){
  bool prev = pumpOn;
  if (on == prev){
    digitalWrite(PIN_RELAY, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
    digitalWrite(PIN_LED, on ? HIGH : LOW);
    return;
  }

  pumpOn = on;
  digitalWrite(PIN_RELAY, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  savePumpFlag(on);

  if (on) appendLog("ON");
  else {
    recordWaterEnd();
    appendLog("OFF");
  }
}

void ensureSoftAP(){
  wifi_mode_t mode = WiFi.getMode();
  bool apUp = (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
  // softAPIP 0.0.0.0 = AP mati / modem hang
  if (!apUp || WiFi.softAPIP() == IPAddress(0, 0, 0, 0)){
    Serial.println("SoftAP down — menyalakan ulang...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    WiFi.setSleep(false);
    Serial.print("AP aktif kembali: http://");
    Serial.println(WiFi.softAPIP());
  }
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
    clearCycleState();
  }
  else if (manualMode == MODE_MAN_OFF){
    desired = false; stateLabel = "MANUAL OFF";
    clearCycleState();
  }
  else {
    if (!inWindow){
      desired = false; stateLabel = "DI LUAR JADWAL";
      clearCycleState();
    }
    else {
      // Masuk jendela baru: mulai siram, kecuali baru restore dari NVS
      if (!inWindowPrev && !cycleRestored){
        cyclePhase = PHASE_SPRAY;
        phaseStart = millis();
      }
      cycleRestored = false;

      unsigned long sprayMs = (unsigned long)cfg.sprayDurationSec * 1000UL;
      unsigned long restMs  = (unsigned long)cfg.restDurationSec  * 1000UL;
      unsigned long elapsed = millis() - phaseStart;
      bool phaseChanged = false;

      if (cyclePhase == PHASE_SPRAY){
        if (sprayMs == 0 || elapsed >= sprayMs){
          cyclePhase = PHASE_REST;
          phaseStart = millis();
          phaseChanged = true;
        }
      } else {
        if (restMs == 0 || elapsed >= restMs){
          cyclePhase = PHASE_SPRAY;
          phaseStart = millis();
          phaseChanged = true;
        }
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
      if (countdownSec < 0) countdownSec = 0;

      // Simpan berkala + saat ganti fase (untuk resume setelah reset)
      static unsigned long lastSave = 0;
      if (phaseChanged || millis() - lastSave >= CYCLE_SAVE_MS){
        lastSave = millis();
        saveCycleState((uint32_t)countdownSec);
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
  j += "},";
  j += "\"system\":{";
  j += "\"resetReason\":\"" + String(resetReasonStr()) + "\",";
  j += "\"bootCount\":" + String(bootCount) + ",";
  j += "\"uptime\":\"" + formatUptime() + "\",";
  j += "\"freeHeapKB\":" + String(ESP.getFreeHeap() / 1024);
  j += "},";
  String stor;
  getStorageJson(stor);
  j += "\"storage\":" + stor;
  j += "}";
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
    saveManualMode();
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
    cycleRestored = false;
    clearCycleState();
    updateLogic();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest *req){
    int Y=argi(req,"Y",2025), Mo=argi(req,"M",1), D=argi(req,"D",1);
    int h=argi(req,"h",0), mi=argi(req,"m",0), s=argi(req,"s",0);
    if (rtcOk) rtc.adjust(DateTime(Y,Mo,D,h,mi,s));
    req->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *req){
    if (countdownSec > 0 && manualMode == MODE_AUTO)
      saveCycleState((uint32_t)countdownSec);
    req->send(200, "application/json", "{\"ok\":true}");
    req->onDisconnect([](){
      delay(100);
      ESP.restart();
    });
  });

  server.on("/api/export", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!fsOk || !LittleFS.exists(LOG_PATH)){
      req->send(404, "text/plain", "Log tidak tersedia");
      return;
    }
    req->send(LittleFS, LOG_PATH, "text/csv", true);
  });

  server.on("/api/export/clear", HTTP_POST, [](AsyncWebServerRequest *req){
    clearLogFile();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound([](AsyncWebServerRequest *req){ req->send(404, "text/plain", "404"); });
  server.begin();
}

void setup(){
  // WAJIB paling awal: cegah relay active-low nyala saat GPIO masih float
  relaySafeOff();
  pumpOn = false;

  Serial.begin(115200);

  // Baca flag pompa SEBELUM setPump, agar INTERRUPT bisa dicatat
  bool wasPumpOn = loadPumpFlag();
  relaySafeOff();  // pastikan tetap OFF setelah baca NVS

  initBootCount();

  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);

  loadSettings();
  loadLastWater();
  loadManualMode();
  loadCycleState();

  fsOk = initFS();
  if (fsOk) Serial.printf("LittleFS OK. Sisa %u KB\n",
                          (unsigned)((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024));

  if (rtc.begin()){
    rtcOk = true;
    if (rtc.lostPower()){
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  } else {
    Serial.println("RTC DS3231 tidak terdeteksi! Cek wiring I2C.");
  }

  // Setelah FS + RTC siap: tutup sesi terputus lalu catat boot
  logBootEvents(wasPumpOn);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.setSleep(false);
  Serial.print("AP aktif. Buka http://");
  Serial.println(WiFi.softAPIP());

  setupServer();
  printSystemStatus();
}

void loop(){
  esp_task_wdt_reset();

  static unsigned long tLogic = 0;
  if (millis() - tLogic >= 250){
    tLogic = millis();
    updateLogic();
  }

  static unsigned long tWifi = 0;
  if (millis() - tWifi >= WIFI_CHECK_MS){
    tWifi = millis();
    ensureSoftAP();
  }
}
