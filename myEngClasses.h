#pragma once

#include <ESP8266WiFi.h>
#include "myGlobals.h"
#include <pz16Class.h>

/* ----------------------------------- PZMEng Class -------------------------------------
 *  Handles all reading of measurements from PZEM.
 *  
 *  Use .readPhase( ph, simul ) to read phase 1, 2 or 1+3. If simul is TRUE, 
 *                              then it sets the simulation values.
 *  Use .getPhase( ph ) to obtain the measurements as read by .readPhase().
 *                      They are either PZEM measurements or simulations                             
 *                              
 *  Use .setSimulV() or .setSimulW() to set simulation voltage and power.
 *  You can inspect the simulation settings using .getSimul()
 *  
 *  Use .zerokWh() to reset the kWh() of both phases
 *  .getkWh() always returns the relative consumptions to the reference.
 *  
 *  Use .getError() to inspect the error of the previous reading
 *  -------------------------------------------------------------------------------------
 */
        
    class PZENG      
    {
    private:
        VAP ph[3];                  // actual measurement of Phase 1 or 2. 0 is 1+2
        VAP sim;                    // settings to be used in simulation
        int myerror[3];             // set by read(). Set to 0 if simulation
        char errstr[3][12];         // error strings
        
        float refkWh[3];            // ref kWh for phase1 or phase2
        bool  doref[3];             // flag to force new reference. 
                                    // Set by .zerokWh(), used by .readPhase()
        
        void readPZM( int i,        // Direct read from PZM. i can be 1 or 2. Not 0
                    float scale );  // Sets error flag. Adjust readings with scale
        void addPhase12();

        int getError( int ph );     // returns error code as set by readPhase()
        const char *getErrorStr( int ph );  // interpretation of the error

    public:
        PZENG();
                
        void readPhase( VAP *vp,
                        int ph,      // ph can be 1, 2 or 1+2. Reads PZEM or returns
                        float scale );
                                    
        VAP getPhase( int ph );     // ph can be 1, 2 or 1+2. Returns previous PZEM
                                    // or simulation settings
        void zerokWh();
    };    

// ----------------------------------- PZMStats Class -------------------------------------
    typedef struct stats_t
    {
        float Vmin;        
        float Vavg;
        float Wavg;
        float Wmax;
        float kWh;               
    } STATS;
    
    class VWSTATS
    {
    private:
        STATS stat; 
        int count;
        float Vsum;
        float Wsum;   
    public:
        VWSTATS();
        void updateStats( VAP *n );         // call periodically
        STATS getStats();                   // get current stats
        const char *getVStats();            // get stats string
        const char *getWStats();            // get stats string
        void resetStats();
    };

// ----------------------------------- Alarm Class -------------------------------------
    #define DISABLE_ALARM -1.0
    
    class ALARM
    {
    private:
        int stateV;             // 0=no alarms, 1=alarm detected
        bool alarmV;            // true = in alarm state
        int triggerV;           // 1=alarm just detected; 2=alarm just removed; 0=no alarms
        char alarmVT[16];
        uint32 alarmVT0;        // number of seconds timestamp
        uint32 alarmVDT;
        
        int stateW;   // 0=no alarms, 1=1st violation, 2=in violation, 3=already reported
        bool alarmW;
        int triggerW;           // 1=alarm just detected; 2=alarm just removed; 0=no alarm
        char alarmWT[16];
        uint32 alarmWT0;      // number of seconds timestamp
        uint32 alarmWDT;

        float Vthrs;        // voltage alarm threshold
        float Wthrs;        // power alarm threshold

    public:
        ALARM();
        void setUVthrs( float value = DISABLE_ALARM );   
        void setOPthrs( float value = DISABLE_ALARM );   
        
        void updateAlarms( VAP *n );        // call periodically
        
        int triggeredUV();                  // see triggerV
        int triggeredOP();                  // see triggerV
        
        const char *getAlarms();
        const char *getUVAlarm();
        const char *getOPAlarm();
        void resetUVAlarm();
        void resetOPAlarm();
    };
// ----------------------------------- Timer Class -------------------------------------

class TMBASE
{
private:
    int _sec;           // 0,10,...50
    int _min;           // 0, 1, ..., 59
    int _hrs;           // 0, 1, ..., 23
        
public:
    TMBASE();
    void setHMS( int h=0, int m=0, int s=0 );     // sets time
    uint32 getSecCount();
    void update();                                // hook in Ticker every second
    const char *getTime();
};
// ---------------------------------
class TOCsec
{
private:
public:
    uint32 _secmax;                 // upper limit before warps to 0 
    uint32 _seccnt;                 // counts 0...secmax-1
    bool _secrdy;                   // true if warp
public:
    TOCsec( uint32 period = 1);     // can be used to set timeout
    void update();                  // call this every "sec"    
    void setModulo( uint32 period );  // optional call 
    void setSecCount( uint32 sec );
    bool ready();                   // ready every SEC_COUNT
};

// ----------------------------------- External References ---------------------------------
    extern CPU cpu;
    extern OLED oled;
    
    extern PZENG   eng;    
    extern VWSTATS shortT;      // short time averages
    extern VWSTATS longT;       // long term averages    
    extern ALARM alm;           // alarms
    
    extern PZ16 pz;             // allocation of the PZEM-016 Class
    extern TOCsec ticMin;       // every hour
    extern TOCsec ticHr;        // every day
    extern TMBASE tmb;          // every second
