#ifndef CHANNELS_H_INCLUDED
#define CHANNELS_H_INCLUDED

#include <stdint.h>

class Envelope {
    public:
        void clock();

        bool start_flag=false;

        uint8_t decay_level=0;
        uint8_t divider_step=0;
        uint8_t reset_level=0;
        bool is_looping=false;
};

class LengthCounter {
    public:
        void clock();
        void SetValue(const uint8_t &value) {
            this->value = length_counter_table[value];
        }
        uint8_t value=0;
        bool is_halted=true;

    private:
        uint8_t length_counter_table[0x20] = {10, 254, 20, 2, 40, 4, 80, 6,
                                              160, 8, 60, 10, 14, 12, 26, 14,
                                              12, 16, 24, 18, 48, 20, 96, 22,
                                              192, 24, 72, 26, 16, 28, 32, 30};
};

class Pulse {
    public:
        Pulse(bool is_channel_1){this->is_channel_1=is_channel_1;}
        void clock_timer();
        void clock_envelope();
        void clock_length_counter();
        void clock_sweep();
        uint8_t sample();

        Envelope envelope;
        LengthCounter length_counter;

        bool enabled=false;
        bool output=false;
        bool is_constant=true;
        uint8_t constant_volume=0;
        uint8_t sequence=0;
        uint16_t timer_reset=0;
        uint16_t timer=0;

        bool sweep_enabled=false;
        uint8_t sweep_divider_period=0;
        uint8_t sweep_divider_counter=0;
        bool sweep_negated=false;
        uint8_t sweep_shift_count=0;
        bool sweep_reload=false;
        void sweep();
        bool IsSweepMuting();

    private:
        bool is_channel_1;
};


#endif // CHANNELS_H_INCLUDED