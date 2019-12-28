//  **** Enable or disable Thinger.io in myThinger.h ****

    #define METER_READING_PERIOD 2                          // seconds. How often to read the meter
    #define ALERT_REPEAT_PERIOD (60/METER_READING_PERIOD )  // seconds. How long to wait for IFTTT-NOTIFYrepeats

// ----------------------------------------------------------------------------
    #include <FS.h>
    #include <ArduinoOTA.h>
    #include <Ticker.h>         // needed to RESET statistics
    
    #include "SimpleSTA.h"      // in GKE-Lw
    #include "CommonCLI.h"      // in GKE-Lw
                                
    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "myCliHandlers.h"
    #include "mySupport.h"
    #include "myEngClasses.h"   // this also includes pz16Class.h
    #include "myThinger.h"  
    
//------------------ References and Class Allocations ------------------------

    Ticker tk;
    CLI cli;
    EXE exe;
    EEP eep;
    BUF buffer( 1600 );         // buffer to hold the response from the local or remote CLI commands.

// ------------------------------------ MAIN SETUP ----------------------------------

    static int line = 0;                        // oled line counter
    #define DSP_CLEAR   oled.dsp( O_CLEAR ); line=0
    #define DSP(A, ...) oled.dsp( line++, A, ##__VA_ARGS__)

    void setupOTA( char *name );                // forward reference

