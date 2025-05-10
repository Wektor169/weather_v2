#include "MQTT_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <MQTTClient.h>
#include <string.h>

void sendMqttMessage(const char *message, const char *topic) {
    if (message == NULL || topic == NULL) {
        fprintf(stderr, "Error: message or topic is NULL.\n");
        return;
    }
    MQTTClient client;
    MQTTClient_connectOptions options = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions sslOptions = MQTTClient_SSLOptions_initializer;
    MQTTClient_message mess = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);
    options.keepAliveInterval = 25;
    options.cleansession = 1;
    options.password = PASSWORD;
    options.ssl = &sslOptions;
    options.username = CLIENTID;
    options.serverURIcount = 0;
    options.serverURIs = NULL;
    if ((rc = MQTTClient_connect(client, &options)) != MQTTCLIENT_SUCCESS) {
        printf("Connect failed with error code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    mess.payload = (void*)message;
    mess.payloadlen = (int)strlen(message);
    mess.qos = QOS;
    mess.retained = 0;

    MQTTClient_publishMessage(client, topic, &mess, &token);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    //printf("Send message: \"%s\" to the topic: \"%s\". \n", message, topic);
    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
}