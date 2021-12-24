#include "smooth-playground.h"

#include <esp_wifi.h>  

#include "smooth/core/filesystem/File.h"
#include "smooth/core/filesystem/MountPoint.h"
#include "smooth/core/network/IPv4.h"

#include "util.h"

namespace tol {

using namespace smooth;
using namespace smooth::core;
using namespace smooth::core::ipc;
using namespace smooth::core::filesystem;
using namespace smooth::application::network;

using namespace std::chrono;

void App::init() {
  Serial.begin(SERIAL_BAUD); // XXX tempwell, not if uart strip tho...
  lg.initOutput("Serial");  //logging can be done after here
  logger = std::make_unique<Logger>();
  // TODO figure out heap situation. ipc0 using 27kb heap before anything inits - why?
  // tbf it also uses 10.5kb in their minimal example so maybe it just is what it is? still weird tho.
  Logger::logHeap();
  Logger::logHeapRegions();
  
  Application::init();
  logging::Log::error("App", "init");

  wifiCrap();
  // esp_log_level_set(mqtt_log_tag, static_cast<esp_log_level_t>(CONFIG_SMOOTH_MQTT_LOGGING_LEVEL)); //<- remember set up my loggers w esp then easy set lvl etc
   
  Logger::logHeap();
  Logger::logHeapRegions();
  
  device = std::make_unique<Device>(deviceId); // this is getting emptied for now (and most stuff native support in idf) but might restore some
  // deviceLoop = std::make_unique<FnTask>("device->loop", 25, [this]{ this->device->loop(); }); // XXX replace with Task
  
  // fs = std::make_unique<SPIFlash>(FlashMount::instance(), "app_storage", 10, true);
  // fs->mount();
  
  // const auto filePath = FlashMount::instance().mount_point() / "test.txt"; // Path
  // const auto webRoot = FlashMount::instance().mount_point() / "web"; // Path
  
  // static constexpr int MaxHeaderSize = 1024;
  // static constexpr int ContentChunkSize = 2048;
  // static constexpr int MaxResponses = 5;
  // http::HTTPServerConfig cfg{ webRoot, { "index.html" }, { ".html" }, {}, MaxHeaderSize,
  //                       ContentChunkSize, MaxResponses };
  
  // auto templater = std::make_shared<DataRetriever>();
  // templater->add("{{title}}", "Smooth - a C++ framework for building apps on ESP-IDF");
  // templater->add("{{message}}", "Congratulations, you're browsing a web page on your ESP32 via Smooth framework by");
  // templater->add("{{github_url}}", "https://github.com/PerMalmberg");
  // templater->add("{{author}}", "Per Malmberg");
  // templater->add("{{from_template}}", "This, and other text on this page are replaced on the fly from a template.");
  
  // insecureServer = std::make_unique<http::InsecureServer>(*this, cfg);
  // insecureServer->start(3, 5, std::make_shared<network::IPv4>("0.0.0.0", 8080));
  
  // auto crap = filesystem::File(std::string("muh.json"));
  // if(!file.write("not even json shiite"))
  //   logging::Log::error("FS", "{}", "Failed to write crap");

  // auto fmt = [](const std::string& loc, const std::string& lvl, const std::string& txt) {
  //                 return std::string("[" + Ansi<Yellow>(lvl) + "] " + Ansi<Blue>(loc) + "\t" + txt); };
  // auto logThroughSmooth = [](const std::string& text) {
  //                           logging::Log::log('I', "", (std::string)text.c_str()); };
  // lg.moreDest.emplace_back("IDF/smooth", logThroughSmooth, fmt);
  // XXX works but needs further fixes (newline)

  
  

  // cmdRunner = std::make_shared<CommandRunner>();
  // cmdRunner->add("testuh", [](const std::string& cmd, const std::vector<std::string>& data) {
  //                               for(auto& s: data)
  //                                 logging::Log::error(cmd, "{}", s);
  //                             });
  
  // auto data = fileToData("mqtt.json");
  // if(!data.empty()) {
  //   auto mqttCfg = json::json::parse(data.data(), nullptr, false);
  //   mqtt = std::make_unique<Mqtt>(deviceId, *this, *cmdRunner, mqttCfg);
  // }

  scheduler = std::make_unique<Scheduler>(deviceId);
  scheduler->start();

  timeSync.start();

  Logger::logHeap();
  Logger::logHeapRegions();
  // SystemStatistics::instance().dump(); // apparently causes stack overflow now
  DEBUG("END SETUP");
}


void App::onEvent(const network::NetworkStatus& e) {
  if (e.get_event() == network::NetworkEvent::GOT_IP) { // guess store so gen knowledge of conn up.  Events gather and once all required for task x start, go
    auto& wifi = Application::get_wifi(); // or just start and let block at each needed point but something like network will prob sub and handle individually...  Buffers is q whether wrap in diff structures dep on user, or pass out queues
    Publisher<IPAddress>::publish(IPAddress(wifi.get_local_ip()));
  } else if(e.get_event() == network::NetworkEvent::DISCONNECTED) {
    DEBUG("NETWORK EVENT: disconnected");
    //turn off w/e services...
  }
}

void App::wifiCrap() {
  wifiOld = std::make_unique<WifiConnection>("pixTol-proto", "wifi");

  // needed to fetch from nvs...
  // smooth doesnt call this until connect. wifimanager should do earlier but was apparently luck.
  wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifiCfg);
  
  wifi_config_t conf;
  auto err = esp_wifi_get_config(WIFI_IF_STA, &conf);
  
  // char errString[64]{};
  // esp_err_to_name_r(err, errString, 64);
  // lg.f("Wifi", Log::INFO, "Errcode %s", errString);
  auto ssid = (std::string)reinterpret_cast<const char*>(conf.sta.ssid);
  auto pw = (std::string)reinterpret_cast<const char*>(conf.sta.password);
  

  if(pw != "") { // use stored creds
    auto& wifi = get_wifi();
    wifi.set_host_name("pixTol-proto");
    wifi.set_auto_connect(true); // more like auto reconnect...
    wifi.set_ap_credentials(ssid.c_str(), pw.c_str());
    wifi.connect_to_ap(); // broken and get spam about old style event loop
  } else {
    wifiOld->start(); // smooth-wifi acting iffy (depr event loop yada but literally new api tho?)
    // wifiOld->setup(); // smooth-wifi acting iffy (depr event loop yada but literally new api tho?)
    // but can still use it to fetch connection info...
  }
    
}

std::vector<uint8_t> fileToData(const std::string& path) {
  auto f = filesystem::File(path);
  std::vector<uint8_t> data;
  if(f.exists()) {
    f.read(data);
    if(!data.empty()) {
      data.push_back(0); // json uh thing. // what?
    }
  }
  return data;
}


std::string timeString(system_clock::time_point t) {
  auto in_time_t = system_clock::to_time_t(t);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
  return ss.str();
}


}
