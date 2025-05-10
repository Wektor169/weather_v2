#include "weather_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 void get_weather(double latitiute, double longitiute, const char* filename){
    char command[512];  // Bufor na komendę
    sprintf(command, "curl -s \"https://api.openweathermap.org/data/2.5/weather?lat=%lf&lon=%lf&appid=0dcaf6e2a4e68809437b92f4e103a2c9\" > \"%s\"", latitiute, longitiute, filename);
    system(command);
     int result_command = system(command);
     if (result_command != 0) {
         fprintf(stderr, "Błąd podczas wykonywania komendy curl. Kod: %d\n", result_command);
     }
};

void trim(char *str) { // Funkcja do usuwania białych znaków z początku i końca łańcucha
    int start = 0;
    unsigned int end = strlen(str) - 1;
    while (str[start] == ' ' || str[start] == '\t' || str[start] == '\n') { // Zignorowanie początkowych białych znaków
        start++;
    }
    while (str[end] == ' ' || str[end] == '\t' || str[end] == '\n') { // Zignorowanie końcowych białych znaków
        end--;
    }

    for (int i = 0; i <= end - start; i++) { // Przesunięcie tekstu
        str[i] = str[start + i];
    }
    str[end - start + 1] = '\0';  // Dodanie null terminatora
}


char* get_field_value(const char* json_str, const char* field_name) { // Funkcja do znajdowania wartości pola w JSON (ręcznie)
    char pattern[256]; // Tworzymy wzorzec do szukania "field_name": "wartość"
    snprintf(pattern, sizeof(pattern), "\"%s\":", field_name);
    char* start_pos = strstr(json_str, pattern); // Znajdujemy pozycję, w której zaczyna się pole
    if (start_pos == NULL) {
        fprintf(stderr, "Nie znaleziono pola: %s\n", field_name);
        return NULL;
    }
    start_pos += strlen(pattern); // Znajdujemy początek wartości (po ":" i ewentualnych białych znakach)
    while (*start_pos == ' ' || *start_pos == '\t' || *start_pos == '\n') {
        start_pos++;
    }
    if (*start_pos == '"') {
        // Sprawdzamy, czy wartość jest w cudzysłowie (ciąg znaków)
        start_pos++;
        char* end_pos = strchr(start_pos, '"');
        if (end_pos != NULL) {
            size_t length = end_pos - start_pos;
            char* value = (char*)malloc(length + 1);
            strncpy(value, start_pos, length);
            value[length] = '\0';
            return value;
        }
    }
    if ((*start_pos >= '0' && *start_pos <= '9') || *start_pos == '-' || *start_pos == '.') { // Jeśli wartość to liczba (całkowita lub zmiennoprzecinkowa)
        char* end_pos = start_pos;
        while ((*end_pos >= '0' && *end_pos <= '9') || *end_pos == '.' || *end_pos == 'e' || *end_pos == '-' || *end_pos == '+') {
            end_pos++;  // Przechodzimy przez całą liczbę (np. 22.34 lub -3.4e+05)
        }
        size_t length = end_pos - start_pos;
        char* value = (char*)malloc(length + 1);
        strncpy(value, start_pos, length);
        value[length] = '\0';
        return value;
    }
    return NULL;
}

char** get_data(const char **tab, int size_tab, const char* filename) {
    char **data_tab = (char**)malloc(sizeof(char *) * size_tab);
    if (!data_tab) {
        fprintf(stderr, "Błąd alokacji pamięci dla data_tab.\n");
        return NULL;
    }
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Nie można otworzyć pliku!\n");
        free(data_tab);
        return NULL;
    }
    fseek(file, 0, SEEK_END); // przesunięcie wskaźnika na koniec
    long file_size = ftell(file);  // obilczenie dlugosci pliku
    if (file_size < 0) {
        fprintf(stderr, "Błąd podczas obliczania rozmiaru pliku.\n");
        fclose(file);
        free(data_tab);
        return NULL;
    }
    fseek(file, 0, SEEK_SET); //  przywrocenie wskaznika na poczatek
    char* json_str = (char*)malloc(file_size + 1); //rezerwacja pamieci na dane z pliku
    if (!json_str) {
        fprintf(stderr, "Błąd alokacji pamięci dla JSON.\n");
        fclose(file);
        free(data_tab);
        return NULL;
    }
    fread(json_str, 1, file_size, file); // odczytanie danych z pliku do zaalakowanej pamieci
    json_str[file_size] = '\0';  // Zakończenie łańcucha znakowego
    for (int i = 0; i < size_tab; i++) {
        char* field = get_field_value(json_str, tab[i]);
        if (field) {
            data_tab[i] = strdup(field);
            free(field);
        } else {
            fprintf(stderr, "Nie udało się pobrać wartości dla pola: %s\n", tab[i]);
            data_tab[i] = NULL; // zachowujemy spójność
        }
    }
    fclose(file); // Zamknij plik
    free(json_str);
    return data_tab;
}

