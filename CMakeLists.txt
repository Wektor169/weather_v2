cmake_minimum_required(VERSION 3.30)
project(untitled C)

set(CMAKE_C_STANDARD 11)

# Ustaw ścieżki do libcurl (zmień jeśli używasz innego katalogu)
include_directories("C:/Users/dawid/CLionProjects/weather_v2/curl/include")
link_directories("C:/Users/dawid/CLionProjects/weather_v2/curl/lib")

include_directories("C:/Users/dawid/Downloads/paho.mqtt.c-master/paho.mqtt.c-master/src")
link_directories("C:/Users/dawid/Downloads/paho.mqtt.c-master/paho.mqtt.c-master/cmake-build-debug/src")

include_directories(include)
include_directories(src)

# Tworzenie pliku wykonywalnego
add_executable(weather_v2 main.c
        src/weather_module.c
        include/weather_module.h
        include/MQTT_module.h
        src/MQTT_module.c
        src/time_synchronization.c
        include/time_synchronization.h
)

# Podłączanie biblioteki curl
target_link_libraries(weather_v2 curl)
target_link_libraries(weather_v2 libpaho-mqtt3c)  # Zmieniaj nazwę na odpowiednią
