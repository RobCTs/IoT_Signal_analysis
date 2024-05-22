#ifndef SAMPLING_AND_ANALYZING_H
#define SAMPLING_AND_ANALYZING_H

#include <stdbool.h>

int oversample();
int sample_and_analyze_signal();
void analyze_fft(const float* signal, float* frequency_magnitude);
float calculate_average_signal(float start_time, float window_length, int otpimal_rate);
float sample_signal_and_compute_average(float window_length);

#endif // SAMPLING_AND_ANALYZING_H
