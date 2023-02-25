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

    if(clock_counter % 2 == 0)
    {
        pulse1.clock();
        pulse2.clock();
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
}

void Apu::writeRegister(const uint16_t &address, const uint8_t &value) {
    switch(address) {
        case 0x4000:
            switch((value&0xC0) >> 6) {
                case 0x00: pulse1.sequence = 0b01000000; break;
                case 0x01: pulse1.sequence = 0b01100000; break;
                case 0x02: pulse1.sequence = 0b01111000; break;
                case 0x03: pulse1.sequence = 0b10011111; break;
            }
            pulse1.envelope = (value&0x0F)<<1;
            break;

        case 0x4002:
            pulse1.timer_reset = (pulse1.timer_reset & 0xFF00) | value;
            break;

        case 0x4003:
            pulse1.timer_reset = (pulse1.timer_reset & 0x00FF) | ((value&0x7) << 8);
            pulse1.timer = pulse1.timer_reset;
            break;

        case 0x4004:
            switch((value&0xC0) >> 6) {
                case 0x00: pulse2.sequence = 0b01000000; break;
                case 0x01: pulse2.sequence = 0b01100000; break;
                case 0x02: pulse2.sequence = 0b01111000; break;
                case 0x03: pulse2.sequence = 0b10011111; break;
            }
            pulse2.envelope = (value&0x0F)<<1;
            break;

        case 0x4006:
            pulse2.timer_reset = (pulse2.timer_reset & 0xFF00) | value;
            break;

        case 0x4007:
            pulse2.timer_reset = (pulse2.timer_reset & 0x00FF) | ((value&0x7) << 8);
            pulse2.timer = pulse2.timer_reset;
            break;
    }
}