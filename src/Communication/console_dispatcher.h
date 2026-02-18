#pragma once
#include <vector>

#define BUF_SIZE 1024
#define MAX_TOKENS 32


typedef enum{
    EVENT_STOP,
    EVENT_START,
    EVENT_CALIB_BEGIN,
    EVENT_CALIB_EXIT,
    EVENT_LENS,
    EVENT_SAVE,
    EVENT_SET,
    EVENT_NONE,
    EVENT_RESET
} sys_event_t;

//Message structure - max commands available = MAX_TOKENS
typedef struct{
    int argc;
    char *argv[MAX_TOKENS];
} console_cmd_t;


typedef enum{
    LENS_MODE_MOVE_REL,
    LENS_MODE_MOVE_ABS,
    LENS_MODE_MOVE_STEP,
    LENS_MODE_MOVE_SEQUENCE,
    LENS_MODE_REZERO, //return to the original position
    LENS_MODE_NONE
    //LENS_MODE_RESET //update the state of the system as the motor stopped moving
} lens_mode_t;

typedef enum{
    COUPLE_SYMMETRIC,
    COUPLE_ASYMMETRIC,
    COUPLE_NONE
} lens_coupling_t; 

typedef enum {
    LENS_TARGET_FIRST,
    LENS_TARGET_SECOND,
    LENS_TARGET_BOTH,
    LENS_TARGET_NONE
} lens_target_t;



/// Lens target table
typedef struct{
    const char * name;
    lens_target_t lens_target;
} lens_entry_t;

static const lens_entry_t lens_table[] = {
    {"fst", LENS_TARGET_FIRST},
    {"scd", LENS_TARGET_SECOND},
    {"both", LENS_TARGET_BOTH}
};
/// Lens target table end

/// Lens mode table
typedef struct{
    const char * name;
    lens_mode_t lens_mode;
} lens_mode_entry_t;

static const lens_mode_entry_t lens_mode_table[] = {
    {"rel", LENS_MODE_MOVE_REL},
    {"abs", LENS_MODE_MOVE_ABS},
    {"step", LENS_MODE_MOVE_STEP},
    {"seq", LENS_MODE_MOVE_SEQUENCE}
};
/// Lens mode table end

typedef struct{
    sys_event_t sys_event;
    lens_target_t lens_target;
    lens_mode_t lens_mode;
    lens_coupling_t lens_couple;
    float value;
    std::vector<float> value_seq_mode_vector;
    int32_t period;
} sys_command_t;

void console_input(char * rx_buffer);

static void console_tokenize(char * rx_buffer, console_cmd_t * cmd);
static bool lens_from_string(const char * line, lens_target_t * out);


static void handle_system(console_cmd_t * cmd);
   
static void handle_calib(console_cmd_t * cmd);

static void handle_lens(console_cmd_t * cmd);
    
static void handle_set(console_cmd_t * cmd);

static void handle_get(console_cmd_t * cmd);

float console_get_lens_fst_move_angle();
float console_get_lens_scd_move_angle();
float console_get_lens_both_move_angle();
int32_t console_get_lens_period();

void console_set_lens_fst_zero_angle();
void console_set_lens_scd_zero_angle();
void console_set_lens_both_zero_angle();