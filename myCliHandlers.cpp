/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <externIO.h>               // IO includes and cpu...exe extern declarations
#include <mgnClass.h>

#include "myGlobals.h"
#include "mySupport.h"
#include "myEngClasses.h"           // this also includes pz16Class.h
#include "myCliHandlers.h"

static MGN mgn;                     // temp usage of Meguno structure

// ----------------------- 1. CLI DIRECT PZM Access -----------------------------------

void requireOFF( BUF *bp )                           // helper function
{
    RESPOND( bp, "Direct access to PZEM requires 'scanON' 0\r\n");
}
static void cliPZSetTrace( int n, char *arg[])
{
    BINIT( bp, arg );
    int tr = (n > 1)? atoi( arg[1] ) : 0;
    pz.setTrace( tr );
    RESPOND( bp, "trace = %d\r\n", tr );
}
static void cliPZFlashSlave( int n, char *arg[])
{
    BINIT( bp, arg );
    int fls = (n > 1)? atoi( arg[1] ) : 1;
    pz.flashSlaveAddr( fls );
    RESPOND( bp, "slave addr = %d flashed\r\n", fls );
}
static void cliPZMeasure( int n, char *arg[])
{
    BINIT( bp, arg );
    if( myp.gp.scanON ){ requireOFF( bp ); return; }

    int my_slave = (n > 1)? atoi( arg[1] ) : 1;
    pz.measure( my_slave );
    RESPOND( bp,  "Ph#%d %.1fV %.2fA %.0fW %.3fkWh %.1fHz %.3fPF\r\n", 
        my_slave, pz.getVolts(), pz.getAmps(), pz.getWatts(), pz.getkWh(), pz.getHertz(), pz.getPF()  );
}
static void cliPZGetVolts( int n, char *arg[])
{
    BINIT( bp, arg );
    if( myp.gp.scanON ){ requireOFF( bp ); return; }

    int my_slave = (n > 1)? atoi( arg[1] ) : 1;
    pz.measure( my_slave, 1 );
    RESPOND( bp, "Err:%d %.1fV\r\n", pz.getError(), pz.getVolts() );
}
static void cliPZGetVoltsAmps( int n, char *arg[])
{
    BINIT( bp, arg );
    if( myp.gp.scanON ){ requireOFF( bp ); return; }

    int my_slave = (n > 1)? atoi( arg[1] ) : 1;
    pz.measure( my_slave, 3 );
    RESPOND( bp, "Err:%d %.1fV %.3fA\r\n", pz.getError(), pz.getVolts(), pz.getAmps() );
}
static void cliPZResetkWh( int n, char *arg[])      // scanON must be OFF
{
    BINIT( bp, arg );
    if( myp.gp.scanON ){ requireOFF( bp ); return; }

    int my_slave = (n > 1)? atoi( arg[1] ) : 1;
    pz.resetkWh( my_slave );
    RESPOND( bp, "OK\r\n");
}
// ----------------------- 2. CLI SIMULATION -----------------------------------

static void cliSetSimulV( int n, char *arg[] )
{
    BINIT( bp, arg );

    myp.simValue.volts = (n>1) ? atof( arg[1] ) : 90.0;
    myp.simValue.amps  = myp.simValue.volts > 0.0 ? myp.simValue.watts/myp.simValue.volts : 0.0;
    RESPOND( bp,  "Set to %.1fV (%.2fA per phase)\r\n", myp.simValue.volts, myp.simValue.amps );
}
static void cliSetSimulW( int n, char *arg[] )
{
    BINIT( bp, arg );

    myp.simValue.watts = (n>1) ? atof( arg[1] ) : 0.0;
    myp.simValue.amps  = myp.simValue.volts > 0.0 ? myp.simValue.watts/myp.simValue.volts : 0.0;
    RESPOND( bp,  "Set to %.1fW total (%.2fA per phase)\r\n", myp.simValue.watts, myp.simValue.amps );
}
static void cliZerokWh( int n, char *arg[] )
{
    BINIT( bp, arg );
    eng.zerokWh();
    RESPOND( bp,  "OK\r\n" );
}
// --------------------- 3. Global volatile VARIABLES -------------------------

static void cliSetTime( int n, char *arg[])
{
    BINIT( bp, arg );
    int tmin, thrs, tsec;
    thrs = tmin = tsec = 0;
    
    if( n>1 ) thrs = atoi( arg[1] );
    if( n>2 ) tmin = atoi( arg[2] );
    if( n>3 ) tsec = atoi( arg[3] );
    
    tmb.setHMS( thrs, tmin, tsec );
    ticMin.setSecCount( tmb.getSecCount() );
    ticHr.setSecCount( tmb.getSecCount() );
    RESPOND( bp, "Time is: %s", tmb.getTime() ); 
}
static void cliSetModulo( int n, char *arg[])
{
    BINIT( bp, arg );
    
    ticMin.setModulo( n>1 ? atoi(arg[1]):2 );
    ticMin.setSecCount( tmb.getSecCount() );
    
    RESPOND( bp,  "Time is: %s", tmb.getTime() );
}
// --------------------- 4. Measurements & Actions -------------------------

