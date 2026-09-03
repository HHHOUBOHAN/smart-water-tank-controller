#include "COMMON/text_parser.h"

#include <limits.h>

static const char *TextParser_SkipSpaces(const char *cursor)
{
    if (cursor == 0)
    {
        return 0;
    }
    while ((*cursor == ' ') || (*cursor == '\t') ||
           (*cursor == '\r') || (*cursor == '\n'))
    {
        cursor++;
    }
    return cursor;
}

static bool TextParser_ParseMagnitude(const char *cursor,
                                      uint32_t limit,
                                      uint32_t *value,
                                      const char **end)
{
    uint32_t parsed = 0U;
    uint32_t digit;
    bool has_digit = false;

    while ((*cursor >= '0') && (*cursor <= '9'))
    {
        digit = (uint32_t)(*cursor - '0');
        if (parsed > ((limit - digit) / 10U))
        {
            return false;
        }
        parsed = (parsed * 10U) + digit;
        has_digit = true;
        cursor++;
    }
    if (!has_digit)
    {
        return false;
    }
    *value = parsed;
    *end = cursor;
    return true;
}

bool TextParser_ParseUInt32(const char **cursor, uint32_t *value)
{
    const char *start;
    const char *end;
    uint32_t parsed;

    if ((cursor == 0) || (*cursor == 0) || (value == 0))
    {
        return false;
    }
    start = TextParser_SkipSpaces(*cursor);
    if (!TextParser_ParseMagnitude(start, UINT32_MAX, &parsed, &end))
    {
        return false;
    }
    *value = parsed;
    *cursor = end;
    return true;
}

bool TextParser_ParseInt32(const char **cursor, int32_t *value)
{
    const char *start;
    const char *end;
    uint32_t magnitude;
    uint32_t limit = INT32_MAX;
    bool negative = false;

    if ((cursor == 0) || (*cursor == 0) || (value == 0))
    {
        return false;
    }
    start = TextParser_SkipSpaces(*cursor);
    if ((*start == '-') || (*start == '+'))
    {
        negative = (*start == '-');
        start++;
    }
    if (negative)
    {
        limit = (uint32_t)INT32_MAX + 1U;
    }
    if (!TextParser_ParseMagnitude(start, limit, &magnitude, &end))
    {
        return false;
    }
    if (negative)
    {
        *value = (magnitude == ((uint32_t)INT32_MAX + 1U))
                     ? INT32_MIN
                     : -(int32_t)magnitude;
    }
    else
    {
        *value = (int32_t)magnitude;
    }
    *cursor = end;
    return true;
}

bool TextParser_ConsumeChar(const char **cursor, char expected)
{
    const char *position;

    if ((cursor == 0) || (*cursor == 0))
    {
        return false;
    }
    position = TextParser_SkipSpaces(*cursor);
    if (*position != expected)
    {
        return false;
    }
    *cursor = position + 1;
    return true;
}
