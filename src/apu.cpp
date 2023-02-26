#include "apu.h"

#include <iostream>
#include <SDL2/SDL.h>

void audio_callback(void *_apu, Uint8 *_stream, int _length) {
    Apu *apu = (Apu*) _apu;
    float *stream = (float*)_stream;
    size_t length = (size_t)_length/sizeof(float);

    if(apu->generated_samples >= length) {
        apu->generated_samples -= length;
    } else {
        apu->generated_samples = 0;
    }

    size_t counter = 0;
    size_t end = std::min(apu->output_buffer.size(), (size_t)length);

    float next = 0;
    while(counter < end) {
        next = apu->output_buffer.front();
        stream[counter++] = next;
        apu->output_buffer.pop();
    }
    while(counter < length) {
        stream[counter++] = 0;
    }
}

void Apu::clock() {

    clock_counter += 1;

    // clock timers
    if(clock_counter % 2 == 0)
    {
        pulse1.clock_timer();
        pulse2.clock_timer();
    }

    // frame steps are clocked at ~240 hz in 5-step sequence mode
    // and ~192 Hz in 4-step sequence mode
    uint32_t frame_clock = 240;
    if(!five_step_sequence) frame_clock = 192;

    frame_timer += frame_clock;
    if(frame_timer >= cpu_frequency) {
        frame_timer -= cpu_frequency;

        if(five_step_sequence) {
            switch(frame_step%5) {
                case 0:
                    pulse1.clock_envelope();
                    pulse2.clock_envelope();
                    break;
                case 1:
                    pulse1.clock_envelope();
                    pulse2.clock_envelope();
                    pulse1.clock_length_counter();
                    pulse2.clock_length_counter();
                    break;
                case 2:
                    pulse1.clock_envelope();
                    pulse2.clock_envelope();
                    break;
                case 3:
                    break;
                case 4:
                    pulse1.clock_envelope();
                    pulse2.clock_envelope();
                    pulse1.clock_length_counter();
                    pulse2.clock_length_counter();
                    break;
            }
        }


        ++frame_step;
    }


    sample_timer += sample_frequency;
    if(sample_timer >= cpu_frequency) {
        sample_timer -= cpu_frequency;

        generated_samples += 1;

        SDL_LockAudioDevice(deviceId);
        output_buffer.push( GetSample() );
        SDL_UnlockAudioDevice(deviceId);
    }

}

float Apu::GetSample() {
    uint8_t pulse1_sample = pulse1.sample();
    uint8_t pulse2_sample = pulse2.sample();

    return pulse_table[(pulse1_sample+pulse2_sample) & 0x1F];
    //return pulse_table[pulse1_sample & 0x1F];
}

void Apu::writeRegister(const uint16_t &address, const uint8_t &value) {
    for(uint16_t base: {0x4000, 0x4004}) {
        Pulse *pulse = &pulse1;
        if(base == 0x4004) pulse = &pulse2;
        switch(address-base) {
            case 0:
                switch((value&0xC0) >> 6) {
                    case 0x00: pulse->sequence = 0b01000000; break;
                    case 0x01: pulse->sequence = 0b01100000; break;
                    case 0x02: pulse->sequence = 0b01111000; break;
                    case 0x03: pulse->sequence = 0b10011111; break;
                }
                pulse->envelope = value&0x0F;
                pulse->is_constant = value&(1<<4);
                pulse->is_infinite = value&(1<<5);
                pulse->start_flag = true;
                break;

            case 1:
                break;

            case 2:
                pulse->timer_reset = (pulse->timer_reset & 0xFF00) | value;
                break;

            case 3:
                pulse->timer_reset = (pulse->timer_reset & 0x00FF) | ((value&0x7) << 8);
                pulse->timer = pulse->timer_reset;
                pulse->length_counter = length_counter_table[(value&0xF8)>>3];
                pulse->start_flag = true;
                break;
        }
    }

    switch(address) {
        case 0x4015:
            pulse1.enabled = value&1;
            pulse2.enabled = value&(1<<1);
            break;

        case 0x4017:
            five_step_sequence = value&(1<<7);
            break;
    }
}