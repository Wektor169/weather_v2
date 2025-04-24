#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "weather_module.h"
#include "MQTT_module.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
//#include <sys/time.h>

#define NUM_WOJEWODZTWA 16
#define NUM_THREADS 16

typedef struct {
    int woj_id;
    char final_string[2048];  // Bufer na dane JSON
    sem_t *semaphore;  // Semafor, który będzie kontrolować dostęp do sekcji krytycznej
    sem_t *send_semaphore;  // Semafor synchronizujący wysyłanie danych
} ThreadArgs;

void *fetch_weather_data(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int woj_id = args->woj_id;
    int num_miast = 0;
    char *final_string = args->final_string;
    while ((dane[woj_id].miasta[num_miast] != NULL) && (num_miast < 5)) {
        get_weather(dane[woj_id].lat[num_miast], dane[woj_id].lon[num_miast], dane[woj_id].nazwa_wojewodztwa + 6);
        const char *name_tab[] = {"temp", "speed", "all"};
        const char *name_tab_pl[] = {"Temperatura", "Predkosc wiatru", "Zachmurzenie"};
        const char **result = get_data(name_tab, 3, dane[woj_id].nazwa_wojewodztwa + 6);
        if (result) {
            char city_string[256];
            snprintf(city_string, sizeof(city_string),
                     "\"%s\": {\"%s\": %s, \"%s\": %s, \"%s\": %s}",
                     dane[woj_id].miasta[num_miast],
                     name_tab_pl[0], result[0],
                     name_tab_pl[1], result[1],
                     name_tab_pl[2], result[2]);
            if (num_miast > 0) strcat(final_string, ", ");
            strcat(final_string, city_string);
            for (int i = 0; i < 3; i++) free(result[i]);
            free(result);
        }
        num_miast++;
    }
    strcat(final_string, "}");  // Zakończenie danych JSON

    // Po pobraniu danych, uruchamiam wątek wysyłający
    sem_post(args->send_semaphore);

    return NULL;
}

void *send_weather_data(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char *final_string = args->final_string;
    int woj_id = args->woj_id;

    // Czekam, aż wątek pobierający zakończy zadanie
    sem_wait(args->send_semaphore);

    // Sekcja krytyczna — wysyłanie MQTT
    sem_wait(args->semaphore);  // Tylko jeden wątek na raz może wysłać dane
    sendMqttMessage(final_string, dane[woj_id].nazwa_wojewodztwa);
    sem_post(args->semaphore);  // Zwalniam semafor, umożliwiając wysyłanie innym wątkom

    return NULL;
}

int main() {
    /* //Mierzenie czasu
    struct timeval start, end;
    gettimeofday(&start, NULL); */

    pthread_t fetch_threads[NUM_THREADS];  // Wątki do pobierania danych
    pthread_t send_threads[NUM_THREADS];   // Wątki do wysyłania danych
    ThreadArgs args[NUM_THREADS];           // Struktury z argumentami dla wątków
    sem_t semaphore;  // Semafor dla sekcji krytycznych (wysyłanie danych)
    sem_t send_semaphore[NUM_THREADS];     // Tablica semaforów do synchronizacji wątków

    // Inicjalizacja semafora dla sekcji krytycznej
    sem_init(&semaphore, 0, 1);  // Tylko jeden wątek wysyła dane w tym samym czasie

    int liczba_iteracji = 0;

    while (liczba_iteracji<5){
        for (int i = 0; i < NUM_THREADS; i++) {
            args[i].woj_id = i;
            args[i].final_string[0] = '\0';  // Inicjalizujemy pusty ciąg
            args[i].semaphore = &semaphore;  // Używamy globalnego semafora dla sekcji krytycznej
            sem_init(&send_semaphore[i], 0, 0);  // Inicjalizujemy semafor dla każdego wątku
            args[i].send_semaphore = &send_semaphore[i];

            // Tworzymy wątki odpowiedzialne za pobieranie danych
            if (pthread_create(&fetch_threads[i], NULL, fetch_weather_data, &args[i]) != 0) {
                perror("Error creating fetch weather data thread");
                exit(1);
            }

            // Tworzymy wątki odpowiedzialne za wysyłanie danych
            if (pthread_create(&send_threads[i], NULL, send_weather_data, &args[i]) != 0) {
                perror("Error creating send weather data thread");
                exit(1);
            }
        }

        // Czekam na zakończenie pobierania danych przez wszystkie wątki
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(fetch_threads[i], NULL);
        }

        // Czekam na zakończenie wysyłania danych przez wszystkie wątki
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(send_threads[i], NULL);
        }
        liczba_iteracji = liczba_iteracji + 1;
        sleep(30);
    }

    // Niszczenie semaforów
    sem_destroy(&semaphore);
    for (int i = 0; i < NUM_THREADS; i++) {
        sem_destroy(&send_semaphore[i]);
    }


    // Obliczenie czasu trwania
    /* gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed_time = seconds + microseconds / 1000000.0;
    printf("Czas trwania programu: %.6f sekund\n", elapsed_time); */

    return 0;
}