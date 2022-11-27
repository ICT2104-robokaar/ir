/*
 * -------------------------------------------
 *    MSP432 DriverLib - v3_21_00_05
 * -------------------------------------------

 * MSP432 Empty Project
 *
 * Description: A Barcode reader project that uses DriverLib
 *
 *                MSP432P401
 *             ------------------
 *         /|\|                  |
 *          | |                  |
 *          --|RST               |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 * Author:
*******************************************************************************/
#include "barcode.h"

int curr = 0;
int prev = 0;
int barCounter = 0;
int mulCount = 0;
int i = 0; // Temp can delete later

int timerValuesP1[10] = {0};
int multiplyCountP1[10] = {0};
int *resultCountP1;

int timerValuesP2[10] = {0};
int multiplyCountP2[10] = {0};
int *resultCountP2;

int timerValuesP3[10] = {0};
int multiplyCountP3[10] = {0};
int *resultCountP3;

bool nextIsBarcode = false;
bool inBarCode = false;


//char code[11];

/* Timer_A UpMode Configuration Parameter */
const Timer_A_UpModeConfig upConfig =
    {
        TIMER_A_CLOCKSOURCE_ACLK,           // ACLK Clock Source -> 32768
        TIMER_A_CLOCKSOURCE_DIVIDER_32,     // 32768/32 = 1024MHz
        TIMER_PERIOD,                              // 1024 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,     // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Disable CCR0 interrupt
        TIMER_A_DO_CLEAR                    // Clear value
};

// TimerA Frequency
// 32768/32 = 1024
//(1/1024)*32768 = 32s

//Function Definitions
int *calculateTimerValues(int *timerValues, int *multiplier);
void createCode(int *result, int thickbar, char *z);
bool checkOnes(char* code);
void getBarcode(char decoded1[4], char x[4]);
void matchCode(char *code, char *decoded, bool asterixMatch);


int main(void)
{
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    // Initialize variables
    curADCResult = 0;
    mulCount = 0;

    // Configuration/ Setup Timer
    MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &upConfig);
    Interrupt_enableInterrupt(INT_TA1_0);
    Interrupt_enableMaster();

    /* Setting Flash wait state */
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Setting DCO to 48MHz  */
    MAP_PCM_setPowerState(PCM_AM_LDO_VCORE1);
    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);

    /* Initializing ADC (MCLK/1/4) */
    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_4, 0);

    /* Configuring P1.0 as output */
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    /* Configuring GPIOs (5.5 A0) */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Configuring ADC Memory */
    MAP_ADC14_configureSingleSampleMode(ADC_MEM0, true);
    MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A0, false);

    /* Configuring Sample Timer */
    MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

    /* Enabling/Toggling Conversion */
    MAP_ADC14_enableConversion();
    MAP_ADC14_toggleConversionTrigger();

    /* Enabling interrupts */
    MAP_ADC14_enableInterrupt(ADC_INT0);
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableMaster();

    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);

    while (1)
    {
        MAP_PCM_gotoLPM0();
    }
}

