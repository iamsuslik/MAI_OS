#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT float area(float a, float b) {
    return (a * b) / 2.0f;
}

EXPORT char* convert(int x) {
    if (x == 0) {
        char* result = (char*)malloc(2 * sizeof(char));
        if (result) {
            result[0] = '0';
            result[1] = '\0';
        }
        return result;
    }

    int temp = x;
    int length = 0;
    while (temp > 0) {
        temp /= 3;
        length++;
    }

    char* result = (char*)malloc((length + 1) * sizeof(char));
    if (!result) return NULL;

    result[length] = '\0';
    temp = x;
    for (int i = length - 1; i >= 0; i--) {
        int remainder = temp % 3;
        result[i] = '0' + remainder;
        temp /= 3;
    }

    return result;
}
