#pragma once
#include <inttypes.h>
#include "DendoStepper.h"
#include "Communication/console_dispatcher.h"


typedef struct{
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t en_pin;
    int steps;
    int gearbox;
    int speed;
    int accel;
} PolarLightStepper_config_t;
    
class PolarLightStepper{
    public:
        void configure(PolarLightStepper_config_t * pls_step_cfd);
        void operate(float target_angle_, bool position_recieved);
        void operate(float target_angle_);
        void operate_both(float target_angle_);
        void apply_mode(sys_command_t * sys_cmd, float target_angle_); //needs console_dispatcher to operate. Shitty design - kaif-vkusno 
        bool is_busy();
        
    private:
        DendoStepper stepper_test;
        uint8_t step_pin;
        uint8_t dir_pin;
        uint8_t en_pin;
        int steps;
        int gearbox_ratio;
        int speed;
        int accel;
        bool motor_state = false;
        bool motor_busy = true;
        inline int32_t degreesToSteps(float _degrees);
        float current_value = 0;
        //step mode variables
        TickType_t last_tick = 0;
        TickType_t now = xTaskGetTickCount();
        bool step_mode_activate = false;
        float step_angle = 0;
        uint32_t step_period = 0;
        
};