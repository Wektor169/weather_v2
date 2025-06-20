#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "weather_module.h"
#include "MQTT_module.h"
#include "time_synchronization.h"

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
    strcpy(final_string, "{");
    while ((dane[woj_id].miasta[num_miast] != NULL) && (num_miast < 5)) {
        get_weather(dane[woj_id].lat[num_miast], dane[woj_id].lon[num_miast], dane[woj_id].nazwa_wojewodztwa + 8);
        const char *name_tab[] = {"temp", "speed", "all"};
        const char *name_tab_pl[] = {"Temperatura", "Predkosc wiatru", "Zachmurzenie"};
        const char **result = get_data(name_tab, 3, dane[woj_id].nazwa_wojewodztwa + 8);
        if (result == NULL) {
            fprintf(stderr, "Błąd: get_data zwróciło NULL dla województwa %s\n", dane[woj_id].nazwa_wojewodztwa);
            continue;
        }
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

    pthread_t fetch_threads[NUM_THREADS];  // Wątki do pobierania danych
    pthread_t send_threads[NUM_THREADS];   // Wątki do wysyłania danych
    ThreadArgs args[NUM_THREADS];           // Struktury z argumentami dla wątków
    sem_t semaphore;  // Semafor dla sekcji krytycznych (wysyłanie danych)
    sem_t send_semaphore[NUM_THREADS];     // Tablica semaforów do synchronizacji wątków

    // Inicjalizacja semafora dla sekcji krytycznej
    sem_init(&semaphore, 0, 1);  // Tylko jeden wątek wysyła dane w tym samym czasie
    if (sem_init(&semaphore, 0, 1) != 0) {
        perror("Error initializing global semaphore");
        exit(1);
    }

    int liczba_iteracji = 0;

    while (1){
    //while (liczba_iteracji<1){
        sleep_until_next_min_interval(5);
        print_date();
        for (int i = 0; i < NUM_THREADS; i++) {
            args[i].woj_id = i;
            args[i].final_string[0] = '\0';  // Inicjalizujemy pusty ciąg
            args[i].semaphore = &semaphore;  // Używamy globalnego semafora dla sekcji krytycznej
            sem_init(&send_semaphore[i], 0, 0);  // Inicjalizujemy semafor dla każdego wątku
            if (sem_init(&semaphore, 0, 1) != 0) {
                perror("Błąd podczas inicjalizacji semafora globalnego");
                exit(1);
            }
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
        printf(" The program successfully received and sent.\n");
        //liczba_iteracji = liczba_iteracji + 1;
    }


    sem_destroy(&semaphore);
    for (int i = 0; i < NUM_THREADS; i++) {
        sem_destroy(&send_semaphore[i]);
    }

    return 0;
}