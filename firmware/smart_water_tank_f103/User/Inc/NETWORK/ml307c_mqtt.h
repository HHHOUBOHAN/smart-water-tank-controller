#ifndef NETWORK_ML307C_MQTT_H
#define NETWORK_ML307C_MQTT_H

#include <stdbool.h>
#include <stddef.h>

bool ML307C_MQTT_IsSupported(const char *response);
int ML307C_MQTT_ParseConnectionState(const char *response, int connect_id);
bool ML307C_MQTT_HasSubAck(const char *response, int connect_id);
bool ML307C_MQTT_HasPubAck(const char *response, int connect_id);
bool ML307C_MQTT_HasPubAckForMessage(const char *response,
                                     int connect_id,
                                     int message_id);
int ML307C_MQTT_ParsePublishMessageId(const char *response,
                                      int connect_id);
bool ML307C_MQTT_HasTimeout(const char *response);
bool ML307C_MQTT_ExtractPublish(const char *line,
                                int connect_id,
                                char *topic,
                                size_t topic_size,
                                char *payload,
                                size_t payload_size);

bool ML307C_MQTT_BuildConnect(char *buffer,
                              size_t buffer_size,
                              int connect_id,
                              const char *host,
                              const char *port,
                              const char *client_id,
                              const char *username,
                              const char *password);
bool ML307C_MQTT_BuildSubscribe(char *buffer,
                                size_t buffer_size,
                                int connect_id,
                                const char *topic,
                                int qos);
bool ML307C_MQTT_BuildPublish(char *buffer,
                              size_t buffer_size,
                              int connect_id,
                              const char *topic,
                              int qos,
                              const char *payload);

#endif /* NETWORK_ML307C_MQTT_H */
