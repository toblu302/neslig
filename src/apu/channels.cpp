#include <iostream>

#include "channels.h"

void Pulse::ClockTimer() {
    timer -= 1;
    if(timer == 0) {
        timer = timer_reset;
        sequence_index = (sequence_index+1)%8;
    }
}

void Pulse::ClockEnvelope() {
    envelope.clock();
}

void Pulse::ClockLengthCounter() {
    length_counter.clock();
}

void Pulse::ClockSweep() {
    if(sweep_reload) {
        sweep_divider_counter=sweep_divider_period+1;
        sweep_reload=false;
    }
    else if(sweep_divider_counter>0) {
        --sweep_divider_counter;
    }
    else {
        sweep_divider_counter=sweep_divider_period+1;
        if(sweep_enabled) {
            Sweep();
        }
    }
}

void Pulse::Sweep() {
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

uint8_t Pulse::GetSample() {
    if(!enabled || !(sequence[sequence_index]) || IsSweepMuting() || length_counter.value==0 ) {
        return 0;
    }

    if(is_constant) {
        return constant_volume;
    } else {
        return envelope.decay_level;
    }
}

void Triangle::ClockTimer() {
    timer -= 1;
    if(timer == 0) {
        position = (position+1)%32;
        timer = timer_reset;
    }
}

void Triangle::ClockLengthCounter() {
    length_counter.clock();
}

void Triangle::ClockLinearCounter() {
    if(counter_reload) {
        counter_value=counter_reload_value;
    }
    else if(counter_value>0) {
        --counter_value;
    }

    if(counter_on) {
        counter_reload=false;
    }
}

uint8_t Triangle::GetSample() {
    if(!enabled || counter_value==0 || length_counter.value==0) {
        return 0;
    }

    return sequence[position];
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