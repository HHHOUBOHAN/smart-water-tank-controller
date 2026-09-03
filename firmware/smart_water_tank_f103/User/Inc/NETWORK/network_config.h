#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

/* ML307C + EMQX integration parameters confirmed by the successful hardware
 * test. Plaintext credentials are acceptable only for this prototype. */
#define MQTT_HOST              "81.70.187.191"
#define MQTT_PORT              "1883"
#define MQTT_CLIENT_ID         "water_tank_001"
#define MQTT_USERNAME          "water_tank_001"
#define MQTT_PASSWORD          "water_tank_001"

#define MQTT_TOPIC_STATUS      "water_tank/water_tank_001/status"
#define MQTT_TOPIC_ALARM       "water_tank/water_tank_001/alarm"
#define MQTT_TOPIC_ACK         "water_tank/water_tank_001/ack"
#define MQTT_TOPIC_COMMAND     "water_tank/water_tank_001/command"
#define MQTT_TOPIC_CONFIG      "water_tank/water_tank_001/config"

#endif /* NETWORK_CONFIG_H */
