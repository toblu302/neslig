#include "apu.h"

#include <iostream>
#include <bit>
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
        timer -= 1;
        if(timer == 0) {
            pulse_sequence = std::rotr(pulse_sequence, 1);
            timer = timer_reset;
            pulse_output = pulse_sequence&1;
        }
    }


    sample_timer += sample_frequency;
    if(sample_timer >= cpu_frequency) {
        sample_timer -= cpu_frequency;

        generated_samples += 1;

        SDL_LockAudioDevice(deviceId);
        if(pulse_output) {
            output_buffer.push(pulse_table[pulse_envelope & 0x1F]);
        } else {
            output_buffer.push(0);
        }
        SDL_UnlockAudioDevice(deviceId);
    }

}

void Apu::writeRegister(const uint16_t &address, const uint8_t &value) {
    switch(address) {
        case 0x4004:
            switch((value&0xC0) >> 6) {
                case 0x00: pulse_sequence = 0b01000000; break;
                case 0x01: pulse_sequence = 0b01100000; break;
                case 0x02: pulse_sequence = 0b01111000; break;
                case 0x03: pulse_sequence = 0b10011111; break;
            }
            pulse_envelope = (value&0x0F)<<1;
            break;

        case 0x4006:
            timer_reset = (timer_reset & 0xFF00) | value;
            break;

        case 0x4007:
            timer_reset = (timer_reset & 0x00FF) | ((value&0x7) << 8);
            timer = timer_reset;
            break;
    }
}