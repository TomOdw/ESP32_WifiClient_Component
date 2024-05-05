/*!
 * @file 	    WifiClient.cpp
 * @brief 	    Singleton wifi interface class
 * @author 	    Tom Christ
 * @date 	    2024-05-02
 * @copyright   Copyright (c) 2024 Tom Christ; MIT License
 * @version	    0.1		Initial Version
 */

#include "WifiClient.h"

#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#include "esp_log.h"
#define TAG "WifiClient"

using namespace std;

WifiClient WifiClient::Singleton;

void WifiClient_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    esp_err_t result;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGD(TAG, "received station start event, connecting...");

        result = esp_wifi_connect();

        if (result != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_connect had an error: %s", esp_err_to_name(result));
        }

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGD(TAG, "received station disconnected event, reconnecting...");
        if(WifiClient::Singleton.isConnected()){
            //Station was connected before, fire disconnected event
            WifiClient::Singleton.fireEvent(WifiClient::Event::DISCONNECTED);
        }
        WifiClient::Singleton.setConnected(false);
        result = esp_wifi_connect();
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_connect had an error: %s", esp_err_to_name(result));
        }

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_WIFI_READY) {
        ESP_LOGD(TAG, "received wifi ready event");

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGD(TAG, "received wifi station connected event");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGD(TAG, "station connected, ip is: " IPSTR,
            IP2STR(&event->ip_info.ip));
        if(!WifiClient::Singleton.isConnected()){
            //Station was not connected before, fire connected event
            WifiClient::Singleton.fireEvent(WifiClient::Event::CONNECTED);
        }
        WifiClient::Singleton.setConnected(true);
    }
}

WifiClient& WifiClient::getInstance()
{
    return Singleton;
}

WifiClient::WifiClient()
{
}

void WifiClient::setConnected(bool connected)
{
    xSemaphoreTake(Singleton.connectedMutex, portMAX_DELAY);
    Singleton.connected = connected;
    xSemaphoreGive(Singleton.connectedMutex);
}

void WifiClient::init(Config const& config)
{
    const static string EXEP_TAG = "WifiClient::init: ";

    //Set log levels, also for system elements
    esp_log_level_set(TAG,LOG_LOCAL_LEVEL);
    esp_log_level_set("wifi",LOG_LOCAL_LEVEL);
    esp_log_level_set("wifi_init",LOG_LOCAL_LEVEL);
    esp_log_level_set("phy_init",LOG_LOCAL_LEVEL);
    esp_log_level_set("esp_netif_handlers",LOG_LOCAL_LEVEL);

    //Create the mutex
    connectedMutex = xSemaphoreCreateMutex();
    
    esp_err_t result;

    //init netif
    result = esp_netif_init();
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "netif init failed with error: " + esp_err_to_name(result));
    }

    //create event loop
    result = esp_event_loop_create_default();
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "event loop create failed with error: " + esp_err_to_name(result));
    }

    //create netif wifi station
    esp_netif_create_default_wifi_sta();

    //initialize wifi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    result = esp_wifi_init(&cfg);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "esp wifi init failed with error: " + esp_err_to_name(result));
    }

    //configure wifi
    wifi_config_t wifiConfig;
    memset(&wifiConfig, 0, sizeof(wifi_config_t));
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    memcpy(wifiConfig.sta.ssid, config.ssid.c_str(), config.ssid.length());
    memcpy(wifiConfig.sta.password, config.password.c_str(), config.password.length());

    result = esp_wifi_set_mode(WIFI_MODE_STA);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "esp wifi set mode failed with error: " + esp_err_to_name(result));
    }

    result = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "esp wifi set conif failed with error: " + esp_err_to_name(result));
    }
    initalized = true;
}

void WifiClient::connect() const
{
    const static string EXEP_TAG = "WifiClient::connect: ";
    esp_err_t result;

    if (!initalized) {
        throw runtime_error(EXEP_TAG + "client not initialized");
    }

    if (isConnected()) {
        // Already connected
        return;
    }

    result = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiClient_event_handler, NULL);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "can't register handler, error: " + esp_err_to_name(result));
    }

    result = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiClient_event_handler, NULL);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "can't register handler, error: " + esp_err_to_name(result));
    }

    result = esp_wifi_start();
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "esp_wifi_start returned error: " + esp_err_to_name(result));
    }
}

void WifiClient::disconnect() const
{
    const static string EXEP_TAG = "WifiClient::disconnect: ";
    esp_err_t result;

    if (!initalized) {
        throw runtime_error(EXEP_TAG + "client not initialized");
    }

    if (!isConnected()) {
        //Already disconnected
        return;
    }

    result = esp_wifi_stop();
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "esp_wifi_stop returned error: " + esp_err_to_name(result));
    }

    result = esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiClient_event_handler);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "can't unregister handler, error: " + esp_err_to_name(result));
    }

    result = esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiClient_event_handler);
    if (result != ESP_OK) {
        throw runtime_error(EXEP_TAG + "can't unregister handler, error: " + esp_err_to_name(result));
    }

    setConnected(false);
}

bool WifiClient::isConnected() const
{
    bool result = false;
    xSemaphoreTake(Singleton.connectedMutex, portMAX_DELAY);
    result = Singleton.connected;
    xSemaphoreGive(Singleton.connectedMutex);
    return result;
}

void WifiClient::registerEventReceiver(QueueHandle_t& queueHandle, uint8_t queueSize){
    if(queueSize == 0){
        throw invalid_argument("WifiClient::registerEventReceiver: Queue size must be greater 0");
    }
    queueHandle = xQueueCreate(queueSize,sizeof(WifiClient::Event));
    if(queueHandle == 0){
        throw runtime_error("WifiClient::registerEventReceiver: Queue could not be created");
    }
    eventReceivers.push_back(&queueHandle);
}

void WifiClient::fireEvent(Event event){
    for(QueueHandle_t* queue: eventReceivers){
        BaseType_t result = xQueueSend(*queue,&event,0);
        if(result != pdTRUE){
            ESP_LOGE(TAG,"Could not fire event, receive queue is full.");
        }
    }
}