void TA1_0_IRQHandler(void)
{
    mulCount++;
    printf("32 secs\n");
    MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

/* ADC Interrupt Handler. This handler is called whenever there is a conversion that is finished for ADC_MEM0. */
void ADC14_IRQHandler(void)
{
    bool threeOnes = false;
    bool isBlackBar = false;
    bool isWhiteBar = false;

    int thickBar = 0;

    char codeP1[30], codeP2[30], codeP3[30];

    uint64_t status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);

    if (ADC_INT0 & status)
    {
        curADCResult = MAP_ADC14_getResult(ADC_MEM0);

        /*Check if an transition occured*/
        prev = curr;

        /* If white surface is detected, turn on P1.0 */
        if (curADCResult < 8000)
        {
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
            isWhiteBar = true;
            isBlackBar = false;
            curr = 0;
        }
        else
        {
            /*If Black surface*/
            GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
            isBlackBar = true;
            isWhiteBar = false;
            curr = 1;
        }
//        printf("%d\n", curADCResult);
        MAP_ADC14_toggleConversionTrigger();
    }

    //    checking for first white bar(noise) before
    if (isWhiteBar && barCounter == 0)
    {
        nextIsBarcode = true;
    }

    // Checking if its the begining of the barcode
    if (isBlackBar && barCounter == 0 && nextIsBarcode)
    {
        // Indicator that currently still part of the barcode
        inBarCode = true;
    }

    if (inBarCode)
    {
        // Check if transition occurred transitions
        if (prev != curr)
        {
            // Increase count on transitions
            barCounter++;

            // Get length of Bars through with Timer

            // FIRST 10 BARS
            if (barCounter >= 1 && barCounter < 12)
            {
                if (isBlackBar && barCounter == 1)
                {
                    Timer_A_clearTimer(TIMER_A1_BASE);
                    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
                }
                else if (isBlackBar && barCounter == 11)
                {
                    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
                    timerValuesP1[barCounter - 2] = Timer_A_getCounterValue(TIMER_A1_BASE);
                    multiplyCountP1[barCounter - 2] = mulCount;
                }
                else
                {
                    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
                    timerValuesP1[barCounter - 2] = Timer_A_getCounterValue(TIMER_A1_BASE);
                    multiplyCountP1[barCounter - 2] = mulCount;
                    Timer_A_clearTimer(TIMER_A1_BASE);
                    i = Timer_A_getCounterValue(TIMER_A1_BASE);
                    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
                }
            }

            // Get length of each bar between the asterix
            if (barCounter >= 11 && barCounter < 22)
            {
                if (isBlackBar && barCounter == 11)
                {
                    Timer_A_clearTimer(TIMER_A1_BASE);
                    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
                }
                else if (isBlackBar && barCounter == 21)
                {
                    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
                    timerValuesP2[barCounter - 12] = Timer_A_getCounterValue(TIMER_A1_BASE);
                    multiplyCountP2[barCounter - 12] = mulCount;
                }
                else
                {
                    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
                    timerValuesP2[barCounter - 12] = Timer_A_getCounterValue(TIMER_A1_BASE);
                    multiplyCountP2[barCounter - 12] = mulCount;
                    Timer_A_clearTimer(TIMER_A1_BASE);
                    i = Timer_A_getCounterValue(TIMER_A1_BASE);
                    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
                }
            }

            // LAST 10 BARS
            if (barCounter >= 21 && barCounter < 32)
            {
                if (isBlackBar && barCounter == 21)
                {
                    Timer_A_clearTimer(TIMER_A1_BASE);
                    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
                }
                else if (isBlackBar && barCounter == 31)
                {
                    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
                    timerValuesP3[barCounter - 22] = Timer_A_getCounterValue(TIMER_A1_BASE);
                    multiplyCountP3[barCounter - 22] = mulCount;
                }
                else
                {
                    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
                    timerValuesP3[barCounter - 22] = Timer_A_getCounterValue(TIMER_A1_BASE);
                    multiplyCountP3[barCounter - 22] = mulCount;
                    Timer_A_clearTimer(TIMER_A1_BASE);
                    i = Timer_A_getCounterValue(TIMER_A1_BASE);
                    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
                }
            }
            mulCount = 0;
        }
        // Out of Barcode
        else if (barCounter >= 31)
        {
            barCounter = 0;
            inBarCode = false;
            nextIsBarcode = false;
            // Calculate timer values
//            int x, y, z;
//            for (x = 0; x < 11; x++)
//            {
//                if (multiplyCount[x] > 0)
//                {
//                    resultCount[x] = (TIMER_PERIOD * multiplyCount[x]) + timerValues[x];
//                }
//            }

//            resultCountP1 = calculateTimerValues(timerValuesP1, multiplyCountP1);
//            resultCountP2 = calculateTimerValues(timerValuesP2, multiplyCountP2);
//            resultCountP3 = calculateTimerValues(timerValuesP3, multiplyCountP3);

             //Checking if the first Thickbar is as bar 2 or 3 (For backward barcode)
            if (timerValuesP1[1] > timerValuesP1[2])
            {
                thickBar = 1;
            }
            else
            {
                thickBar = 2;
            }


            memset(codeP1, 0, sizeof(codeP1));
            memset(codeP2, 0, sizeof(codeP2));
            memset(codeP3, 0, sizeof(codeP3));

            // Convert Values to Code (1s and 0s)
            createCode(timerValuesP1, timerValuesP1[thickBar], codeP1);
            createCode(timerValuesP2, timerValuesP1[thickBar], codeP2);
            createCode(timerValuesP3, timerValuesP1[thickBar], codeP3);
//            printf("decodeP1: %s\n", codeP1);
//            printf("decodeP2: %s\n", codeP2);
//            printf("decodeP3: %s\n", codeP3);
//
//            // check no. of 1s
//            threeOnes = checkOnes(codeP1);
//            threeOnes = checkOnes(codeP2);
//            threeOnes = checkOnes(codeP3);

            //match code
//            if(threeOnes == true){
                //match Code
                char a[10] = {0}, b[10] = {0};
                char c[10] = {0};

                bool asterixMatch = false;
                int result = strcmp(codeP1, "0010100100");
                if (result == 0)
                {
                    printf("Barcode is backwards\n");
                    asterixMatch = true;
                }

                matchCode(codeP1, a, asterixMatch);
                matchCode(codeP2, b, asterixMatch);
                matchCode(codeP3, c, asterixMatch);

//                getBarcode(codeP1, a);
//                getBarcode(codeP2, b);
//                getBarcode(codeP3, c);
                printf("getbarcode %s%s%s\n", a,b,c);
//            }
//            else{
                //no need to match
//                printf("More than 3 1s -> Forget about matching\n");
//            }
        }
        printf("Counter: %d\n", barCounter);
    }
}

