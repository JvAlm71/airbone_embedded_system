#include "../include/airborne_system.h"
#include <string.h>
#include <errno.h>
#include <math.h>

// Variável global para controle do sistema
static airborne_system_t *g_system = NULL;

// Implementação do buffer circular thread-safe
int buffer_init(data_buffer_t *buffer) {
    if (!buffer) return -1;
    
    memset(buffer, 0, sizeof(data_buffer_t));
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        return -1;
    }
    
    if (pthread_cond_init(&buffer->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        return -1;
    }
    
    if (pthread_cond_init(&buffer->not_full, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        pthread_cond_destroy(&buffer->not_empty);
        return -1;
    }
    
    return 0;
}

int buffer_put(data_buffer_t *buffer, sensor_data_t *data) {
    if (!buffer || !data) return -1;
    
    pthread_mutex_lock(&buffer->mutex);
    
    // Aguarda espaço no buffer
    while (buffer->count == BUFFER_SIZE) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }
    
    // Adiciona dados ao buffer
    buffer->data[buffer->head] = *data;
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->count++;
    
    // Sinaliza que há dados disponíveis
    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
    
    return 0;
}

int buffer_get(data_buffer_t *buffer, sensor_data_t *data) {
    if (!buffer || !data) return -1;
    
    pthread_mutex_lock(&buffer->mutex);
    
    // Aguarda dados no buffer
    while (buffer->count == 0) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }
    
    // Remove dados do buffer
    *data = buffer->data[buffer->tail];
    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    buffer->count--;
    
    // Sinaliza que há espaço disponível
    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
    
    return 0;
}

void buffer_cleanup(data_buffer_t *buffer) {
    if (!buffer) return;
    
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
}

// Simulação de sensores da aeronave
void* sensor_thread_func(void *arg) {
    airborne_system_t *system = (airborne_system_t *)arg;
    sensor_data_t sensor_data;
    
    printf("[SENSOR] Thread iniciada\n");
    
    while (system->running) {
        // Simula leitura de sensores com variações realísticas
        time_t now = time(NULL);
        
        // Temperatura varia entre -50°C e 50°C com ruído
        sensor_data.temperature = 20.0f + 10.0f * sin(now * 0.01) + 
                                (rand() % 20 - 10) * 0.1f;
        
        // Pressão atmosférica varia com altitude simulada
        sensor_data.pressure = 1013.25f - (rand() % 100) * 0.1f;
        
        // Altitude simulada entre 0 e 10000m
        sensor_data.altitude = 5000.0f + 2000.0f * sin(now * 0.005) + 
                              (rand() % 200 - 100);
        
        sensor_data.timestamp = now;
        
        // Adiciona dados ao buffer
        if (buffer_put(&system->buffer, &sensor_data) == 0) {
            printf("[SENSOR] Dados coletados: T=%.1f°C, P=%.1fhPa, A=%.0fm\n",
                   sensor_data.temperature, sensor_data.pressure, sensor_data.altitude);
        }
        
        // Espera intervalo de amostragem (usando nanosleep para compatibilidade POSIX)
        struct timespec delay = {
            .tv_sec = SAMPLING_RATE_MS / 1000,
            .tv_nsec = (SAMPLING_RATE_MS % 1000) * 1000000
        };
        nanosleep(&delay, NULL);
    }
    
    printf("[SENSOR] Thread finalizada\n");
    return NULL;
}

// Thread de processamento dos dados
void* processing_thread_func(void *arg) {
    airborne_system_t *system = (airborne_system_t *)arg;
    sensor_data_t sensor_data;
    static float temp_avg = 0.0f;
    static float pressure_avg = 0.0f;
    static float altitude_avg = 0.0f;
    static int sample_count = 0;
    
    printf("[PROCESSING] Thread iniciada\n");
    
    while (system->running) {
        // Obtém dados do buffer
        if (buffer_get(&system->buffer, &sensor_data) == 0) {
            sample_count++;
            
            // Calcula médias móveis
            temp_avg = (temp_avg * (sample_count - 1) + sensor_data.temperature) / sample_count;
            pressure_avg = (pressure_avg * (sample_count - 1) + sensor_data.pressure) / sample_count;
            altitude_avg = (altitude_avg * (sample_count - 1) + sensor_data.altitude) / sample_count;
            
            // Verifica condições críticas
            if (sensor_data.temperature < -40.0f || sensor_data.temperature > 80.0f) {
                printf("[PROCESSING] ALERTA: Temperatura crítica: %.1f°C\n", sensor_data.temperature);
            }
            
            if (sensor_data.pressure < 300.0f) {
                printf("[PROCESSING] ALERTA: Pressão muito baixa: %.1fhPa\n", sensor_data.pressure);
            }
            
            if (sensor_data.altitude > 12000.0f) {
                printf("[PROCESSING] ALERTA: Altitude muito alta: %.0fm\n", sensor_data.altitude);
            }
            
            // A cada 10 amostras, exibe estatísticas
            if (sample_count % 10 == 0) {
                printf("[PROCESSING] Médias (n=%d): T=%.1f°C, P=%.1fhPa, A=%.0fm\n",
                       sample_count, temp_avg, pressure_avg, altitude_avg);
            }
        }
    }
    
    printf("[PROCESSING] Thread finalizada\n");
    return NULL;
}

