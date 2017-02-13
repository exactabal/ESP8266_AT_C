#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <termios.h>


//#define DEBUG_ESP8266


#ifdef ANDROID
#define tcdrain(fd) ioctl(fd, TCSBRK, 1)
#endif


uint32_t getCurrentMS (){
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    return round(spec.tv_nsec / 1.0e6) + spec.tv_sec * 1000; // Convert nanoseconds to milliseconds
}

void delayMS(uint32_t ms){
    uint32_t start = getCurrentMS();
    while (getCurrentMS() - start < ms);
}


void espPrintln(int fd, char *buf, int len){
    //len+=2;
#ifdef DEBUG_ESP8266
    write(2, buf, len);
#endif
    write(fd, buf, len);
    tcdrain(fd);
}

int espRead(int __fd, void *__buf, size_t __nbytes){
   int num = read(__fd, __buf, __nbytes);
#ifdef DEBUG_ESP8266
   write(2, __buf, num);
#endif
   return num;
}
