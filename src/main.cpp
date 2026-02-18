#include "stdio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "Config.hpp"

#include "Communication/console_dispatcher.h"
extern "C"{

#include "Communication/wifi_AP_UDP_static.h"
#include "Communication/console_uart0.h"
#include "motor.h"

TaskHandle_t xMotorTask_Handle_t;
QueueHandle_t xQueue_cmd;

void xWifiTask(void * param);
void xMotorTask_TEST(void * param);
void xConsoleTask(void * param);
void xControlTask(void * param);



typedef enum{
    STATE_IDLE,
    STATE_CALIB,
    STATE_RUNNING
} system_state_t;
volatile system_state_t sys_state = STATE_IDLE;

/* typedef enum{
    MOTOR_IDLE,
    MOTOR_SINGLE_AXIS, 
    MOTOR_BOTH_OUTWARD_INWARD, 
    MOTOR_RESET
} motor_state_t;
volatile motor_state_t motor_state = MOTOR_IDLE; */

void xControlTask(void * param){
    
    extern sys_command_t sys_cmd;
    static const char * TAG = "CONTROL TASK";
    //extern sys_event_t sys_event;
    while(1){
        ESP_LOGI(TAG, "Event is: %d", sys_cmd.sys_event);
        /* if(motor_state == MOTOR_RESET){
            motor_state = MOTOR_IDLE;
            sys_event = EVENT_NONE;
            console_set_lens_fst_zero_angle();
            console_set_lens_scd_zero_angle();
            console_set_lens_both_zero_angle();
        } */
        if(sys_cmd.sys_event == EVENT_RESET){
            sys_cmd.lens_target = LENS_TARGET_NONE;
            sys_cmd.sys_event = EVENT_NONE;
            sys_cmd.lens_mode = LENS_MODE_NONE;
            sys_cmd.lens_couple = COUPLE_NONE;
            console_set_lens_fst_zero_angle();
            console_set_lens_scd_zero_angle();
            console_set_lens_both_zero_angle();
        }
        switch(sys_state){
            case STATE_IDLE:{
                sys_cmd.lens_mode = LENS_MODE_NONE;
                sys_cmd.lens_target = LENS_TARGET_NONE;
                switch(sys_cmd.sys_event){
                    case EVENT_START:{
                        ESP_LOGI(TAG, "Changing to START!!!!!");
                        sys_state = STATE_RUNNING;
                        break;
                    }
                    case EVENT_CALIB_BEGIN:{
                        ESP_LOGI(TAG, "Changing to CALIB!!!!!");
                        sys_state = STATE_CALIB;
                        break;
                    }
                    default:{
                        //ESP_LOGI(TAG, "NONE IDLE!!!!!");
                        break;
                    }
                }
                sys_cmd.lens_mode = LENS_MODE_NONE;
                ESP_LOGI(TAG, "STATE IDLE!!!!!");
                break;
            }
            case STATE_CALIB:{
                switch(sys_cmd.sys_event){
                    case EVENT_CALIB_EXIT:{
                        ESP_LOGI(TAG, "Changing to IDLE");
                        sys_state = STATE_IDLE;
                        break;
                    }
                    case EVENT_LENS:{
                        ESP_LOGI(TAG, "Moving lens in CALIB mode");
                        xTaskNotifyGive(xMotorTask_Handle_t);
                        break;
                    }
                    default:{
                        //ESP_LOGI(TAG, "NONE CALIB!!!!!");
                        break;
                    }
                }
                ESP_LOGI(TAG, "STATE CALIB!!!!!");
                break;
            }
            case STATE_RUNNING:{
                switch(sys_cmd.sys_event){
                    case EVENT_STOP:{
                        ESP_LOGI(TAG, "Changing to IDLE!!!!!");
                        sys_state = STATE_IDLE;
                        break;
                    }
                    case EVENT_LENS:{
                        ESP_LOGI(TAG, "Moving lens in RUNNING mode");
                        xTaskNotifyGive(xMotorTask_Handle_t);
                        break;
                    }
                    default:{
                        //ESP_LOGI(TAG, "NONE RUNNING!!!!!");
                        break;
                    }
                }
                ESP_LOGI(TAG, "STATE RUNNING!!!!!");
                break;
            }
        }
        printf("\n");
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void app_main() {
     gpio_config_t led_t = {
        .pin_bit_mask = (1ULL << 2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&led_t));
    gpio_set_level(GPIO_NUM_2, 0);
    xQueue_cmd = xQueueCreate(1, sizeof(uint64_t));


    xTaskCreatePinnedToCore(xControlTask, "CONTROL", XT_STACK_MIN_SIZE * 3, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(xConsoleTask, "CONSOLE", XT_STACK_MIN_SIZE * 5, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(xWifiTask, "WIFI", XT_STACK_MIN_SIZE * 5, NULL, 1, NULL, 0);

    xTaskCreatePinnedToCore(xMotorTask_TEST, "xMotorTask_TEST", XT_STACK_MIN_SIZE * 5, NULL, 1, &xMotorTask_Handle_t, 1);

    

}

void xMotorTask_TEST(void * param){
    PolarLightStepper lens1;
    PolarLightStepper lens2;
    PolarLightStepper_config_t lens1_cfg = {
        .step_pin = LENS_1_STEP_PIN,
        .dir_pin = LENS_1_DIR_PIN,
        .en_pin = LENS_1_EN_PIN,
        .steps = STEPS,
        .gearbox = GEARBOX,
        .speed = SPEED,
        .accel = ACCEL
    };
    PolarLightStepper_config_t lens2_cfg = {
        .step_pin = LENS_2_STEP_PIN,
        .dir_pin = LENS_2_DIR_PIN,
        .en_pin = LENS_2_EN_PIN,
        .steps = STEPS,
        .gearbox = GEARBOX,
        .speed = SPEED,
        .accel = ACCEL
    };
    lens1.configure(&lens1_cfg);
    lens2.configure(&lens2_cfg);
    static const char * TAG = "LENS SYSTEM";
    extern sys_command_t sys_cmd;
    while(1){
       switch(sys_cmd.lens_target){
        case(LENS_TARGET_FIRST):{
            ESP_LOGE(TAG, "LENS - FIRST");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            gpio_set_level(GPIO_NUM_2, 1);
            lens1.apply_mode(&sys_cmd, console_get_lens_fst_move_angle());
            /* while(lens1.is_busy() || lens2.is_busy()){
                vTaskDelay(pdMS_TO_TICKS(20));
            } */
            gpio_set_level(GPIO_NUM_2, 0);
            if(sys_cmd.lens_mode == LENS_MODE_MOVE_STEP){
                printf("Test step");
                break;
            }
            else{
                printf("Default mode");
                sys_cmd.lens_target = LENS_TARGET_NONE;
                sys_cmd.sys_event = EVENT_RESET;
                break;
            }
        }
        case(LENS_TARGET_SECOND):{
            ESP_LOGE(TAG, "LENS - SECOND");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            gpio_set_level(GPIO_NUM_2, 1);
            lens2.apply_mode(&sys_cmd, console_get_lens_scd_move_angle());
            /* while(lens1.is_busy() || lens2.is_busy()){
                vTaskDelay(pdMS_TO_TICKS(20));
            } */
            gpio_set_level(GPIO_NUM_2, 0);
            sys_cmd.lens_target = LENS_TARGET_NONE;
            sys_cmd.sys_event = EVENT_RESET;
            break;
        }
        case(LENS_TARGET_BOTH):{
            //
            ESP_LOGE(TAG, "LENS - BOTH");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            gpio_set_level(GPIO_NUM_2, 1);
            switch(sys_cmd.lens_couple){
                case(COUPLE_SYMMETRIC):{
                   ESP_LOGE(TAG, "Coupling - Symmetric");
                   lens1.apply_mode(&sys_cmd, console_get_lens_both_move_angle());
                   lens2.apply_mode(&sys_cmd, console_get_lens_both_move_angle());
                   break;
                }
                case(COUPLE_ASYMMETRIC):{
                    ESP_LOGE(TAG, "Coupling - Asymmetric");
                    lens1.apply_mode(&sys_cmd, console_get_lens_both_move_angle());
                    lens2.apply_mode(&sys_cmd, (-1) * console_get_lens_both_move_angle());
                    break;
                }
                default: break;
            }
            /* while(lens1.is_busy() || lens2.is_busy()){
                vTaskDelay(pdMS_TO_TICKS(20));
            } */
            gpio_set_level(GPIO_NUM_2, 0);
            sys_cmd.lens_target = LENS_TARGET_NONE;
            sys_cmd.sys_event = EVENT_RESET;
            break;
        }
        default: break;
       }
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}



void xWifiTask(void * param){
    wifi_init_softap();
    udp_server_set_param();
    while(1){
        udp_server_read();
        vTaskDelay(pdMS_TO_TICKS(25));
    }
} 

void xConsoleTask(void * param){
    console_uart0_init();
    while(1){
       console_uart0_read();
       vTaskDelay(pdMS_TO_TICKS(50));
    }
}

///Extern "C" end
}
