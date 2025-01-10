#pragma once

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

struct Input_Controller
{
    bool enabled;
    uint16_t current_state;
    uint16_t last_state;
};

struct Input
{
    Input_Controller controllers[2];
    int controllers_count;
};

void input_init(Input &inp);
void input_handle_event(Input &inp, const sapp_event *ev);
void input_update(Input &inp);

bool input_controller_button_down(const Input_Controller &c,
                                  Input_Controller_Button button);
bool input_controller_button_up(const Input_Controller &c,
                                Input_Controller_Button button);
bool input_controller_button_pressed(const Input_Controller &c,
                                     Input_Controller_Button button);
bool input_controller_button_released(const Input_Controller &c,
                                      Input_Controller_Button button);
