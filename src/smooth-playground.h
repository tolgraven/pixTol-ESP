#pragma once

// #include <AsyncUDP.h>

#include <ctime>
#include <utility>
#include <iostream>
#include <iomanip>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>

#include <nlohmann/json.hpp>

#include <fmt/chrono.h>

#include "smooth/core/ipc/Publisher.h"
#include "smooth/core/ipc/SubscribingTaskEventQueue.h"

#include "smooth/core/sntp/Sntp.h"
#include "smooth/core/sntp/TimeSyncEvent.h"

#include <smooth/application/network/mqtt/MqttClient.h>
#include <smooth/core/network/IPv4.h>

#include "smooth/core/Task.h"
#include "smooth/core/task_priorities.h"
#include "smooth/core/SystemStatistics.h" //could this still cause trouble? was lacking pragma once
#include "smooth/core/Application.h"

#include "smooth/core/filesystem/SPIFlash.h"
#include "smooth/core/json/JsonFile.h"

#include "smooth/application/network/http/HTTPServer.h"
#include "smooth/application/network/http/HTTPServerClient.h"
#include "smooth/application/network/http/HTTPProtocol.h"


#include "task.h"
#include "device.h"
#include "scheduler.h"
#include "log.h"
#include "commands.h"
#include "util.h"
#include "logger.h"


namespace tol {

using namespace smooth;
using namespace smooth::core;
using namespace smooth::core::ipc;
using namespace smooth::core::filesystem;
using namespace smooth::application::network;
using namespace std::chrono;

namespace json = nlohmann;
using LOG = logging::Log;


std::string timeString(system_clock::time_point t = system_clock::now());

std::vector<uint8_t> fileToData(const std::string& path);

class Mqtt;

class DataRetriever: public http::ITemplateDataRetriever {
  std::unordered_map<std::string, std::string> data{};
  public:
  std::string get(const std::string& key) const override {
      std::string res;
      try {
          res = data.at(key);
      } catch (std::out_of_range&) { }
      return res;
  }

  void add(const std::string&& key, std::string&& value) {
      data.emplace(key, value);
  }
};


class App: public core::Application,
           public Sub<network::NetworkStatus>,
           public Sub<sntp::TimeSyncEvent> {

  sntp::Sntp timeSync{std::vector<std::string>{"pool.ntp.org", "time.euro.apple.com"}};

  const char* deviceId = "pixTol";
  std::unique_ptr<Device>  device;
  std::unique_ptr<FnTask>  deviceLoop; // for now
  
  std::unique_ptr<Logger>  logger;

  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<WifiConnection> wifiOld;
  // console_shit_yo;
  // AsyncUDP udp;
  std::unique_ptr<Mqtt> mqtt{};
  
  std::unique_ptr<SPIFlash> fs;
  using Client = http::HTTPServerClient;
  using Protocol = http::HTTPProtocol;
  std::unique_ptr<http::InsecureServer> insecureServer{};
  
  std::shared_ptr<CommandRunner> cmdRunner;

  public:
    App(): Application(APPLICATION_BASE_PRIO - 1, seconds(60)),
      Sub<network::NetworkStatus>(this),
      Sub<sntp::TimeSyncEvent>(this)
    {}

    void init() override;

    void tick() override {
      // logging::Log::info("Stats++", "\n{}", moreStatz());
      // XXX uncommented cause causing crashes. Almost eclusively loadprohibited
      // but at least once indicating heap poisoning!! so definitely investigate
      // seems to run fine tho with no major errors tho without these calls...

      // util::logHeap();
      // heap_caps_print_heap_info(MALLOC_CAP_8BIT);
      // vTaskDelay(5);
      // util::logHeapRegions();
      // SystemStatistics::instance().dump();
    }

    void onEvent(const network::NetworkStatus& e) override;
    void onEvent(const sntp::TimeSyncEvent& e) override {
      logging::Log::info("Current time", "{}", timeString()); // appears someone else listening to this and actually sets time whoo
    }

    void wifiCrap();
};


// ok so steal this from G3 now then try to rework/incorporate...
using namespace smooth::application::network;
using namespace smooth::core::logging;

// cfg and file handling should not be in scope of this class. maybe... nah
// well it must have access to file ops but go through usual channels.
// want broker functionality?
class Mqtt: public ipc::IEventListener<mqtt::MQTTData> { // then sub outgoings as well. Guess Logging best handled w queue + subscribers as well f.ex.
  public:
  Mqtt(std::string id, core::Task& task, CommandRunner& cmdRunner, json::json& cfg):
    id(std::move(id)), task(task), cmdRunner(&cmdRunner), cfg(cfg),
    incoming(MQTTQueue::create(10, task, *this)) {}
  ~Mqtt() {
    if(isConnected()) client->disconnect();
  } // guy had an override here

