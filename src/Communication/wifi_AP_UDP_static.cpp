#include <string.h>
#include <stdio.h>
#include <bits/stdc++.h>
/* #include "freertos/FreeRTOS.h"
#include "freertos/task.h" */
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "console_dispatcher.h"
extern "C"{


#define WIFI_SSID       "ESP32_AP"
#define WIFI_PASS       "123456qwerty"
#define WIFI_CHANNEL    1
#define MAX_STA_CONN    1

#define UDP_PORT        4444
#define RX_BUF_SIZE     128

char rx_buffer[RX_BUF_SIZE];
char addr_str[128];
int addr_family = AF_INET;
int ip_protocol = IPPROTO_IP;
int sock;
static const char  * mirror1 = "m1";
static const char  * mirror2 = "m2";

static float mirror1_deg = 0.0f;
static float mirror2_deg = 0.0f;

static bool new_info1 = false;
static bool new_info2 = false;

static const char *TAG = "SOFTAP_UDP";

void wifi_init_softap(){
    ESP_ERROR_CHECK(nvs_flash_init());
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* wifi_config_t wifi_config = {
    .ap = {
        .ssid = WIFI_SSID,
        .ssid_len = strlen(WIFI_SSID),
        .channel = WIFI_CHANNEL,
        .password = WIFI_PASS,
        .max_connection = MAX_STA_CONN,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    }; */

    wifi_config_t wifi_config = {};

    strcpy((char *)wifi_config.ap.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.ap.password, WIFI_PASS);

    wifi_config.ap.ssid_len = strlen(WIFI_SSID);
    wifi_config.ap.channel = WIFI_CHANNEL;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = MAX_STA_CONN;
    wifi_config.ap.beacon_interval = 100;

    if(strlen(WIFI_PASS) == 0){
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "SoftAP started. SSID:%s  PASS:%s",
             WIFI_SSID, WIFI_PASS);
}

void udp_server_set_param(){
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if(sock < 0){
        ESP_LOGE(TAG, "Unable to create socket");
        return;
    }

    bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    ESP_LOGI(TAG, "UDP server listening on port %d", UDP_PORT);
}

void udp_server_read(){
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer)-1, 0, (struct sockaddr *)&source_addr, &socklen);

    if(len < 0){
        ESP_LOGE(TAG, "recvfrom failed");
        close(sock);
        return;
    }
    rx_buffer[len] = 0;
    inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str)-1);
    ESP_LOGI(TAG, "Recieved %d bytes from %s: %s", len, addr_str, rx_buffer);

    const char * reply = "ACK";
    sendto(sock, reply, strlen(reply), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));


    //float deg;
    console_input(rx_buffer);
    /* if(strstr(rx_buffer, mirror1)){
        mirror1_deg = sscanf(rx_buffer, "%f_m1", &deg);
        mirror1_deg = deg;
        new_info1 = true;
        ESP_LOGI(TAG, "Mirror1: %f", mirror1_deg);
    }
    else if(strstr(rx_buffer, mirror2)){
        mirror2_deg = sscanf(rx_buffer, "%f_m2", &deg);
        mirror2_deg = deg;
        new_info2 = true; 
        ESP_LOGI(TAG, "Mirror2: %f", mirror2_deg);
    }
    else{
        new_info1 = false;
        new_info2 = false;
        ESP_LOGW(TAG, "Well...Dunno what this is");
    } */
    //new_info1 = false;
    //new_info2 = false;
}

float get_mirror1_angle(){
    //ESP_LOGI(TAG, "Getting mirror 1 angle");
    return mirror1_deg;
}

float get_mirror2_angle(){
    //ESP_LOGI(TAG, "Getting mirror 2 angle");
    return mirror2_deg;
}

bool new_info_recieved_1(){
    return new_info1;
}

bool new_info_recieved_2(){
    return new_info2;
}

void set_info_recieved1(){
    new_info1 = false;
}

void set_info_recieved2(){
    new_info2 = false;
}
//Extern "C" end
}
