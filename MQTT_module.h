#ifndef MQTT_MODULE_H
#define MQTT_MODULE_H

#define ADDRESS     "mqtt://192.168.194.242:1883/"  // moj broker mqtt
#define CLIENTID    "pscr-mqtt-user"
#define PASSWORD    "O#$0RvcmHeqN6%t*miF8"
#define QOS         1
#define TIMEOUT     10000

void sendMqttMessage(const char *message, const char *topic);

#endif //MQTT_MODULE_H
