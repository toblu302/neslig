#include <bit>
#include <iostream>

#include "channels.h"

void Pulse::clock_timer() {
    timer -= 1;
    if(timer == 0) {
        sequence = std::rotr(sequence, 1);
        timer = timer_reset;
        output = (sequence&1);
    }
}

void Pulse::clock_envelope() {
    envelope.clock();
}

void Pulse::clock_length_counter() {
    length_counter.clock();
}

void Pulse::clock_sweep() {
    if(sweep_divider_counter==0 && sweep_enabled && !IsSweepMuting()) {
        sweep();
    }
    else if(sweep_divider_counter==0 || sweep_reload) {
        sweep_divider_counter=sweep_divider_period+1;
        sweep_reload=false;
    }
    else {
        --sweep_divider_counter;
    }
}

void Pulse::sweep() {
    uint16_t delta = timer_reset >> sweep_shift_count;
    if(sweep_negated) {
        if(is_channel_1) {
            timer_reset -= delta+1;
        } else {
            timer_reset -= delta;
        }
    }
    else {
        timer_reset += delta;
    }
}

bool Pulse::IsSweepMuting() {
    return timer<8 || timer_reset > 0x7FF;
}

uint8_t Pulse::sample() {
    if(!output || !enabled || IsSweepMuting() || length_counter.value==0 ) {
        return 0;
    }

    if(is_constant) {
        return constant_volume;
    } else {
        return envelope.decay_level;
    }
}

void Envelope::clock() {
    if(start_flag) {
        start_flag = false; // the start flag is cleared
        decay_level=15; // the decay level counter is loaded with 15
        divider_step=reset_level; // the divider's period is immediately reloaded
    }
    else {
        // divider is clocked
        if(divider_step>0) {
            --divider_step;
        }
        else {
            divider_step=reset_level; // it is loaded with V

            // decay level counter is clocked
            if(decay_level > 0) {
                --decay_level; // if the counter is non-zero, it is decremented
            }
            else if(is_looping) {
                decay_level=15;
            }
        }
    }
}

void LengthCounter::clock() {
    if(!is_halted && value>0) {
        --value;
    }
}