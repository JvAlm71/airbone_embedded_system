#ifndef AIRBORNE_SYSTEM_H
#define AIRBORNE_SYSTEM_H

// Define before includes to get POSIX functions
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

// Sistema de monitoramento de aeronave embarcado
#define MAX_SENSORS 10
#define BUFFER_SIZE 100
#define SAMPLING_RATE_MS 1000

// Estruturas de dados dos sensores
typedef struct {
    float temperature;    // Celsius
    float pressure;      // hPa
    float altitude;      // metros
    time_t timestamp;
} sensor_data_t;

typedef struct {
    sensor_data_t data[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} data_buffer_t;

typedef struct {
    bool running;
    pthread_t sensor_thread;
    pthread_t processing_thread;
    pthread_t monitoring_thread;
    data_buffer_t buffer;
    pthread_mutex_t system_mutex;
} airborne_system_t;

// Protótipos das funções
int airborne_system_init(airborne_system_t *system);
int airborne_system_start(airborne_system_t *system);
void airborne_system_stop(airborne_system_t *system);
void airborne_system_cleanup(airborne_system_t *system);

void* sensor_thread_func(void *arg);
void* processing_thread_func(void *arg);
void* monitoring_thread_func(void *arg);

int buffer_init(data_buffer_t *buffer);
int buffer_put(data_buffer_t *buffer, sensor_data_t *data);
int buffer_get(data_buffer_t *buffer, sensor_data_t *data);
void buffer_cleanup(data_buffer_t *buffer);

void signal_handler(int sig);

#endif // AIRBORNE_SYSTEM_H