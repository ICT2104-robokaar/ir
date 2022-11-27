#ifndef BARCODE_H_
    #define BARCODE_H_

    #ifndef DRIVERLIB_H
        #define DRIVERLIB_H
        #include "driverlib.h"
    #endif

    #define TIMER_PERIOD 65535

    /* Standard Includes */
    #include <stdint.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdio.h>

    /* Statics */
    static volatile uint16_t curADCResult;
    extern int curr;
    extern int prev;
    extern int barCounter;
    extern int i;
    extern int mulCount;

    extern bool inBarCode;

    extern int timerValuesP1[10];
    extern int multiplyCountP1[10];
    extern int *resultCountP1;

    extern int timerValuesP2[10];
    extern int multiplyCountP2[10];
    extern int *resultCountP2;

    extern int timerValuesP3[10];
    extern int multiplyCountP3[10];
    extern int *resultCountP3;


    int Infraredmain(void);
    void TA1_0_IRQHandler(void);
    void ADC14_IRQHandler(void);
    int *calculateTimerValues(int *timerValues, int *multiplier);
    void createCode(int *result, int thickbar, char *z);
    bool checkOnes(char* code);
    void getBarcode(char decoded1[4], char x[4]);
    void matchCode(char *code, char *decoded, bool asterixMatch);
#endif
