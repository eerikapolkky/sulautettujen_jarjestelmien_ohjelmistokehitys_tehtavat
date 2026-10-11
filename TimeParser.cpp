// clear version
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {

    // Check that string is not null
    if (time == NULL) {
        return TIME_ARRAY_ERROR;
    }

    // Check that string length is exactly 6 characters
    if (strlen(time) != 6) {
        return TIME_LEN_ERROR;
    }

    // Check that all characters are numbers
    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            return TIME_CHAR_ERROR;
        }
    }

    // Parse values from time string
    int values[3];

    values[2] = atoi(time + 4); // seconds
    time[4] = 0;

    values[1] = atoi(time + 2); // minutes
    time[2] = 0;

    values[0] = atoi(time); // hours

    // Boundary checks
    if (values[0] < 0 || values[0] > 23) {
        return TIME_VALUE_ERROR;
    }

    if (values[1] < 0 || values[1] > 59) {
        return TIME_VALUE_ERROR;
    }

    if (values[2] < 0 || values[2] > 59) {
        return TIME_VALUE_ERROR;
    }

    // Calculate minutes + seconds
    int seconds = values[1] * 60 + values[2];

    // Timer with 0 seconds would be pointless
    if (seconds == 0) {
        return TIME_ZERO_ERROR;
    }

    return seconds;
}
int sequence_parse(char *sequence) {

    if (sequence == NULL) {
        return SEQUENCE_ERROR_NULL;
    }

    if (strlen(sequence) == 0) {
        return SEQUENCE_ERROR_EMPTY;
    }

    for (int i = 0; sequence[i] != '\0'; i++) {

        if (sequence[i] != 'R' &&
            sequence[i] != 'Y' &&
            sequence[i] != 'G') {

            return SEQUENCE_ERROR_CHAR;
        }
    }

    return 0;
}
