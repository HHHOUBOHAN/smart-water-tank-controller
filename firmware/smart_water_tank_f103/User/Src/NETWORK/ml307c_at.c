#include "NETWORK/ml307c_at.h"

#include "COMMON/text_parser.h"

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
    int32_t value;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+CME ERROR:");
    if (position != 0)
    {
        position += strlen("+CME ERROR:");
        if (TextParser_ParseInt32(&position, &value))
        {
            return (int)value;
        }
    }
    return -1;
}

int ML307C_AT_ParseSignalQuality(const char *response)
{
    const char *position;
    int32_t signal;
    int32_t error_rate;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+CSQ:");
    if (position != 0)
    {
        position += strlen("+CSQ:");
        if (TextParser_ParseInt32(&position, &signal) &&
            TextParser_ConsumeChar(&position, ',') &&
            TextParser_ParseInt32(&position, &error_rate))
        {
            (void)error_rate;
            return (int)signal;
        }
    }
    return -1;
}

int ML307C_AT_ParseRegistration(const char *response)
{
    const char *position;
    int32_t first;
    int32_t status;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+CEREG:");
    if (position == 0)
    {
        position = strstr(response, "+CGREG:");
    }
    if (position == 0)
    {
        return -1;
    }
    position = strchr(position, ':');
    if (position == 0)
    {
        return -1;
    }
    position++;
    if (!TextParser_ParseInt32(&position, &first))
    {
        return -1;
    }
    if (TextParser_ConsumeChar(&position, ','))
    {
        return TextParser_ParseInt32(&position, &status) ? (int)status : -1;
    }
    return (int)first;
}