int *calculateTimerValues(int timerValues[], int multiplier[])
{
    int static result[9];

//    for (int i = 0; i < 10; i++){
//        if (multiplier[i] > 0)
//        {
//            result[i] = (TIMER_PERIOD * multiplier[i]) + timerValues[i];
//        }
//    }
    return result;
}

void createCode(int timer[], int thickbar, char* z){
    char code[10] = {0};
    int y;
    for(y = 0; y < 10; y++)
    {
        if (timer[y] >= thickbar)
        {
//            code[i] = '1';
            strcat(code, "1");
        }
        else
        {
//            code[i] = '0';
            strcat(code, "0");
        }

      }
//      strcpy(z, code);
//        printf("CODE !!! ->  %s\n", code);
        strcat(z, code);
//        printf("Z !!! ->  %s\n", z);
}

bool checkOnes(char code[]){
    int x, onesCount = 0;
    for(x = 0; x < 10; x++){
        if(code[x] == '1'){
            onesCount++;
        }
    }
    if(onesCount <= 3){
        printf("3 !!\n");
        return true;
    }
    else{
        printf("NOPE !!\n");
        return false;
    }
}

//Function that return the 
void getBarcode(char decoded1[4], char x[4]){
    strcpy(x, decoded1);
}

//Function to decode the barcode
void matchCode(char *code, char *decoded, bool asterixMatch){
    char *decode[] = {"A", "1000010010", "B", "0010010010", "C", "1010010000", "D", "0000110010",
                      "E", "1000110000", "F", "0010110000", "G", "0000011010", "H", "1000011000",
                      "I", "0010011000", "J", "0000111000", "K", "1000000110", "L", "0010000110",
                      "M", "1010000100", "N", "0000100110", "O", "1000100100", "P", "0010100100",
                      "Q", "0000001110", "R", "1000001100", "S", "0010001100", "T", "0000101100",
                      "U", "1100000010", "V", "0110000010", "W", "1110000000", "X", "0100100010",
                      "Y", "1100100000", "Z", "0110100000", "*", "0100101000"};

    char *backwardDecode[] = {"A", "1001000010", "B", "1001001000", "C", "0001001010", "D", "1001100000",
                              "E", "0001100010", "F", "0001101000", "G", "1011000000", "H", "0011000010",
                              "I", "0011001000", "J", "0011100000", "K", "1100000010", "L", "1100001000",
                              "M", "0100001010", "N", "1100100000", "O", "0100100010", "P", "0100101000",
                              "Q", "1110000000", "R", "0110000010", "S", "0110001000", "T", "0110100000",
                              "U", "1000000110", "V", "1000001100", "W", "0000001110", "X", "1000100100",
                              "Y", "0000100110", "Z", "0000101100", "*", "0010100100"};

    bool matchFound = false;
    int i;
    // match code
    if (asterixMatch)
    { // Means is backwards
        i = 0;
        while (matchFound == false)
        {
            if (i % 2 == 1)
            {
                int res = strcmp(code, backwardDecode[i]);
                if (res == 0)
                {
                    printf("(backwards) Decoded message: %s\n", backwardDecode[i - 1]);
                    strcpy(decoded, backwardDecode[i - 1]);
                    matchFound = true;
                }
                if (i > 54)
                {
                    printf("No Match! (backwards) \n");
                    matchFound = true;
                }
            }
            i++;
        }
    }
    else
    {
        i = 0;
        while (matchFound == false)
        {
            // This is for the array of strings as all the coded messages are on odd values
            if (i % 2 == 1)
            {
                int res = strcmp(code, decode[i]);
                if (res == 0)
                {
                    printf("Decoded message: %s\n", decode[i - 1]);
                    strcpy(decoded, decode[i - 1]);
                    matchFound = true;
                }
                if (i > 54)
                {
                    printf("No Match !!\n");
                    matchFound = true;
                }
            }
            i++;
        }
        matchFound = false;
    }
}
