// clear version apart from the main
#ifndef TIMEPARSER_H
#define TIMEPARSER_H

// Error codes
#define TIME_LEN_ERROR        -1
#define TIME_ARRAY_ERROR      -2
#define TIME_VALUE_ERROR      -3
#define TIME_CHAR_ERROR       -4
#define TIME_ZERO_ERROR       -5
#define SEQUENCE_ERROR_NULL   -10
#define SEQUENCE_ERROR_EMPTY  -11
#define SEQUENCE_ERROR_CHAR   -12

int sequence_parse(char *sequence);

int time_parse(char *time);

#endif
