
#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "transport_wifi.h"
#include "controller_input.h"
#include "rover_controller.h"
#include "leds.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "web_server.h"
#include "config.h"
#include "transport_lora.h"

static const char *TAG = "main";

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    
    switch (event_id) {
        case WIFI_EVENT_AP_START:
            webserver_start();
            break;
        case WIFI_EVENT_AP_STOP:
            webserver_stop();
            break;
        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGW(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGW(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
    }
}

static void start_ap(void) {
    esp_netif_t* netif = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
    esp_netif_ip_info_t ip_info;

    ip_info.ip.addr = esp_ip4addr_aton(ROVER_CONTROLLER_STATIC_IP);;
   	IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
   	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = 3,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    if (strlen(AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", AP_SSID, AP_PASS);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    leds_init();
    transport_lora_init();
    webserver_init();
    transport_ws_init();
    rover_controller_init();
    transport_ws_start();
    
    start_ap();
}
