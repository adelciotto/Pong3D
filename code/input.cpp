#include "input.h"

void input_init(Input *inp)
{
    inp->controllers_count = 1;
    inp->controllers[INPUT_CONTROLLER_INDEX_0].enabled = true;
}

static void input_controller_set_button(Input_Controller *c,
                                        Input_Controller_Button button, b8 down)
{
    if (down)
    {
        c->current_state |= button;
    }
    else
    {
        c->current_state &= ~button;
    }
}

void input_handle_event(Input *inp, const sapp_event *ev)
{
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN)
    {
        if (ev->key_code == SAPP_KEYCODE_UP)
        {
            input_controller_set_button(
                &inp->controllers[INPUT_CONTROLLER_INDEX_0],
                INPUT_CONTROLLER_BUTTON_UP, true);
        }
        if (ev->key_code == SAPP_KEYCODE_DOWN)
        {
            input_controller_set_button(
                &inp->controllers[INPUT_CONTROLLER_INDEX_0],
                INPUT_CONTROLLER_BUTTON_DOWN, true);
        }
    }
    if (ev->type == SAPP_EVENTTYPE_KEY_UP)
    {
        if (ev->key_code == SAPP_KEYCODE_UP)
        {
            input_controller_set_button(
                &inp->controllers[INPUT_CONTROLLER_INDEX_0],
                INPUT_CONTROLLER_BUTTON_UP, false);
        }
        if (ev->key_code == SAPP_KEYCODE_DOWN)
        {
            input_controller_set_button(
                &inp->controllers[INPUT_CONTROLLER_INDEX_0],
                INPUT_CONTROLLER_BUTTON_DOWN, false);
        }
    }
}

void input_update(Input *inp)
{
    for (isize i = 0; i < INPUT_CONTROLLER_COUNT; i += 1)
    {
        Input_Controller *c = &inp->controllers[i];
        if (c->enabled)
        {
            c->last_state = c->current_state;
        }
    }
}

b8 input_controller_button_down(Input_Controller *c,
                                Input_Controller_Button button)
{
    return (c->current_state & button) != 0;
}

b8 input_controller_button_up(Input_Controller *c,
                              Input_Controller_Button button)
{
    return (c->current_state & button) == 0;
}

b8 input_controller_button_pressed(Input_Controller *c,
                                   Input_Controller_Button button)
{
    return (c->current_state & button) != 0 && (c->last_state & button) == 0;
}

b8 input_controller_button_released(Input_Controller *c,
                                    Input_Controller_Button button)
{
    return (c->current_state & button) == 0 && (c->last_state & button) != 0;
}
