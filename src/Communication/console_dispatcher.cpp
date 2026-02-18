#include <stdint.h>
#include <string.h>
#include <bits/stdc++.h>
#include <vector>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "console_dispatcher.h"

#define ARRAY_SIZE(lens_table) (sizeof(lens_table) / sizeof((lens_table)[0]))


static const char * TAG = "LENS_CONSOLE_DISPATCHER";

volatile sys_event_t sys_event = EVENT_NONE;
volatile lens_mode_t lens_mode = LENS_MODE_NONE;
volatile lens_target_t lens_target = LENS_TARGET_NONE;
volatile lens_coupling_t lens_couple = COUPLE_NONE;
sys_command_t sys_cmd;

static float lens_fst_deg_val;
static float lens_scd_deg_val;
static float lens_both_deg_val;

static int32_t lens_period;

///Layer_0
static const char * sys_call = "sys";


static const char * sys_start = "start";
static const char * sys_stop = "stop"; 
static const char * sys_save = "save"; //Save changes in config
static const char * sys_default = "default"; //Return to default values



static const char * calib_call = "calib"; //Enter Calib mode
static const char * calib_begin_str = "begin"; // Calib mode BEGIN
static const char * calib_exit_str = "exit"; //EXIT Calib mode



static const char * lens_call = "lens";
//Action mode
static const char * lens_action_move = "move";
static const char * lens_action_rezero = "zero"; 
//Coupling mode
static const char * lens_coupling_call_symmetric = "sym"; 
static const char * lens_coupling_call_asymmetric = "asym";
//Operational mode
static const char * lens_op_mode_rel = "rel";
static const char * lens_op_mode_abs = "abs";
static const char * lens_op_mode_step = "step";
static const char * lens_op_mode_seq = "seq";






static const char * set_call = "set"; 
static const char * get_call = "get"; 

void console_input(char * rx_buffer){

    console_cmd_t cmd;
    console_tokenize(rx_buffer, &cmd);

    if (cmd.argc == 0){
        ESP_LOGW(TAG, "Print <help> to get commands list");
        return;
    }
    

    if(!strcasecmp(cmd.argv[0], sys_call)) handle_system(&cmd);
    else if(!strcasecmp(cmd.argv[0], calib_call)) handle_calib(&cmd);
    else if(!strcasecmp(cmd.argv[0], lens_call)) handle_lens(&cmd);
    else if(!strcasecmp(cmd.argv[0], set_call)) handle_set(&cmd);
    else if(!strcasecmp(cmd.argv[0], get_call)) handle_get(&cmd);
    
    else{
        ESP_LOGW(TAG, "Unknown command.\nPrint <help> to get commands list");
    }
}

