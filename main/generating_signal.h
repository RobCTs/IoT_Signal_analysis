#ifndef GENERATING_SIGNAL_H
#define GENERATING_SIGNAL_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t signalQueue; 

// Initialize and run the signal generator
void setup_signal_queue();
// static void signal_timer_callback(void* arg)
void generate_random_signal();
float get_signal_at(float t);

#endif // GENERATING_SIGNAL_H
