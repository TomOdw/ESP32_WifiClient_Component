/*!
 * @file 	    WifiClient.h
 * @brief 	    Singleton wifi interface class
 * @author 	    Tom Christ
 * @date 	    2024-05-05
 * @copyright   Copyright (c) 2024 Tom Christ; MIT License
 * @version	    0.1		Initial Version
 */

#ifndef WifiClient_H_
#define WifiClient_H_

#include <stdexcept>
#include <string>
#include <cstring>
#include <vector>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"

/*!
 * @brief   Event Handler for esp_wifi
 * 
 *          Sets connected and disconnected and fires WifiClientEvents to
 *          registered receivers.
 */
extern "C" void WifiClient_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);

/*!
 * @class   WifiClient
 * @brief   Singleton Interface Class for esp_wifi client 
 */
class WifiClient {

/*!
 * @brief   Event Handler as friend function
 */
friend void WifiClient_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);

/** ******************/
/** PUBLIC TYPEDEFS **/
/** ******************/
public:
    /*!
     * @brief   Struct which containes the Configuration values
     */
    struct Config{
        std::string ssid = "";      /*!< @brief WIFI SSID*/
        std::string password = "";  /*!< @brief WIFI PASSWORD*/
    };

    /*!
     * @brief   Enum Class which stores events.
     */
    enum class Event{
        CONNECTED,  /*!< @brief Event is fired on client connected*/
        DISCONNECTED /*< @brief Event is fired on client disconnected*/
    };

/** ****************************/
/** PRIVATE STATIC ATTRIBUTES **/
/** ****************************/
private:
    static WifiClient Singleton;    /*!< @brief Singleton Instance */

/** ************************/
/** PUBLIC STATIC METHODS **/
/** ************************/
public:
    /*!
     * @brief   Get the Singleton Instance
     * 
     * @return  WifiClient& Singleton Instance
     */
    static WifiClient& getInstance();

/** *************************/
/** PRIVATE STATIC METHODS **/
/** *************************/
private:
    /*!
     * @brief   Set the the connected attribute
     *
     *          attribute is protected by mutex, method blocks
     *          for portMAX_DELAY if mutex is taken.
     * 
     * @param connected 
     */
    static void setConnected(bool connected); 

/** **************/
/** CONSTRUCTOR **/
/** **************/
private:
    /*!
     * @brief   Construct a new Wifi Client object
     */
    WifiClient();

/** *************/
/** ATTRIBUTES **/
/** *************/
private:
    SemaphoreHandle_t connectedMutex;   /*!< @brief Mutex for connected attribute*/
    bool connected; /*!< @brief connected attribute*/
    bool initalized; /*!< @brief initialized attribute*/
    std::vector<QueueHandle_t*> eventReceivers; /*!< @brief stors all queue handles which receive the events*/

/** *****************/
/** PUBLIC METHODS **/
/** *****************/
public:
    /*!
     * @brief   Initializes the singleton object with the passed config
     * 
     *          Sets log level to LOG_LOCAL_LEVEL (defined in cpp file).
     *          NVS has to be initialized before this method is called.
     * 
     * @throws  runtime_error if initialization failed.
     * 
     * @param   config Config object, holds ssid and password
     */
    void init(Config const& config);

    /*!
     * @brief   Connectes the Client
     *
     *          Client has to be initalized first. 
     * 
     * @throws  runtime_error if connect failed.
     */
    void connect() const;

    /*!
     * @brief   Disconnected the client
     *
     * @throws  runtime_error if disconnect failed.
     */
    void disconnect() const;

    /*!
     * @brief   Returns the clients connection status
     * 
     *          connected attribute is protected by mutex, method blocks
     *          for portMAX_DELAY if mutex is taken.
     * 
     * @return  true if client is connected
     * @return  false if client is not connected 
     */
    bool isConnected() const;

    /*!
     * @brief   Register a queue handle in which disconnected/ connected
     *          events are sent in.
     * 
     * @param   queueHandle returns queueHandle by reference
     * @param   queueSize default is 1
     * @throws  invalid_argument if queueSize is 0
     * @throws  runtime_error if queue could not be created
     */
    void registerEventReceiver(QueueHandle_t& queueHandle, uint8_t queueSize = 1);

/** ******************/
/** PRIVATE METHODS **/
/** ******************/
private:
    /*!
     * @brief   Fires a passed event to all registered queues.
     *
     *          Method does not block, if a queue is full, an error
     *          log message is printed.
     * 
     * @param   event to send to all registeres queues. 
     */
    void fireEvent(Event event);
};

#endif /* WifiClient_H_ */