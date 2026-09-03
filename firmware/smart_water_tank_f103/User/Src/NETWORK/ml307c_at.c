#include "NETWORK/ml307c_at.h"

#include <stdio.h>
#include <string.h>

bool ML307C_AT_HasError(const char *response)
{
    if (response == 0)
    {
        return true;
    }
    return (strstr(response, "+CME ERROR:") != 0) ||
           (strstr(response, "ERROR") != 0) ||
           (strstr(response, "FAIL") != 0);
}

bool ML307C_AT_IsOk(const char *response)
{
    return (response != 0) &&
           (strstr(response, "OK\r\n") != 0) &&
           !ML307C_AT_HasError(response);
}

bool ML307C_AT_IsSimReady(const char *response)
{
    return (response != 0) &&
           (strstr(response, "+CPIN: READY") != 0);
}

bool ML307C_AT_IsAttached(const char *response)
{
    return (response != 0) &&
           (strstr(response, "+CGATT: 1") != 0);
}

int ML307C_AT_ParseCmeError(const char *response)
{
    const char *position;
    int value;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+CME ERROR:");
    if ((position != 0) &&
        (sscanf(position, "+CME ERROR: %d", &value) == 1))
    {
        return value;
    }
    return -1;
}

int ML307C_AT_ParseSignalQuality(const char *response)
{
    const char *position;
    int signal;
    int error_rate;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+CSQ:");
    if ((position != 0) &&
        (sscanf(position, "+CSQ: %d,%d", &signal, &error_rate) == 2))
    {
        return signal;
    }
    return -1;
}

int ML307C_AT_ParseRegistration(const char *response)
{
    const char *position;
    const char *format_with_mode;
    const char *format_status_only;
    int mode;
    int status;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+CEREG:");
    format_with_mode = "+CEREG: %d,%d";
    format_status_only = "+CEREG: %d";
    if (position == 0)
    {
        position = strstr(response, "+CGREG:");
        format_with_mode = "+CGREG: %d,%d";
        format_status_only = "+CGREG: %d";
    }
    if (position == 0)
    {
        return -1;
    }
    if (sscanf(position, format_with_mode, &mode, &status) == 2)
    {
        return status;
    }
    if (sscanf(position, format_status_only, &status) == 1)
    {
        return status;
    }
    return -1;
}
