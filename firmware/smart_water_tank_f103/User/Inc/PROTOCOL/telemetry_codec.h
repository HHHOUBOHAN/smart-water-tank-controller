#ifndef PROTOCOL_TELEMETRY_CODEC_H
#define PROTOCOL_TELEMETRY_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TELEMETRY_STATUS_PART_COUNT  4U
#define TELEMETRY_CONFIG_PART_COUNT  2U

bool TelemetryCodec_EncodeStatusPart(char *buffer,
                                     size_t buffer_size,
                                     uint8_t part);
bool TelemetryCodec_EncodeConfigPart(char *buffer,
                                     size_t buffer_size,
                                     uint8_t part);
bool TelemetryCodec_EncodeAlarm(char *buffer, size_t buffer_size);
bool TelemetryCodec_EncodeAck(char *buffer,
                              size_t buffer_size,
                              uint32_t message_id,
                              const char *result,
                              const char *reason);

#endif /* PROTOCOL_TELEMETRY_CODEC_H */
