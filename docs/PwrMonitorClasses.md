## Classes

#### PZENG eng

```c++
typedef struct eng_t
{
    float volts;        	// as read by the PZEM meter
    float amps;
    float watts;
    float kWh;       
} VAP;

void setSimulV( float V );  // specifies simulation voltage
void setSimulW( float W );  // specifies simulation power
VAP getSimul();             // returns simulation voltage & power

VAP readPhase( int ph,      // ph can be 1, 2 or 1+2. Reads PZEM or returns
              float scale,
              bool simul ); // ..simulation settings

VAP getPhase( int ph );     // ph can be 1, 2 or 1+2. Returns previous PZEM
							// or simulation settings
void zerokWh();
int getError( int ph );     // returns error code as set by readPhase()
const char *getErrorStr( int ph );  // interpretation of the error
```

#### VWSTATS shortT, longT

```c++
typedef struct stats_t
{
    float Vmin;        
    float Vavg;
    float Wavg;
    float Wmax;
    float kWh;               
} STATS;

void updateStats( VAP *n );         // call periodically
STATS getStats();                   // get current stats
const char *getVStats();            // get stats string
const char *getWStats();            // get stats string
void resetStats();
```

#### TMBASE tmb

```c++
void setHMS( int h=0, int m=0, int s=0 );     // sets time
uint32 getSecCount();
void update();                                // hook in Ticker every second
const char *getTime();
```
