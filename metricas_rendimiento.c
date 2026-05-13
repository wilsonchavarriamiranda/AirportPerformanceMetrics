#define DEBUG 0

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define TOTAL_SERVICIOS 4
#define MALETAS 1000

int TOTAL_VUELOS;
int PISTAS_DISPONIBLES;
int TAMANO_BANDA;
int PERSONAL_TIERRA;
int MODO_SERIAL;

pthread_mutex_t mutex;

sem_t semaforoPistas;
int pistas_ocupadas = 0;

typedef struct {
    int idVuelo;
    int idMaleta;
} Maleta;

typedef struct {
    int idVuelo;
    int serviciosTerminados;
    pthread_mutex_t mutexBarrera;
    pthread_cond_t condBarrera;
} Vuelo;

typedef struct {
    Vuelo *vuelo;
    int idServicio;
} ServicioArgs;

Maleta *banda;
int indiceEntrada = 0;
int indiceSalida = 0;

sem_t vacia;
sem_t llena;

volatile int corriendo = 1;

const char *nombre_servicio(int id) {
    switch (id) {
        case 1: return "Reabastecimiento de combustible";
        case 2: return "Limpieza de cabina";
        case 3: return "Abastecimiento";
        case 4: return "Revision tecnica";
        default: return "Servicio desconocido";
    }
}

void render_radar_log(int vuelo_id, const char *accion) {
    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d %s | Pistas Ocupadas: %d/%d\n",
               vuelo_id, accion, pistas_ocupadas, PISTAS_DISPONIBLES);    
    }
    pthread_mutex_unlock(&mutex);
}

void barrera_servicios(Vuelo *v) {
    pthread_mutex_lock(&v->mutexBarrera);
    v->serviciosTerminados++;

    if (v->serviciosTerminados < TOTAL_SERVICIOS) {
        while (v->serviciosTerminados < TOTAL_SERVICIOS) {
            pthread_cond_wait(&v->condBarrera, &v->mutexBarrera);
        }
    } else {
        pthread_cond_broadcast(&v->condBarrera);
    }

    pthread_mutex_unlock(&v->mutexBarrera);
}

void *rutina_servicio(void *arg) {
    ServicioArgs *datos = (ServicioArgs *)arg;
    Vuelo *v = datos->vuelo;
    int id = datos->idServicio;

    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d %s iniciado.\n", v->idVuelo, nombre_servicio(id));
    }
    pthread_mutex_unlock(&mutex);

    switch (id) {
        case 1:
            usleep(1000 + rand() % 2000);
            break;
        case 2:
            usleep(1000 + rand() % 2000);
            break;
        case 3:
            usleep(1000 + rand() % 2000);
            break;
        case 4:
            usleep(4000 + rand() % 2000);
            break;
    }

    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d %s finalizado. Esperando a los demas servicios...\n",
               v->idVuelo, nombre_servicio(id));
    }
    pthread_mutex_unlock(&mutex);
    
    if (!MODO_SERIAL) {
        barrera_servicios(v);
    }

    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d %s da el visto bueno.\n", v->idVuelo, nombre_servicio(id));
    }
    pthread_mutex_unlock(&mutex);

    free(datos);
    return NULL;
}