static void console_tokenize(char * line, console_cmd_t * cmd){
    cmd->argc = 0;
    char * token = strtok(line, " \t\r\n");
    while(token && cmd ->argc < MAX_TOKENS){
        cmd->argv[cmd->argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }
}

static bool lens_from_string(const char * line, lens_target_t * out){
    for(int i = 0; i < ARRAY_SIZE(lens_table); i++){
        if(!strcasecmp(line, lens_table[i].name)){
            *out = lens_table[i].lens_target;
            //ESP_LOGI(TAG, "%d", *out);
            return true;
        }
    }
    return false;
}


void handle_system(console_cmd_t * cmd){
    if(cmd->argc < 2){
        ESP_LOGW(TAG, "Available commands are: start/stop/save/default");
        return;
    }

    if(!strcasecmp(cmd->argv[1], sys_start)){
        sys_cmd.sys_event = EVENT_START;
        ESP_LOGI(TAG, "Event is: %d", sys_event);
        //system_start();
    } 
    else if(!strcasecmp(cmd->argv[1], sys_stop)){
        sys_cmd.sys_event = EVENT_STOP;
        ESP_LOGI(TAG, "Event is: %d", sys_event);
        //system_stop();
    } 
    else if(!strcasecmp(cmd->argv[1], sys_save)){
        sys_cmd.sys_event = EVENT_SAVE;
        //system_save();
    } 
    else if(!strcasecmp(cmd->argv[1], sys_default)){
        sys_cmd.sys_event = EVENT_NONE;
        //system_defaults();
    } 
    else ESP_LOGW(TAG, "Unknown command.\nAvailable commands are: start/stop");
}

void handle_calib(console_cmd_t * cmd){
    if(cmd->argc < 2){
        ESP_LOGW(TAG, "Available commands are: start/stop");
        return;
    }

    if(!strcasecmp(cmd->argv[1], calib_begin_str)){
        sys_cmd.sys_event = EVENT_CALIB_BEGIN;
    } 
    else if(!strcasecmp(cmd->argv[1], calib_exit_str)){
        sys_cmd.sys_event = EVENT_CALIB_EXIT;
    } 
    else{
        ESP_LOGW(TAG, "Unknown command.\nAvailable commands are: start/stop");
    }
}

static void handle_set(console_cmd_t * cmd){
    if(cmd->argc < 3){
        ESP_LOGW(TAG, "ERR usage: set <target> <value>");
        return;
    }
    ESP_LOGW(TAG, "SET");
}

static void handle_get(console_cmd_t * cmd){
   if(cmd->argc < 3){
        ESP_LOGW(TAG, "ERR usage: set <target> <value>");
        return;
    }
    ESP_LOGW(TAG, "GET");
}

static void handle_lens(console_cmd_t * cmd){
    //Enter Lens event
    sys_cmd.sys_event = EVENT_LENS;
    if(cmd->argc < 2){
        ESP_LOGW(TAG, "ERR usage: lens <target> <action> <coupling> <mode> <value> <period>(only in \"step\" and \"seq\" modes");
        return;
    }
    //Decide lens target
    //lens_target_t lens_target;
    if(!lens_from_string(cmd->argv[1], &sys_cmd.lens_target)){
        ESP_LOGW(TAG, "Unknown lens %s", cmd->argv[1]);
        return;
    }
    /* 
    switch (sys_cmd.lens_target) {
        case LENS_FIRST:  ESP_LOGI(TAG, "Lens parsed: First");  break;
        case LENS_SECOND:  ESP_LOGI(TAG, "Lens parsed: Second"); break;
        case LENS_BOTH: ESP_LOGI(TAG, "Lens parsed: Both");   break;
        default: break;
    } */
    //Lens action
    if(cmd->argc < 3){
        ESP_LOGW(TAG, "Missing <action>");
        return;
    }

    if(!strcasecmp(cmd->argv[2], lens_action_move)){
        if(cmd->argc < 4){
            ESP_LOGW(TAG, "You have to input coupling mode next");
            return;
        }
    }
    else if(!strcasecmp(cmd->argv[2], lens_action_rezero)){
        ESP_LOGI(TAG, "Returning lenses to zero position!");
        sys_cmd.lens_mode = LENS_MODE_REZERO;
        return;
    }
    else{
        if(sys_cmd.lens_target == LENS_TARGET_FIRST || sys_cmd.lens_target == LENS_TARGET_SECOND){
            ESP_LOGE(TAG, "You have to input lens mode next");
        }
        else if(sys_cmd.lens_target == LENS_TARGET_BOTH){
            ESP_LOGE(TAG, "You have to input lens coupling next");
        }
    }

    //Coupling mode
    int argv_index = 3; //Substituting hardcoded index in cmd->argv[<index>] in order to remove the need to input symmetry mode in single lens target
    if(sys_cmd.lens_target == LENS_TARGET_BOTH){

        if(!strcasecmp(cmd->argv[argv_index], lens_coupling_call_asymmetric)){
            ESP_LOGE(TAG, "coupling - asymmetric");
            sys_cmd.lens_couple = COUPLE_ASYMMETRIC;
        }
        else if(!strcasecmp(cmd->argv[argv_index], lens_coupling_call_symmetric)){
            ESP_LOGE(TAG, "coupling - symmetric");
            sys_cmd.lens_couple = COUPLE_SYMMETRIC;
        }
        else{
            ESP_LOGE(TAG, "Unknown command on coupling section");
            return;
        }
        if(cmd->argc < argv_index + 2){ //dunno why +2 here and +1 everywhere else (actually know)
            ESP_LOGW(TAG, "You have to input lens mode next");
            return;
        }
        argv_index++; 
    }
    
    
    //Lens operating mode
    if(!strcasecmp(cmd->argv[argv_index], lens_op_mode_rel)){
        sys_cmd.lens_mode = LENS_MODE_MOVE_REL;
    }
    else if(!strcasecmp(cmd->argv[argv_index], lens_op_mode_abs)){
        sys_cmd.lens_mode = LENS_MODE_MOVE_ABS;
    }
    else if(!strcasecmp(cmd->argv[argv_index], lens_op_mode_step)){
        sys_cmd.lens_mode = LENS_MODE_MOVE_STEP;
    }
    else if(!strcasecmp(cmd->argv[argv_index], lens_op_mode_seq)){
        sys_cmd.lens_mode = LENS_MODE_MOVE_SEQUENCE;
    }
    else if(!strcasecmp(cmd->argv[argv_index], lens_op_mode_step)){
        sys_cmd.lens_mode = LENS_MODE_MOVE_STEP;
    }
    else{
        ESP_LOGE(TAG, "Unknown command on mode section");
        return;
    }
    argv_index++;



    //"Eject" angle/angles to sequential mode
    if(cmd->argc < argv_index + 1){
        ESP_LOGW(TAG, "You have to input value in degrees next");
        return;
    }

    if(sys_cmd.lens_mode == LENS_MODE_MOVE_SEQUENCE){
        //float value_array[10];
        std::vector<float> seq_value_vector;
        seq_value_vector.reserve(100);
        int vector_index = 0;
        for(int i = argv_index; i < cmd->argc-1; i++){
            //value_array[i] = strtof(cmd->argv[i], nullptr);
            //ESP_LOGI(TAG, "Value at index %i: %f\n",i ,value_array[i]);
            seq_value_vector.push_back(strtof(cmd->argv[i], nullptr));
            ESP_LOGW(TAG, "Value at index %i: %f\n",i ,seq_value_vector[vector_index]);
            argv_index++;
            vector_index++;
        }
        seq_value_vector.shrink_to_fit();
        sys_cmd.value_seq_mode_vector = seq_value_vector;
        seq_value_vector.clear();
        seq_value_vector.shrink_to_fit();
        argv_index--;
    }

    //Ejecting angle 
    float value_deg = strtof(cmd->argv[argv_index], nullptr);
    switch (sys_cmd.lens_target)
    {
    case LENS_TARGET_FIRST:
        lens_fst_deg_val = value_deg;
        sys_cmd.value = value_deg;
        ESP_LOGI(TAG, "First lens: %f", value_deg);
        break;
    case LENS_TARGET_SECOND:
        lens_scd_deg_val = value_deg;
        sys_cmd.value = value_deg;
        ESP_LOGI(TAG, "Second lens: %f", value_deg);
        break;
    case LENS_TARGET_BOTH:
        lens_both_deg_val = value_deg;
        sys_cmd.value = value_deg;
        ESP_LOGI(TAG, "Both lenses: %f", value_deg);
        break;
    default: break;
    }
    argv_index++;

    //Eject "period" from step and seq lens modes
    if(sys_cmd.lens_mode == LENS_MODE_MOVE_SEQUENCE || sys_cmd.lens_mode == LENS_MODE_MOVE_STEP){
        if(cmd->argc < argv_index + 1){
            ESP_LOGW(TAG, "You have to input value in milliseconds or seconds next");
            return;
        }
        int len = strlen(cmd->argv[argv_index]);
        
        if(len > 0 && cmd->argv[argv_index][len-1] == 's'){ //decide whether input is in seconds
            //find commas and translate them to dots 3,5s -> 3.5s
            if(strchr(cmd->argv[argv_index], ',')){
                for(int i = 0; cmd->argv[argv_index][i]; i++){
                    if(cmd->argv[argv_index][i] == ',') cmd->argv[argv_index][i] = '.';
                }
            }
            float period_val_float = strtof(cmd->argv[argv_index], nullptr);
            ESP_LOGI(TAG, "Period float is: %f \n", period_val_float);
            sys_cmd.period = static_cast<int32_t>(period_val_float * 1000.0f);
            ESP_LOGI(TAG, "Period casted to int32_t is: %" PRId32 "\n", sys_cmd.period);
        }
        else if(len > 0){ //decide whether input is in millis
            int32_t period_val_int32_t = strtol(cmd->argv[argv_index], nullptr, 10);
            printf("Period int32_t: %" PRId32 "\n", period_val_int32_t);
            sys_cmd.period = period_val_int32_t;
        }
        else{
            ESP_LOGE(TAG, "Input formats:\n\t<float>+'s' - for seconds. Ex: 3.5s or 3,5s  \n\t<int> - for milliseconds. Ex: 6785");
            return;
        }
    }
        
}

float console_get_lens_fst_move_angle(){
    return lens_fst_deg_val;
} 
float console_get_lens_scd_move_angle(){
    return lens_scd_deg_val;
}
float console_get_lens_both_move_angle(){
    return lens_both_deg_val;
}

int32_t console_get_lens_period(){
    return lens_period;
}

void console_set_lens_fst_zero_angle(){
    lens_fst_deg_val = 0.0f;
}
void console_set_lens_scd_zero_angle(){
    lens_scd_deg_val = 0.0f;
}
void console_set_lens_both_zero_angle(){
     lens_both_deg_val = 0.0f;
}