void setup() 
{
	int runcount = setjmp( myp.env );           // env is allocated in myGlobals
	cpu.init( 9600 );
    cpu.setSecBaud( 9600, 12 );                 // set sec port baud and OE to GPIO12 (i.e D6)

    ASSERT( SPIFFS.begin() );                   // start the filesystem;
    
    char *name = getDevName( "Pwr" );           // make the name for this device

    oled.dsp( O_LED130 );                       // initialize OLED
    DSP_CLEAR;                                  

// *********************************            // Revise this!
    DSP( "\t%s (Dec 2019)", name  );
    PFN( "mDNS: %s.local", name );
// Use 
//      dns-sd -Z _arduino._tcp
//      dns-sd -G v4 name.local
//      mobile ZeroConfig
// *********************************

    pz.setOLED( NULL );                         // disable OLED in PZ16    

    myp.initAllParms();                         // initialize volatile & user EEPROM parameters
    DSP( "Read %d parms", nmp.getParmCount() );
    oled.dsp( BRIGHT( myp.gp.obright ) );		// adjust OLED brightness
  
    linkParms2cmnTable( &myp );                 // Initialize CLI tables
    exe.initTables();                           // clear all tables
    exe.registerTable( cmnTable );              // register common CLI tables. See GKE-Lw
    exe.registerTable( mypTable );              // register tables from myCliHandlers.cpp
    exe.registerTable( thsTable );              // register tables from mySupport.cpp
    
    DSP( "Waiting for CLI" );           
    DSP( "or BTN SmartConf" );           
    startCLIAfter( 5/*sec*/, &buffer );         // this also initializes cli(). See SimpleSTA.cpp    
                                                // If button is pressed, does SmartConfig of WiFi
    DSP( "Waiting for WiFi" );

    setupWiFi();                                // use the EEP to start the WiFi. If no connection
    setupOTA( name );                           // prepares and starts OTA. mDNS is GkePwr-MAC05
    
    DSP( "%s", WiFi.SSID().c_str() );
    DSP( "RSSI:%ddBm", WiFi.RSSI() );
    IPAddress ip=WiFi.localIP();
    DSP( "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
    
    SETUP_THINGER();                            // if thinger is not included, it does nothing

    // ------------- Other initialization --------------------------------

    secTic.setSecCount( myp.gp.scansec );       // update the tic used to read PZEM
  
    longT.resetStats();                         // reset hourly statistics
    shortT.resetStats();                        // reset minute stats
    
    alm.setUVthrs( myp.gp.notifyV > 90.1 ? myp.gp.notifyV  : -1 );
    alm.setOPthrs( myp.gp.notifykW>  0.1 ? myp.gp.notifykW * 1000.0 : -1 );

    tmb.setHMS( 20, 40, 0 );                    // set current time
    ticMin.setSecCount( tmb.getSecCount() );
    ticHr.setSecCount( tmb.getSecCount() );
    tk.attach_ms( 1000, [](){ tmb.update(), ticMin.update(); ticHr.update(); } );  

    showMegAllParms();                          // located in mySupport.cpp
    delay( 4000 );
    DSP_CLEAR;
    cli.prompt();
}

// ------------------------- OTA PREP -------------------------------------------------------

void setupOTA( char *devname )                          // generic OTA handler
{
    // ArduinoOTA.setPort(8266);                        // this is the default port for OTA
  
    ArduinoOTA.setHostname( devname );                  // set mDNS name

    ArduinoOTA.onStart([]() { oled.dsp( O_CLEAR ); oled.dsp( 0,"\t\bOTA"); });
    ArduinoOTA.onEnd([]()   { oled.dsp( 6, "\t\bDONE"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        oled.dsp(3, "\t\b%u%%", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) 
    { 
        oled.dsp( O_CLEAR );
        oled.dsp( 0,"\t\bOTA Err%u", error);
        if (error == OTA_AUTH_ERROR)            oled.dsp(3, "\tAuth Failed");
        else if (error == OTA_BEGIN_ERROR)      oled.dsp(3, "\tBegin Failed");
        else if (error == OTA_CONNECT_ERROR)    oled.dsp(3, "\tConnect Failed");
        else if (error == OTA_RECEIVE_ERROR)    oled.dsp(3, "\tReceive Failed");
        else if (error == OTA_END_ERROR)        oled.dsp(3, "\tEnd Failed");
    });
    ArduinoOTA.begin();    
}
// ------------------------ MAIN LOOP -------------------------------------------------------
TICsec pr(10);
PROF prof( 10, 50, 100, 200, 300 );
static bool process_withWiFi = false;           // this flag is set by stdLoop()to indicate to
                                                // main loop() that WiFi processing is necessary
void stdLoop()                                  // This is always executed regardless if WiFi connected or not
{
    ArduinoOTA.handle();
   
    if( cli.ready() )                           // handle serial interactions
    {
        char *p = cli.gets();
        exe.dispatchBuf( p, buffer );           // required by Meguno
        buffer.print();
        cli.prompt();
    }
    if(  secTic.ready() )                       // secTic is allocated in mySupport.cpp
    {                
        cpu.led( ON );
        
        myp.phase = (myp.phase<=1) ? 2 : 1;     // set phase 1, 2, 1, 2, etc
        int i = myp.phase;
        
        if( isSimulON() )
        {
            readSimulPhase( &myp.vap[i], i );
            myp.vap[0]  = getSimulPhase( 1+2 );           
        }
        else
        {
            if( myp.gp.scanON )                 // scanning 
            {
                eng.readPhase( &myp.vap[i], i, myp.gp.ampscale );                
                myp.vap[0]  = eng.getPhase( 1+2 );  // compute phase 1+2
            }
            // do nothing if scan is OFF
        }
        shortT.updateStats( &myp.vap[0] );      // update short term stats (used in buckets)
        longT.updateStats( &myp.vap[0] );       // update long term stats (used by dropbox)
        alm.updateAlarms( &myp.vap[0] );                

        reportVAP( myp.gp.display );            // show to console accordingly
        process_withWiFi = true;                // show to Thinger if activated and WiFi OK
        cpu.led( OFF );
        
    }
//    if( pr.ready() )
//        prof.report(true);
}

TICsec streamTic( 30 );                         // 30 second reporting, regardless if streaming is ON or OFF

void loop()                                     // Main loop
{
    //prof.start();
    stdLoop();
    //prof.profile();
    if( checkWiFi() )                           // Good WiFi connection?
    {    
        if( process_withWiFi )
        {
            process_withWiFi = false;            
            timedToDropbox();                    // check if need to send to dropbox. If so, use short or long term averages
            
            if( alm.triggeredUV() || alm.triggeredOP() )
                alarmViaIFTTT();
            
            if( myp.streamON || streamTic.ready() )                  // this is the only place that myp.streamON is used
                THING_STREAM();                 // streamON is turned off every HourlyStat to save bandwidth
        }
        THING_HANDLE();                         // Continue polling. If WiFi is diconnected
    }
    else
        reconnectWiFi();                        // reconnect to WiFi (non-blocking function)
}