Wojewodztwo dane[NUM_WOJEWODZTWA] = {
    {"Dawid/Dolnoslaskie", {"Wroclaw", "Legnica", "Walbrzych", "Jelenia Gora", "Lubin"},
     {51.1079, 51.2103, 50.8978, 50.8974, 51.3920},
     {17.0385, 16.1557, 16.2893, 15.5101, 16.1907}},

    {"Dawid/Kujawsko-pomorskie", {"Bydgoszcz", "Torun", "Wloclawek", "Inowroclaw", NULL},
     {53.1235, 53.0138, 52.6464, 52.7984, 0.0},
     {17.9920, 18.5984, 19.1212, 18.2335, 0.0}},

    {"Dawid/Lubelskie", {"Lublin", "Zamosc", "Chelm", "Biala Podlaska", NULL},
     {51.2465, 50.7272, 51.1309, 51.2454, 0.0},
     {22.5687, 23.1651, 23.2837, 22.5533, 0.0}},

    {"Dawid/Lubuskie", {"Zielona Gora", "Gorzow Wielkopolski", "Nowa Sol", NULL, NULL},
     {51.1194, 52.7366, 51.7965, 0.0, 0.0},
     {15.5061, 15.2272, 15.5672, 0.0, 0.0}},

    {"Dawid/Lodzkie", {"Lodz", "Piotrkow Trybunalski", "Belchatow", "Skierniewice", NULL},
     {51.7592, 51.4040, 51.3682, 51.9637, 0.0},
     {19.4560, 19.6364, 19.3102, 19.6825, 0.0}},

    {"Dawid/Malopolskie", {"Krakow", "Tarnow", "Nowy Sacz", "Oswiecim", NULL},
     {50.0647, 50.0114, 49.6294, 50.0413, 0.0},
     {19.9448, 21.0227, 20.5500, 19.7734, 0.0}},

    {"Dawid/Mazowieckie", {"Warszawa", "Radom", "Plock", "Ostroleka", "Siedlce"},
     {52.2298, 51.3962, 52.5461, 52.4165, 52.0997},
     {21.0118, 21.1460, 19.7033, 21.6092, 22.0775}},

    {"Dawid/Opolskie", {"Opole", "Kedzierzyn-Kozle", "Nysa", NULL, NULL},
     {50.6758, 50.6073, 50.4672, 0.0, 0.0},
     {17.9213, 18.2080, 17.3337, 0.0, 0.0}},

    {"Dawid/Podkarpackie", {"Rzeszow", "Przemysl", "Krosno", "Tarnobrzeg", NULL},
     {50.0415, 49.7837, 49.6931, 50.5924, 0.0},
     {22.0007, 22.6199, 21.6955, 21.7152, 0.0}},

    {"Dawid/Podlaskie", {"Bialystok", "Suwalki", "Lomza", NULL, NULL},
     {53.1325, 54.0995, 52.7431, 0.0, 0.0},
     {23.1688, 22.9392, 22.0736, 0.0, 0.0}},

    {"Dawid/Pomorskie", {"Gdansk", "Gdynia", "Slupsk", "Tczew", NULL},
     {54.3520, 54.5186, 54.4642, 54.0864, 0.0},
     {18.6466, 18.5293, 17.6472, 18.0000, 0.0}},

    {"Dawid/Slaskie", {"Katowice", "Czestochowa", "Gliwice", "Bielsko-Biala", NULL},
     {50.2649, 50.8113, 50.2945, 49.8222, 0.0},
     {19.0295, 19.1245, 18.6753, 19.1407, 0.0}},

    {"Dawid/Swietokrzyskie", {"Kielce", "Starachowice", "Ostrowiec Swietokrzyski", NULL, NULL},
     {50.8667, 51.2895, 50.7434, 0.0, 0.0},
     {20.6286, 21.1378, 21.3833, 0.0, 0.0}},

    {"Dawid/Warminsko-mazurskie", {"Olsztyn", "Elblag", "Elk", "Gizycko", NULL},
     {53.7793, 54.1867, 53.6630, 53.7232, 0.0},
     {20.4847, 19.4089, 22.3943, 21.7635, 0.0}},

    {"Dawid/Wielkopolskie", {"Poznan", "Kalisz", "Konin", "Leszno", NULL},
     {52.4080, 51.7825, 52.2200, 51.8481, 0.0},
     {16.9280, 18.0832, 18.2396, 17.0040, 0.0}},

    {"Dawid/Zachodniopomorskie", {"Szczecin", "Koszalin", "Stargard", "Swinoujscie", NULL},
     {53.4289, 54.3074, 53.3266, 53.9114, 0.0},
     {14.5528, 16.1681, 15.6825, 14.5504, 0.0}},
};

