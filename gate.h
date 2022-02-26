#pragma once
#include <CircularBuffer.h>
#include "pulse.h"

class gate
{
  public:
    gate();

    void setup();
    void self_test();
    bool is_triggered();
    void dial();

    // TODO: move drawing routines to private. caller should only dial
    void draw_event_horizon();
    void draw_collapsing_gate();
    void draw_gate_open();
    void draw_gate_closed();

  private:
    static void isr();

    void draw_current_dialing_status();

    uint8_t get_dial_position_led();
    uint8_t dial_position;
    void move_dial_position();
    void change_dial_spin_direction();
    bool dial_is_on_chevron();

    void lock_chevron();

    enum class direction { CLOCKWISE, COUNTERCLOCKWISE };
    direction spin_direction;
    uint8_t next_chevron_to_lock;

    // Locations of the chevrons
    uint8_t get_led_chevron(uint8_t n);
    static const uint8_t led_chevron_position[];
    static const uint8_t chevron_sequence[];

    CircularBuffer<pulse_sol21, 3> pulses;
    volatile static unsigned long pulseRisingEdge;
    static bool got_pulseRisingEdge;
    volatile static unsigned long pulseFallingEdge;
    volatile static bool newPulseAvailable;
};
