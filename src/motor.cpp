#include "stdio.h"
#include <vector>


//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"




extern "C"{
#include "motor.h" 



static const char * TAG = "MOTOR";

void PolarLightStepper::configure(PolarLightStepper_config_t * pls_step_cfd){
    step_pin = pls_step_cfd->step_pin;
    dir_pin = pls_step_cfd->dir_pin;
    en_pin = pls_step_cfd->en_pin;
    steps = pls_step_cfd->steps;
    gearbox_ratio = pls_step_cfd->gearbox;
    speed = pls_step_cfd->speed;
    accel = pls_step_cfd->accel;

    DendoStepper_config_t step_cfd = {
        .stepPin = step_pin,
        .dirPin = dir_pin,
        .enPin = en_pin,
        .miStep = MICROSTEP_1,
        .stepAngle = (360.0f / (float)steps) / (float)(gearbox_ratio)//0.05625f
    };
    stepper_test.config(&step_cfd);
    stepper_test.init();
    stepper_test.setSpeed(speed, accel, accel);
    stepper_test.disableMotor();
    ESP_LOGW("TEST", "SUCC!");
}

void PolarLightStepper::operate(float target_angle_, bool position_recieved){
    float deg = target_angle_;
    PolarLightStepper::motor_state = position_recieved;
    ESP_LOGW(TAG, "Motor state: %d", motor_state);

        if(deg != 0.0f && motor_state == true)
        {
            motor_state = false;
            int32_t steps = degreesToSteps(deg);
            
            if(steps != 0){
                stepper_test.runPos(steps);
                while(stepper_test.getState() !=  IDLE){
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
               return;
            }
        }
}
void PolarLightStepper::operate(float target_angle_){
    float deg = target_angle_;
        if(deg != 0.0f)
        {
            
            int32_t steps = degreesToSteps(deg);
            if(steps != 0){
                
                stepper_test.runPos(steps);
                /* while(stepper_test.getState() !=  IDLE){
                    vTaskDelay(pdMS_TO_TICKS(50));
                } */
                //stepper_test.disableMotor();
                //ESP_LOGE(TAG, "Motor has finished its movement!");    
                //return;
            }
        }
}

void PolarLightStepper::operate_both(float target_angle_){
    float deg = target_angle_;
        if(deg != 0.0f)
        {
            int32_t steps = degreesToSteps(deg);
            if(steps != 0){
                stepper_test.runPos(steps);
            }
            if(stepper_test.getState() == IDLE) stepper_test.disableMotor();
            ESP_LOGE(TAG, "Motor has finished its movement!");    

        }
}

bool PolarLightStepper::is_busy(){
    return stepper_test.getState() != IDLE;
}

void PolarLightStepper::apply_mode(sys_command_t * sys_cmd, float target_angle_){
    switch(sys_cmd->lens_mode){
        case(LENS_MODE_MOVE_REL):{
            ESP_LOGW(TAG, "RELATIVE MODE");
            operate(target_angle_);
            break;
        }
        case(LENS_MODE_MOVE_ABS):{
            ESP_LOGW(TAG, "ABSOLUTE MODE");
            stepper_test.runAbs(degreesToSteps(target_angle_));
            break;
        }
        case(LENS_MODE_MOVE_STEP):{
            ESP_LOGW(TAG, "STEP MODE");
            /* while(sys_cmd->lens_mode == LENS_MODE_MOVE_STEP){
                operate(target_angle_);
                vTaskDelay(pdMS_TO_TICKS(sys_cmd->period));
            } */
           if(now - last_tick >= sys_cmd->period){
                operate(target_angle_);
                last_tick = now;
            }
            break;
        }
        case(LENS_MODE_MOVE_SEQUENCE):{
            ESP_LOGW(TAG, "SEQUENCE MODE");
            for(int i = 0; i < sys_cmd->value_seq_mode_vector.size(); i++){
                printf("Executing angle: %f", sys_cmd->value_seq_mode_vector[i]);
                operate(sys_cmd->value_seq_mode_vector[i]);
                if(i == sys_cmd->value_seq_mode_vector.size()-1) continue;
                vTaskDelay(pdMS_TO_TICKS(sys_cmd->period));
            }
            break;
        }
        case(LENS_MODE_REZERO):{
            ESP_LOGW(TAG, "REZERO MODE");
            stepper_test.runAbs(0);
            break;
        }
        default: break;
    }
} 


inline int32_t PolarLightStepper::degreesToSteps(float _degrees){
    return static_cast<int32_t>(lroundf((_degrees / 360.0f) * steps * gearbox_ratio));
}

///Extern "C" end
}