void *rutina_vuelo(void *arg) {
    int vuelo_id = *(int *)arg;
    free(arg);

    render_radar_log(vuelo_id, ": entrando al espacio aereo. Solicitando pista para el aterrizaje.");

    sem_wait(&semaforoPistas);

    pthread_mutex_lock(&mutex);
    pistas_ocupadas++;
    if(DEBUG) {
        printf("Vuelo %02d ha sido autorizado para aterrizar. Pista asignada %d.\n",
            vuelo_id, pistas_ocupadas);
    }
    pthread_mutex_unlock(&mutex);

    render_radar_log(vuelo_id, ">> aterrizando y desembarcando.");

    usleep(1000 + rand() % 4000);

    int maletas = MALETAS;
    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d ha desembarcado. Tiene %d maletas para colocar en la banda.\n",
               vuelo_id, maletas);
    }
    pthread_mutex_unlock(&mutex);

    for (int m = 1; m <= maletas; m++) {
        sem_wait(&vacia);

        pthread_mutex_lock(&mutex);
        banda[indiceEntrada] = (Maleta){vuelo_id, m};
        indiceEntrada = (indiceEntrada + 1) % TAMANO_BANDA;
        pthread_mutex_unlock(&mutex);

        sem_post(&llena);

        int llenas;
        sem_getvalue(&llena, &llenas);

        pthread_mutex_lock(&mutex);
        if(DEBUG) {
            printf("Vuelo %02d ha colocado maleta %d en la banda. Maletas en banda: %d\n",
                   vuelo_id, m, llenas);
        }
        pthread_mutex_unlock(&mutex);

        usleep(1000 + rand() % 2000);
    }

    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d entra en preparacion tecnica. Iniciando 4 servicios en paralelo.\n",
               vuelo_id);
    }
    pthread_mutex_unlock(&mutex);

    Vuelo v;
    v.idVuelo = vuelo_id;
    v.serviciosTerminados = 0;
    pthread_mutex_init(&v.mutexBarrera, NULL);
    pthread_cond_init(&v.condBarrera, NULL);

    if (MODO_SERIAL) {
        for (int i = 0; i < TOTAL_SERVICIOS; i++) {
            ServicioArgs *datos = malloc(sizeof(ServicioArgs));
            datos->vuelo = &v;
            datos->idServicio = i + 1;

            rutina_servicio(datos);
        }
    } else {
        pthread_t servicios[TOTAL_SERVICIOS];

        for (int i = 0; i < TOTAL_SERVICIOS; i++) {
            ServicioArgs *datos = malloc(sizeof(ServicioArgs));
            if (datos == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            datos->vuelo = &v;
            datos->idServicio = i + 1;

            if (pthread_create(&servicios[i], NULL, rutina_servicio, datos) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < TOTAL_SERVICIOS; i++) {
            pthread_join(servicios[i], NULL);
        }
    }

    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Vuelo %02d: todos los servicios tecnicos han finalizado. Visto bueno completo para despegue.\n",
               vuelo_id);
    }
    pthread_mutex_unlock(&mutex);

    pthread_cond_destroy(&v.condBarrera);
    pthread_mutex_destroy(&v.mutexBarrera);

    usleep(1000 + rand() % 2000);

    pthread_mutex_lock(&mutex);
    pistas_ocupadas--;
    pthread_mutex_unlock(&mutex);

    render_radar_log(vuelo_id, "<< ha despegado. Pista liberada.");

    sem_post(&semaforoPistas);
    return NULL;
}