// Thread de monitoramento do sistema
void* monitoring_thread_func(void *arg) {
    airborne_system_t *system = (airborne_system_t *)arg;
    
    printf("[MONITORING] Thread iniciada\n");
    
    while (system->running) {
        pthread_mutex_lock(&system->buffer.mutex);
        int buffer_count = system->buffer.count;
        pthread_mutex_unlock(&system->buffer.mutex);
        
        printf("[MONITORING] Buffer: %d/%d utilizados\n", buffer_count, BUFFER_SIZE);
        
        // Verifica se o buffer está ficando cheio
        if (buffer_count > BUFFER_SIZE * 0.8) {
            printf("[MONITORING] AVISO: Buffer quase cheio (%d%%)\n", 
                   (buffer_count * 100) / BUFFER_SIZE);
        }
        
        // Monitora a cada 5 segundos
        sleep(5);
    }
    
    printf("[MONITORING] Thread finalizada\n");
    return NULL;
}

// Inicialização do sistema
int airborne_system_init(airborne_system_t *system) {
    if (!system) return -1;
    
    memset(system, 0, sizeof(airborne_system_t));
    system->running = false;
    
    // Inicializa buffer
    if (buffer_init(&system->buffer) != 0) {
        printf("Erro ao inicializar buffer\n");
        return -1;
    }
    
    // Inicializa mutex do sistema
    if (pthread_mutex_init(&system->system_mutex, NULL) != 0) {
        printf("Erro ao inicializar mutex do sistema\n");
        buffer_cleanup(&system->buffer);
        return -1;
    }
    
    printf("Sistema embarcado inicializado com sucesso\n");
    return 0;
}

// Iniciar threads do sistema
int airborne_system_start(airborne_system_t *system) {
    if (!system) return -1;
    
    pthread_mutex_lock(&system->system_mutex);
    system->running = true;
    pthread_mutex_unlock(&system->system_mutex);
    
    // Cria threads
    if (pthread_create(&system->sensor_thread, NULL, sensor_thread_func, system) != 0) {
        printf("Erro ao criar thread de sensores\n");
        return -1;
    }
    
    if (pthread_create(&system->processing_thread, NULL, processing_thread_func, system) != 0) {
        printf("Erro ao criar thread de processamento\n");
        return -1;
    }
    
    if (pthread_create(&system->monitoring_thread, NULL, monitoring_thread_func, system) != 0) {
        printf("Erro ao criar thread de monitoramento\n");
        return -1;
    }
    
    printf("Todas as threads iniciadas com sucesso\n");
    return 0;
}

// Parar o sistema
void airborne_system_stop(airborne_system_t *system) {
    if (!system) return;
    
    printf("Parando sistema embarcado...\n");
    
    pthread_mutex_lock(&system->system_mutex);
    system->running = false;
    pthread_mutex_unlock(&system->system_mutex);
    
    // Sinaliza condições para acordar threads bloqueadas
    pthread_cond_broadcast(&system->buffer.not_empty);
    pthread_cond_broadcast(&system->buffer.not_full);
    
    // Aguarda threads terminarem
    pthread_join(system->sensor_thread, NULL);
    pthread_join(system->processing_thread, NULL);
    pthread_join(system->monitoring_thread, NULL);
    
    printf("Sistema parado com sucesso\n");
}

// Limpeza do sistema
void airborne_system_cleanup(airborne_system_t *system) {
    if (!system) return;
    
    buffer_cleanup(&system->buffer);
    pthread_mutex_destroy(&system->system_mutex);
    
    printf("Recursos do sistema liberados\n");
}

// Handler para sinais
void signal_handler(int sig) {
    if (g_system) {
        printf("\nSinal %d recebido. Parando sistema...\n", sig);
        airborne_system_stop(g_system);
    }
}

// Função principal
int main() {
    airborne_system_t system;
    
    // Configura handler de sinais
    g_system = &system;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== Sistema Embarcado de Aeronave ===\n");
    printf("Pressione Ctrl+C para parar\n\n");
    
    // Inicializa sistema
    if (airborne_system_init(&system) != 0) {
        printf("Falha na inicialização do sistema\n");
        return 1;
    }
    
    // Inicia sistema
    if (airborne_system_start(&system) != 0) {
        printf("Falha ao iniciar sistema\n");
        airborne_system_cleanup(&system);
        return 1;
    }
    
    // Aguarda sinal para parar
    pause();
    
    // Limpeza
    airborne_system_cleanup(&system);
    g_system = NULL;
    
    printf("Sistema finalizado\n");
    return 0;
}