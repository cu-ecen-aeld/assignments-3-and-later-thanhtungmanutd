#include <stdio.h>
#include "string.h"
#include <syslog.h>

int main(int argc, void **args) {
    openlog(NULL, 0, LOG_USER);
    if (argc != 3) {
        syslog(LOG_ERR, "Number of input arguments should be equal to 02");
        return 1;
    } else {
        FILE *ptr = fopen(args[1], "w");
        fwrite((char *)args[2], sizeof(char), strlen((char *)args[2]), ptr);
        fclose(ptr);
        return 0;
    }
    return 0;
}