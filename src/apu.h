#ifndef APU_H_INCLUDED
#define APU_H_INCLUDED

#include <stdint.h>
#include <bit>
#include <iostream>
#include <SDL2/SDL_audio.h>
#include <queue>
#include <atomic>

void audio_callback(void *, Uint8*, int);

class Pulse {
    public:
        void clock_timer() {
            timer -= 1;
            if(timer == 0) {
                sequence = std::rotr(sequence, 1);
                timer = timer_reset;
                output = ((sequence&1) == 1);
            }
        }

        void clock_envelope() {
            if(start_flag) {
                start_flag = false; // the start flag is cleared
                decay_level=15; // the decay level counter is loaded with 15
                divider_step=envelope; // the divider's period is immediately reloaded
            }
            else {
                // divider is clocked
                if(divider_step>0) {
                    --divider_step;
                }
                else {
                    divider_step=envelope; // it is loaded with V

                    // decay level counter is clocked
                    if(decay_level > 0) {
                        --decay_level; // if the counter is non-zero, it is decremented
                    }
                    else if(is_infinite) {
                        decay_level=15;
                    }
                }
            }
        }

        void clock_length_counter() {
            if(!is_infinite && length_counter>0) {
                --length_counter;
            }
        }

        uint8_t sample() {
            if(!output || !enabled || timer<8 || timer_reset > 0x7ff || length_counter==0 ) {
                    return 0;
            }

            if(is_constant) {
                return envelope;
            } else {
                return decay_level;
            }
        }

        bool enabled=false;
        bool output=false;
        bool is_constant=true;
        bool is_infinite=false;
        uint8_t envelope=0;
        uint8_t sequence=0;
        uint16_t timer_reset=0;
        uint16_t timer=0;
        uint8_t length_counter=0;

        bool start_flag=false;
        uint8_t decay_level=0;
        uint8_t divider_step=0;
};

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

            for(uint8_t i=0; i<=32; ++i) {
                pulse_table[i] = 95.52/(8128.0/i + 100);
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
        Pulse pulse1;
        Pulse pulse2;

        uint16_t clock_counter = 0;

        uint32_t sample_timer = 0;
        const uint32_t cpu_frequency = 1789773;
        const int sample_frequency = 44100;

        uint32_t frame_timer = 0;
        bool five_step_sequence = 0;
        uint32_t frame_step = 0;

        uint8_t length_counter_table[0x20] = {10, 254, 20, 2, 40, 4, 80, 6,
                                              160, 8, 60, 10, 14, 12, 26, 14,
                                              12, 16, 24, 18, 48, 20, 96, 22,
                                              192, 24, 72, 26, 16, 28, 32, 30};

        float pulse_table[32];
        float GetSample();


        SDL_AudioDeviceID deviceId;
};

#endif // APU_H_INCLUDED