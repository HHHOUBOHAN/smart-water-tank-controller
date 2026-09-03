#ifndef NETWORK_ML307C_AT_H
#define NETWORK_ML307C_AT_H

#include <stdbool.h>

bool ML307C_AT_HasError(const char *response);
bool ML307C_AT_IsOk(const char *response);
bool ML307C_AT_IsSimReady(const char *response);
bool ML307C_AT_IsAttached(const char *response);
int ML307C_AT_ParseCmeError(const char *response);
int ML307C_AT_ParseSignalQuality(const char *response);
int ML307C_AT_ParseRegistration(const char *response);

#endif /* NETWORK_ML307C_AT_H */
