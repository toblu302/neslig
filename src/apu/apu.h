#ifndef APU_H_INCLUDED
#define APU_H_INCLUDED

#include <stdint.h>
#include <bit>
#include <iostream>
#include <SDL2/SDL_audio.h>
#include <queue>
#include <atomic>

#include "channels.h"

void audio_callback(void *, Uint8*, int);

class Apu {
    public:
        Apu() {
            SDL_AudioSpec desiredSpec;

            desiredSpec.freq = sample_frequency;
            desiredSpec.format = AUDIO_F32SYS;
            desiredSpec.channels = 1;
            desiredSpec.samples = samples_per_callback;
            desiredSpec.callback = audio_callback;
            desiredSpec.userdata = this;

            deviceId = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);

            for(uint8_t i=0; i<32; ++i) {
                pulse_table[i] = 95.52/(8128.0/((float)i) + 100.0);
            }

            SDL_PauseAudioDevice(deviceId, 0);
        }

        ~Apu() {
            SDL_CloseAudioDevice(deviceId);
        }

        void clock();
        void writeRegister(const uint16_t &address, const uint8_t &value);

        std::queue<float> output_buffer;

        SDL_AudioStream *stream;

        std::atomic<uint32_t> generated_samples = 0;
        const Uint16 samples_per_callback = 2048;


    private:
        Pulse pulse1 = Pulse(true);
        Pulse pulse2 = Pulse(false);

        uint16_t clock_counter = 0;

        uint32_t sample_timer = 0;
        const uint32_t cpu_frequency = 1789773;
        const int sample_frequency = 44100;

        uint32_t frame_timer = 0;
        bool five_step_sequence = 0;
        uint32_t frame_step = 0;

        float pulse_table[32];
        float GetSample();

        SDL_AudioDeviceID deviceId;
};

#endif // APU_H_INCLUDED