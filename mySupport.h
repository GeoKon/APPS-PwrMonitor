#pragma once
#include <ticClass.h>
#include "myGlobals.h"          // needed for VAP definition (only)

#define STREAM_CONSOLE 1
#define STREAM_MEGUNO  2
#define STREAM_ALL     3
#define STREAM_NONE    0

    extern TICsec secTic; 

    void reportVAP( int mask );     // reports streaming values
    
    bool setAnyMegParm( char *parm, char *value, BUF *bp );
    bool setAnyEEParm ( char *parm, char *value, BUF *bp );
    
    int isSimulON();                // true if simulation is ON
    int isNotifyON();               // true if notifications are ON
    
    char *getAllState( BUF *s );
    void showMegAllParms();
    
    // keep consistency with PZ ENG
    void readSimulPhase( VAP *vp, int phase );
    VAP getSimulPhase( int phase );
    
    void refreshState( int mask, BUF *bp );
