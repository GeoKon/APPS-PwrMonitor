/*
 * Used by:
 *      PwrMonitor      -- to report to console when streaming is ON
 *      myCliHandlers   -- to set parameters
 *      myThinger       -- to set parameters
 * 
 * Depends on:
 *      myEngClasses
 *      myGlobals
 */

#include <mgnClass.h>
#include <oledClass.h>

#include <SimpleSTA.h>      // needed for devName()

#include "mySupport.h"
#include "myGlobals.h"
#include "myEngClasses.h"

// ----------------------------- CLASS ALLOCATIONS ---------------------------------
    
static MGN mgn("CONFIG");                  // used for this module
TICsec secTic( 2 );                        // every second. Used by main loop() but modified here

/* --------------- Used by main loop() to show data if scan is on ------------------
 *  See header for mask values
 *  --------------------------------------------------------------------------------
 */
void reportVAP( int mask )                      // reports whatever the current values are
{
    const char *format = "%s: %5.1fV, %5.2fA, %4.0fW, %5.3fkWh";
    
    if( mask & STREAM_CONSOLE )                 // report to console
    {
        PF( format, myp.vap[myp.phase].error, 
                    myp.vap[myp.phase].volts, 
                    myp.vap[myp.phase].amps, 
                    myp.vap[myp.phase].watts, 
                    myp.vap[myp.phase].kWh );
        
        PF( ", %s, %s, %s\r\n", 
            shortT.getVStats(), 
            shortT.getWStats(),
            alm.getAlarms());
    }
    if( mask & STREAM_MEGUNO )
    {
        B80( s );
        
        if( myp.phase == 1 )
        {
            s.set( format,  myp.vap[1].error, 
                            myp.vap[1].volts, 
                            myp.vap[1].amps, 
                            myp.vap[1].watts, 
                            myp.vap[1].kWh );
            mgn.controlSetText( "dPhase1", s.c_str() );
        }
        if( myp.phase == 2 )
        {
            s.set( format,  myp.vap[2].error, 
                            myp.vap[2].volts, 
                            myp.vap[2].amps, 
                            myp.vap[2].watts, 
                            myp.vap[2].kWh );
            mgn.controlSetText( "dPhase2", s.c_str() );

            mgn.controlSetValue( "mVolts", myp.vap[0].volts  );
            mgn.controlSetValue( "mAmps",  myp.vap[0].amps  );
        }
    }
    
    // --------------- always display OLED ------------------------------
    
    static int dspmode = 0;             // 0 = first time ever
                                        // 1 = run mode, 2 = parm mode
    if( !cpu.button() )
    {
        if( dspmode!=1 )
        {
            oled.dsp( O_CLEAR );
            dspmode = 1;
        }
        oled.dsp( 0, "\t\a%s",              myp.vap[myp.phase].error );
        oled.dsp( 2, "\t\a%.1fV %.2fA",     myp.vap[myp.phase].volts, myp.vap[myp.phase].amps );
        oled.dsp( 4, "\t\b%.0fW",           myp.vap[0].watts );
        oled.dsp( 7, "\t\a%.2fkWh",         myp.vap[0].kWh );
    }
    else
    {
        if( dspmode != 2 )
        {
            oled.dsp( O_CLEAR );
            dspmode = 2;
        }
        oled.dsp( 0, "\t%s", getDevName(NULL) );
        IPAddress ip=WiFi.localIP();
        oled.dsp( 1, "\t%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
        oled.dsp( 2, "\tScan %s", myp.gp.scanON ? "ON" : "OFF" );
        oled.dsp( 3, "\tScanT %dsec", myp.gp.scansec );
        oled.dsp( 4, "\tStream %s", myp.streamON ? "ON" : "OFF" );
        oled.dsp( 5, "\tDisplay %d", myp.gp.display );
        oled.dsp( 7, "\t%s", tmb.getTime() );
    }
}
// Returns !=0 is one of the simulations has been activated
    int isSimulON()
    {       
        int type = 0;
        if( myp.simValue.volts > 90.1 )
            type |= 1;
        if( myp.simValue.watts > 0.0 )
            type |= 2;
        return type;
    }

// Returns !=0 if one of the notifications has been activated
    int isNotifyON()
    {
        int notify = 0;
        if( myp.gp.notifyV > 90.1 )
            notify |= 1;
        if( myp.gp.notifykW > 0.1 )
            notify |= 2;
        return notify;
    }

    // This routine is called if ANY PARAMETER has changed
                                                       
    #define IF(A) if( !strcasecmp( A, parm ) )
    static void doParmFunc( char *parm, char *value )
    {
        // Here, parameter has been updated. Save in EEPROM, update Meguno, and 
        //  selectively call initialization functions.
                 
        PFN( "Calling function associated to 'set %s %s'", parm, value );

        IF( "scanON" )
            mgn.controlSetCheck( "cScan", atoi(value) );      // update check box
        IF( "scanSec" )
            secTic.setSecCount( atoi( value ) );
        IF( "notifyV" )
            alm.setUVthrs( myp.gp.notifyV > 90.1 ? myp.gp.notifyV  : -1.0 );
        IF( "notifykW" )
        {
            float v = myp.gp.notifykW > 0.1 ? myp.gp.notifykW*1000.0 : -1.0;
            //PFN("OP thrs set to %.2f", v );
            alm.setOPthrs( v );
        }
        IF( "obright" )
        {
            oled.dsp( O_LED130 );
            oled.dsp( BRIGHT(myp.gp.obright) );
        }
    }    
    #undef IF

    bool setAnyEEParm( char *parm, char *value, BUF *bp )
    {        
        if( !nmp.setParmByStr( parm, value ) )          // if EEPROM User parm not found
        {
            RESPOND( bp, "%s not found\r\n", parm );
            return false;
        }        
        // Here, parameter has been updated. Save in EEPROM, update Meguno, and 
        //  selectively call initialization functions.

        myp.saveMyEEParms();                            // save to EEPROM
        doParmFunc( parm, value );                      // perform function associated with this parameter
        RESPOND( bp, "User parm %s updated\r\n", parm ); 
        return true;
    }   
    bool setAnyMegParm( char *parm, char *value, BUF *bp )
    {
        char *channel="CONFIG";
        if( !nmp.setParmByStr( parm, value ) )          // if EEPROM User parm not found
        {
            nmp.printMgnInfo( channel, parm, "is unknown" );     
            RESPOND( bp, "%s not found\r\n", parm );
            return false;
        }         
        myp.saveMyEEParms();                            // save to EEPROM
        nmp.printMgnParm( channel, parm );              // update the table
        nmp.printMgnInfo( channel, parm, "updated" );   // update the INFO  

        doParmFunc( parm, value );                      // perform function associated with this parameter
        RESPOND( bp, "User parm %s updated\r\n", parm ); 
        return true;
    }
    
    void showMegAllParms()
    {
        mgn.tableClear();
        mgn.tableSet( " ",          " ", "All EEP Parms" );
        mgn.tableSet( "WIFI PARMS", " ", "----------------------------" );
        eep.printMgnWiFiParms( "" ); 
        mgn.tableSet( "USER PARMS", " ", "----------------------------" );
        nmp.printMgnAllParms( "" );
    }

    char *getAllState( BUF *s )
    {
        s->set("simul():%d\r\n", isSimulON() );
        s->add("notify():%d\r\n", isNotifyON() );
        s->add("streamON:%d\r\n", myp.streamON );
        STATS st = longT.getStats();
        s->add("Vavg=%.1fV\r\nVmin=%.1fV\r\n", st.Vavg, st.Vmin );
        s->add("Wavg=%.0fW\r\nWmax=%.0fW\r\n", st.Wavg, st.Wmax );  
        return s->c_str();
    }
    // refreshes state based on display setting
    void refreshState( int mask, BUF *bp )
    {
        BUF sb(200);
        char *p = getAllState( &sb );
        if( mask & STREAM_MEGUNO )
            mgn.controlSetText( "state", p );
        if( mask & STREAM_CONSOLE )
            RESPOND( bp, "%s", p );
    }

    void readSimulPhase( VAP *vp, int phase )
    {
       vp->volts = myp.simValue.volts;
       vp->amps  = myp.simValue.amps/2.0;
       vp->watts = myp.simValue.watts/2.0;
       vp->kWh   = 0.0;  
       switch( phase )
       {
            case 1: vp->error = "Sim#1 OK";            
                    break;
            case 2: vp->error = "Sim#2 OK";
                    break;
            default:vp->error = "Sim#12 OK";
                    break;
       }
    }
    VAP getSimulPhase( int phase )
    {
        VAP vp;
        readSimulPhase( &vp, phase );
        if( !(phase == 1 || phase == 2) )
            vp.watts = myp.simValue.watts;
       return vp;
    }
    