void *rutina_personal(void *arg) {
    int id = *(int *)arg;
    free(arg);

    while (1) {
        sem_wait(&llena);

        if (!corriendo) {
            break;
        }

        pthread_mutex_lock(&mutex);
        Maleta m = banda[indiceSalida];
        indiceSalida = (indiceSalida + 1) % TAMANO_BANDA;
        pthread_mutex_unlock(&mutex);

        sem_post(&vacia);

        int llenas;
        sem_getvalue(&llena, &llenas);

        pthread_mutex_lock(&mutex);
        if(DEBUG) {
            printf("Personal %d ha recogido maleta %d del vuelo %02d. Maletas restantes en banda: %d\n",
                   id, m.idMaleta, m.idVuelo, llenas);
        }
        pthread_mutex_unlock(&mutex);

        usleep(1000 + rand() % 2000);
    }

    pthread_mutex_lock(&mutex);
    if(DEBUG) {
        printf("Personal %d ha terminado su turno.\n", id);
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

double calcular_segundos(struct timespec inicio, struct timespec fin) {
    return (fin.tv_sec - inicio.tv_sec) +
           (fin.tv_nsec - inicio.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {

    struct timespec t_inicio_init, t_fin_init;
    struct timespec t_inicio_paralelo, t_fin_paralelo;
    struct timespec t_inicio_destroy, t_fin_destroy;

    srand(time(NULL));

    if (argc != 6) {
        fprintf(stderr, "Uso: %s <TOTAL_VUELOS> <PISTAS_DISPONIBLES> <TAMANO_BANDA> <PERSONAL_TIERRA> <MODO_SERIAL>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t_inicio_init);

    TOTAL_VUELOS = atoi(argv[1]);
    PISTAS_DISPONIBLES = atoi(argv[2]);
    TAMANO_BANDA = atoi(argv[3]);
    PERSONAL_TIERRA = atoi(argv[4]);
    MODO_SERIAL = atoi(argv[5]);

    banda = malloc(sizeof(Maleta) * TAMANO_BANDA);

    srand(time(NULL));

    sem_init(&semaforoPistas, 0, PISTAS_DISPONIBLES);
    sem_init(&vacia, 0, TAMANO_BANDA);
    sem_init(&llena, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    pthread_t *personal = malloc(sizeof(pthread_t) * PERSONAL_TIERRA);
    pthread_t *torre_control = malloc(sizeof(pthread_t) * TOTAL_VUELOS);

    clock_gettime(CLOCK_MONOTONIC, &t_fin_init);

    if(DEBUG) {
        printf("=== Sistema de control de trafico aereo ===\n");
        printf("Capacidad del aeropuerto: %d pistas simultaneas | Banda soporta %d maletas | Personal disponible: %d\n\n",
                PISTAS_DISPONIBLES, TAMANO_BANDA, PERSONAL_TIERRA);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_inicio_paralelo);

    for (int i = 0; i < PERSONAL_TIERRA; i++) {
        int *id = malloc(sizeof(int));
        if (id == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *id = i + 1;
        pthread_create(&personal[i], NULL, rutina_personal, id);
    }

    for (int i = 0; i < TOTAL_VUELOS; i++) {
        int *id = malloc(sizeof(int));
        if (id == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *id = i + 1;
        pthread_create(&torre_control[i], NULL, rutina_vuelo, id);

        usleep(rand() % 5000);
    }

    for (int i = 0; i < TOTAL_VUELOS; i++) {
        pthread_join(torre_control[i], NULL);
    }

    if(DEBUG) {
        printf("Ya no quedan aviones, esperando a que se vacie la banda de equipaje...\n");
    }

    int bandaSigue;
    do {
        sem_getvalue(&llena, &bandaSigue);
        usleep(1000);
    } while (bandaSigue > 0);

    corriendo = 0;

    for (int i = 0; i < PERSONAL_TIERRA; i++) {
        sem_post(&llena);
    }

    for (int i = 0; i < PERSONAL_TIERRA; i++) {
        pthread_join(personal[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_fin_paralelo);

    if(DEBUG) {
        printf("\n=== CIERRE DE OPERACIONES. ESPACIO AEREO DESPEJADO. ===\n");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t_inicio_destroy);
     
    sem_destroy(&semaforoPistas);
    sem_destroy(&vacia);
    sem_destroy(&llena);
    pthread_mutex_destroy(&mutex);

    free(torre_control);
    free(personal);
    free(banda);

    clock_gettime(CLOCK_MONOTONIC, &t_fin_destroy);

    double tiempo_init = calcular_segundos(t_inicio_init, t_fin_init);
    double tiempo_paralelo = calcular_segundos(t_inicio_paralelo, t_fin_paralelo);
    double tiempo_destroy = calcular_segundos(t_inicio_destroy, t_fin_destroy);
    double tiempo_serial_puro = tiempo_init + tiempo_destroy;

    printf("\n=== METRICAS ===\n");
    printf("Vuelos: %d\n", TOTAL_VUELOS);
    printf("Maletas: %d\n", MALETAS);
    printf("Pistas: %d\n", PISTAS_DISPONIBLES);
    printf("Tamano banda: %d\n", TAMANO_BANDA);
    printf("Personal tierra: %d\n", PERSONAL_TIERRA);
    printf("Tiempo init: %.6f\n", tiempo_init);
    printf("Tiempo paralelo: %.6f\n", tiempo_paralelo);
    printf("Tiempo destroy: %.6f\n", tiempo_destroy);
    printf("Tiempo serial puro init + destroy: %.6f\n", tiempo_serial_puro);

    return 0;
}