  void start() {
    if(client) return;
    core::json::JsonFile f{mqttCfgPath};
    if(!f.exists()) writeDefault(f);
    auto& v = f.value();
    auto keep_alive = std::chrono::seconds{5};
    std::string broker = "192.168.1.100";
    auto port = 1883;
    // auto keep_alive = seconds{valueOr(cfg, "keep_alive_seconds", 5)}; // no need for unworking defaults w this so?
    // auto broker = valueOr(cfg["broker"], "address", "192.168.1.100");
    // auto port = valueOr(cfg["broker"], "port", 1883);

    if(broker.empty()) { logging::Log::error("Mqtt", "No broker specified");
    } else {             logging::Log::info("Mqtt", "Starting MQTT client, id {}", id);
      client = std::make_unique<mqtt::MqttClient>(id, keep_alive, 8 * 1024,
                                                  APPLICATION_BASE_PRIO, incoming);
      for(const auto& topic: subs)
        client->subscribe(topic, qos);
      client->connect_to(std::make_shared<network::IPv4>(broker, port), true);
    }
  }

  // void event(networkup...) { start(); }

  void event(const mqtt::MQTTData& data) {
    const auto& payload = mqtt::MqttClient::get_payload(data);
    cmdRunner->execute(data.first, payload); // cmdRunner.execute(data.first, "yada", payload);
  }

  void addSub(const std::string& topic) {
    if(std::find(subs.begin(), subs.end(), topic) == subs.end()) {
      subs.emplace_back(topic);
      client->subscribe(topic, qos);
    }
  }
  [[nodiscard]] bool isConnected() const { return client && client->is_connected(); }

  private:
  void startMqtt();
  void addTime(json::json& v) {
    v["timestamp"] = std::to_string(static_cast<int64_t>(system_clock::now().time_since_epoch().count()));
  }
  void send(const std::string& topic, json::json& v) {
    addTime(v);
    client->publish(topic, v.dump(), qos, false);
  }

  void writeDefault(core::json::JsonFile& f) { // private no? also would (if needed) do v different full falback to cfg mode etc yada?
    auto& v = f.value();
    v["keep_alive_seconds"] = 5;
    v["broker"]["address"] = "192.168.1.100";
    v["broker"]["port"] = 1883;
    if(!f.save()) logging::Log::error("MQTT", "Could not write default MQTT config");
  }

  std::string id;
  core::Task& task;
  std::shared_ptr<CommandRunner> cmdRunner;

  json::json cfg; // some method to modify this and apply new settings?

  using MQTTQueue = ipc::TaskEventQueue<mqtt::MQTTData>;
  std::shared_ptr<MQTTQueue> incoming;
  std::unique_ptr<mqtt::MqttClient> client{};
  std::vector<std::string> subs{};

  // static const auto mqtt_config = filesystem::SPIFlash::instance().mount_point() / "mqtt.json";
  inline static constexpr const char* mqttCfgPath = "mqtt.json";
  inline static auto qos = mqtt::QoS::EXACTLY_ONCE;
};


}
