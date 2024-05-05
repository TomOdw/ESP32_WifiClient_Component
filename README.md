# ESP32_WifiClient_Component
**ESP-IDF component as a cpp interface class for FreeRTOS**

# Info
- IDE: VS Code with ESP-IDF Extension
- ESP-IDF version: 5.2.1

# Usage
- Must be placed within the ESP-IDF projects "components" folder
- nvs_flash_init() must be called before component init (and I had no luck calling it in C++, so I called it in C before starting the C++ application...)

# Example
```c++
void run()
{
    WifiClient::Config myConfig;
    myConfig.password = "WIFI PASSWORD";
    myConfig.ssid = "WIFI SSID";

    WifiClient& wifiClient = WifiClient::getInstance();

    QueueHandle_t rcvEventQueue;

    try{
        wifiClient.registerEventReceiver(rcvEventQueue);
        wifiClient.init(myConfig);
        wifiClient.connect();

        WifiClient::Event event;

        while(true){
            BaseType_t result = xQueueReceive(rcvEventQueue,&event,portMAX_DELAY);
            if(result == pdTRUE){
                if(event == WifiClient::Event::CONNECTED){
                    ESP_LOGI(TAG, "received connected");
                }
                if(event == WifiClient::Event::DISCONNECTED){
                    ESP_LOGI(TAG, "received disconnected");
                }
            }
        }
    }catch(...){
        //TODO Error Handling
    }
}
```