#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

// #include "smooth/core/Task.h"
// #include "smooth/core/ipc/Publisher.h"


static EventGroupHandle_t s_wifi_event_group; /* FreeRTOS event group to signal when we are connected*/

/* The event group allows multiple bits for each event, but we only care about two events: */
#define WIFI_CONNECTED_BIT BIT0   // * - we are connected to the AP with an IP
#define WIFI_FAIL_BIT      BIT1   // * - we failed to connect after the maximum amount of retries

static const char *TAG = "wifi station";

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "WIFI GOT got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    uint32_t ip = 0;
    memcpy(&ip, &event->ip_info.ip, 4);
    // smooth::core::ipc::Publisher<IPAddress>::publish(IPAddress(ip));
  }
}

void wifi_init_sta() {
    ESP_LOGI(TAG, "try subscribe to wifi events: %s %s", "no", "yes");
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            // portMAX_DELAY);
            pdMS_TO_TICKS(10000));

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT) { ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", "no", "yes");
    } else if (bits & WIFI_FAIL_BIT) { ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", "no", "yes");
    } else { ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}
