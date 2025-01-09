#pragma once

#include "zpl_wrap.h"
#include "sokol_app.h"

enum Input_Controller_Button
{
    INPUT_CONTROLLER_BUTTON_A = (1 << 0),
    INPUT_CONTROLLER_BUTTON_B = (1 << 1),
    INPUT_CONTROLLER_BUTTON_LEFT = (1 << 2),
    INPUT_CONTROLLER_BUTTON_RIGHT = (1 << 3),
    INPUT_CONTROLLER_BUTTON_UP = (1 << 4),
    INPUT_CONTROLLER_BUTTON_DOWN = (1 << 5),
};

enum Input_Controller_Index
{
    INPUT_CONTROLLER_INDEX_0,
    INPUT_CONTROLLER_INDEX_1,
    INPUT_CONTROLLER_COUNT,
};

struct Input_Controller
{
    b8 enabled;
    u16 current_state;
    u16 last_state;
};

struct Input
{
    Input_Controller controllers[INPUT_CONTROLLER_COUNT];
    isize controllers_count;
};

void input_init(Input *inp);
void input_handle_event(Input *inp, const sapp_event *ev);
void input_update(Input *inp);

b8 input_controller_button_down(Input_Controller *c,
                                Input_Controller_Button button);
b8 input_controller_button_up(Input_Controller *c,
                              Input_Controller_Button button);
b8 input_controller_button_pressed(Input_Controller *c,
                                   Input_Controller_Button button);
b8 input_controller_button_released(Input_Controller *c,
                                    Input_Controller_Button button);
