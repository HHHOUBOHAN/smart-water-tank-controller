#ifndef COMMON_TEXT_PARSER_H
#define COMMON_TEXT_PARSER_H

#include <stdbool.h>
#include <stdint.h>

/* Lightweight integer parsing for embedded protocols. On success, cursor is
 * advanced to the first character after the parsed token. */
bool TextParser_ParseUInt32(const char **cursor, uint32_t *value);
bool TextParser_ParseInt32(const char **cursor, int32_t *value);
bool TextParser_ConsumeChar(const char **cursor, char expected);

#endif /* COMMON_TEXT_PARSER_H */