static void cliMeasure( int n, char *arg[] )
{
    if( myp.gp.scanON )                 // if scanON, myp.vap is already read
    {
        reportVAP( exe.cmd1stUpper() ? STREAM_ALL : STREAM_CONSOLE );       
    }
    else                                // if scan is OFF, take a measurement and display
    {
        for( myp.phase = 1; myp.phase <=2; myp.phase++ )
        {
            int i = myp.phase;            
            if( isSimulON() )
            {
                readSimulPhase( &myp.vap[i], i );
                myp.vap[0] = getSimulPhase( 1+2 );
            }
            else
            {
                eng.readPhase( &myp.vap[i], i, myp.gp.ampscale );
                myp.vap[0]  = eng.getPhase( 1+2 );
            }
            reportVAP( exe.cmd1stUpper() ? STREAM_ALL : STREAM_CONSOLE );
        }
    }
}
static void cliResetkWh( int n, char *arg[])
{
    BINIT( bp, arg );
    eng.zerokWh();
    if( exe.cmd1stUpper() )
        mgn.controlSetText( "dkWh1","0.000kWh" );
    else
        RESPOND( bp, "OK\r\n" );
}

static void cliShowState( int n, char *arg[])
{
    BINIT( bp, arg );
    refreshState( exe.cmd1stUpper() ? STREAM_MEGUNO : STREAM_CONSOLE, bp ); // in mySupport.cpp
}
static void cliThisHelp( int n, char *arg[])
{
    BINIT( bp, arg );
    CMDTABLE *row = mypTable;
    for( int j=0; row->cmd ; j++, row++ )
    {
        RESPOND( bp,  "\t%s\t%s\r\n", row->cmd, row->help );
    }
}
// ----------------------- 5. SetEE IMPLEMENTATION -----------------------------------

void cliSetMyParm( int n, char **arg )
{
    BINIT( bp, arg );
    if( exe.missingArgs( 2 ) )
        return;
    if( exe.cmd1stUpper() )
        setAnyMegParm( arg[1], arg[2], bp );
    else
        setAnyEEParm ( arg[1], arg[2], bp );
}  

// ----------------- CLI command table ----------------------------------------------

/* To program a new PZEM-016, use "flash addr". Test using "v addr".
 */
    CMDTABLE mypTable[]= 
    {
        {"t",         "This help",                                  cliThisHelp },    

// 1. PZ Direct Access        
        {"pztrace", "n. Set trace of MODBUS",                       cliPZSetTrace},
        {"pzflash", "addr. Flash new PZEM slave address",           cliPZFlashSlave},
        {"pzmeas",  "[slave]. PZEM Measure",                        cliPZMeasure},        
        {"pzv",     "[slave]. PZEM Volts",                          cliPZGetVolts},
        {"pzva",    "[slave]. PZEM Volts and Amps",                 cliPZGetVoltsAmps},
        {"pzreset", "[slave]. Resets PZEM kWh",                     cliPZResetkWh},

// 2. Simulation        
        {"setSimV","volts. Set simulation V",                       cliSetSimulV},
        {"setSimW","watts. Set simulation W",                       cliSetSimulW},

// 3. Volatile Variables       
        {"setTime", "hrs min sec. Set current time",                cliSetTime},
        //{"setMod",  "sec. Set modulo period",                     cliSetModulo},
        
// 4. High Level Measurements/Actions

        {"measure", "[or Measure]. Read Ph#1, Ph#2, Ph#12",         cliMeasure},
        {"reset",   "[or Reset]. Zero all kWh",                     cliResetkWh},

// 5. Sets a user EE Parameter
        {"ShowAll", "Clears and redraws Meguno Parm Table",   [](int, char**){showMegAllParms();} },
        {"state",   "[or State]. Show state",                       cliShowState },
        {"setEE",   "[or SetEE] parm value. Sets any parameter",    cliSetMyParm},

        {NULL, NULL, NULL}
    };
/*
Voltage or Watts slider -> sets SimulV or Watts
Display 1=text, 2=meguno, 3=both (LED is always ON)
Stream ON/OFF
READ --> flips Stream OFF reads once
SIMUL --> flips Stream OFF reads once
*/


 #undef RESPONSE
 
