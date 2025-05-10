#ifndef WEATHER_MODULE_H
#define WEATHER_MODULE_H

#define NUM_WOJEWODZTWA 16   // Liczba województw

void get_weather(double latitiute, double longitiute, const char* filename);
void trim(char *str);
char* get_field_value(const char* json_str, const char* field_name);
char** get_data(const char **tab, int size_tab, const char* filename);

typedef struct { // Struktura, która przechowuje dane dla każdego województwa
    char* nazwa_wojewodztwa;
    char* miasta[5];      // Tablica miast (max 5 miast w województwie)
    double lat[5];         // Szerokości geograficzne
    double lon[5];         // Długości geograficzne
} Wojewodztwo;

extern Wojewodztwo dane[NUM_WOJEWODZTWA];

#endif //WEATHER_MODULE_H
