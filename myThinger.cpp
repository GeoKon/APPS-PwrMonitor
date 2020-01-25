#include "myThinger.h"
#ifdef INC_THINGER

    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "mySupport.h"
    #include "myEngClasses.h"   // this also includes pz16Class.h
    
    #define USERNAME "GeoKon"
    #define DEVICE_ID "PZEM004"
    #define DEVICE_CREDENTIAL "success"
//  #define _DEBUG_
    #define _DISABLE_TLS_           // very important for the Thinger.io 

    #include <ThingerESP8266.h>     // always included even if not #ifdef
       
//------------------ Class Allocations ------------------------------------------------------------------

    ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);   // changed in the setupThinger();

// ------------------------------- THINGER INITIALIZATION -----------------------------------------------
    struct ThingerUpdates_t
    {
        const char *simul;
        const char *notify;
        const char *alarmLV;
        const char *alarmOP;
        const char *dropboxOn;                
    } thg;
    
    static void updateThinger()
    {
        int type = isSimulON();     // get simulation values
        bool bv = type & 1;
        bool bw = type & 2;
        thg.simul = bv ? ( bw ? "Volts & kWatts" :"Volts")
                         : ( bw ? "kWatts" : "Disabled");
        //refreshState( myp.gp.display, &exbuf);

        int notify = isNotifyON();                                                    
        thg.notify = (notify & 1) ? ((notify & 2) ? "sensing UV & OP" :"sensing UV")
                    : ((notify & 2) ? "sensing OP":"Disabled");

        thg.alarmLV  = alm.getUVAlarm();
        thg.alarmOP  = alm.getOPAlarm();

        static char dtemp[16];
        if( myp.gp.dbMinut > 0 )
            sf( dtemp, 16, "every %dmin", myp.gp.dbMinut );
        else
            sf( dtemp, 16, "Disabled" );                                                        
        thg.dropboxOn = dtemp;             
    }
        
    void thingHandle()
    {
        thing.handle();
    }
    void thingStream()
    {
       updateThinger();             // if you remove this, remove also pson from ["reading"]
        
        thing.stream( thing["reading"] );
        //thing.stream( thing["extra"] );
    }    
    
    static char exrsp[1500];        // Buffer used for Thinger CLI. Must be static
    BUF exbuf( exrsp, 1500 );       // BUF wrapper of exstr[] 
    static STATS stShortT;          // crated by Thinker used by the Dropbox
    static STATS stLongT; 

    void setupThinger()
    {
        PF("----- Initializing THING\r\n");
        thing.add_wifi( eep.wifi.ssid, eep.wifi.pwd );  // initialize Thinker WiFi

        thing.set_credentials( USERNAME, DEVICE_ID, DEVICE_CREDENTIAL );

// ----------------------------------------------------------------------------
//                   Thinger.io CLI
// ----------------------------------------------------------------------------
        thing["CliCmd"] << [](pson &in)
        {   
            const char *p = (const char *)in["extCmd"];
            PFN("Thinger cmd [%s]", p );
            exe.dispatchBuf( p, exbuf );            // execute command
            PR( exrsp );                            // print results to console     
        };        
        thing["CliRsp"] >> [](pson& ot )                          
        {   
            //PFN( "Thinger rsp");
            PR( exrsp );
            ot["value1"] = (const char *)exrsp;         
        };
        thing["help"] << [](pson &inp)
        {   
			if( inp.is_empty() ) 
			{
				inp=0;
				return;
			}
			int sel = inp;
            char *p;
            switch( sel )
            {
            	default:
            	case 1: p="h"; break;
            	case 2: p="Parms"; break;
            	case 3: p="Wifi"; break; 
            }
            exe.dispatchBuf( p, exbuf );         	// execute command
            PR( exrsp );                            // print results to console     
        };

/* --------------------------------------- HELP & DIAGNOSTICS ------------ ------------------------------------- 
  * 
  * ------------------------------------------------------------------------------------------------------------ */
        thing["led"]        << invertedDigitalPin( MYLED ); 

 /* --------------------------------------- ACQUISITION & STREAMING CONTROL ------------------------------------- 
  * 'scanON is saved in EEPROM (non-volatile). 'streamON' starts as OFF (volatile).
  * ------------------------------------------------------------------------------------------------------------ */
         thing["scanON"]     << [](pson &inp)   {   if( inp.is_empty() ) 
                                                    {
                                                        inp = (bool) myp.gp.scanON;     // return simulated kW
                                                    }
                                                    else                                        // set simulated watts
                                                    {
                                                        char value[8];                                                            
                                                        sf( value,8,"%d", (bool) inp );
                                                        if( myp.gp.display & 2 ) setAnyMegParm( "scanON", value, &exbuf );
                                                        else                     setAnyEEParm ( "scanON", value, &exbuf );   
                                                    }
                                                };
                                                 
        thing["streamON"] << [](pson &inp)    {   if( inp.is_empty() ) 
                                                    inp = myp.streamON ? true : false;     // get state of streamON
                                                else                            
                                                {
                                                    myp.streamON = (bool) inp;
                                                    refreshState( myp.gp.display, &exbuf);
                                                    myp.streamAll = true;
                                                }
                                              };                                                                                                      

/* --------------------------------------- SIMULATION ---------- ------------------------------------------------
 * 'simulV' is set by a slider to values 90V-140V. 'simulW' is also set by a slider to values 0-10kW. 
 * -------------------------------------------------------------------------------------------------------------- */
        thing["simulV"] << [](pson &inp) {   
                                            if( inp.is_empty() ) 
                                            {
                                                inp=myp.simValue.volts;      //eng.getSimul().volts;              // return simulated voltage
                                            }
                                            else                            // set simulated voltage
                                            {
                                                myp.simValue.volts = (float)inp;
                                                myp.simValue.amps  = myp.simValue.volts > 0.1 ? myp.simValue.watts/myp.simValue.volts : 0.0;
                                                myp.streamAll = true;
                                            }                                            
                                          };
        thing["simulkW"] << [](pson &inp){   
                                            if( inp.is_empty() ) 
                                            {
                                                inp = myp.simValue.watts/1000.0;        // return simulated kW
                                            }
                                            else                                        // set simulated watts
                                            {
                                                myp.simValue.watts = (float)inp * 1000.0;    // this is per phase
                                                myp.simValue.amps  = myp.simValue.volts > 0.1 ? myp.simValue.watts/myp.simValue.volts : 0.0;
                                                myp.streamAll = true;
                                            }                                            
                                          };

/* --------------------------------------- Notification HANDLERS ------------------------------------------------
 * notfyV is entered as a slide in Thinger. It indicates the undervoltage condition to trigger IFTTT notification
 * notfyW is entered as a slide in Thinger. It indicates the overpower condition to trigger IFTTT notification
 * The 'notfy'-bits 1, 2, 4 eeprom parameter are used as indicators/masks of which condition were set
 * -------------------------------------------------------------------------------------------------------------- */
        thing["notfyV"]     << inputValue( myp.gp.notifyV,
                                                {   
                                                    char value[8];                                                            
                                                    sf( value,8,"%.1f",myp.gp.notifyV );
                                                    if( myp.gp.display & 2 ) setAnyMegParm( "notifyV", value, &exbuf );   
                                                    else                     setAnyEEParm ( "notifyV", value, &exbuf );   
                                                    myp.streamAll = true;
                                                });
        thing["notfykW"]    << inputValue( myp.gp.notifykW,
                                                {   
                                                    char value[8];                                                            
                                                    sf( value,8,"%.2f",myp.gp.notifykW );
                                                    if( myp.gp.display & 2 ) setAnyMegParm( "notifykW", value, &exbuf );  
                                                    else                     setAnyEEParm ( "notifykW", value, &exbuf );   
                                                    myp.streamAll = true;
                                                });

/* --------------------------------------- RESET kWh Energy -----------------------------------------------*/

       thing["resetkWh"]    << [](pson &inp)    {  if( inp.is_empty() ) 
                                                    inp = false;
                                                else                            
                                                {
                                                    inp = true;
                                                    eng.zerokWh();
                                                }
                                              }; 

/* --------------------------------------- DROBOX HANDLER -------------------------------------------------------
 * [dbMinut] is an input set by the slider in Thinger. This can also be set using SETEE dbMinute <value>
 * -------------------------------------------------------------------------------------------------------------- */
        thing["dbMinut"]    << inputValue( myp.gp.dbMinut, 
                                                {  
                                                    char value[8];                                                            
                                                    sf( value,8,"%d",myp.gp.dbMinut );
                                                    if( myp.gp.display & 2 ) setAnyMegParm( "dbMinut", value, &exbuf );
                                                    else                     setAnyEEParm ( "dbMinut", value, &exbuf );   
                                                    myp.streamAll = true;
                                                });

/* --------------------------------------- DISPLAY CONTROL ----------------------------------------------------- */

        thing["display"]     << [](pson &inp)   {   if( inp.is_empty() ) 
                                                    {
                                                        inp = (int) myp.gp.display;     // return simulated kW
                                                    }
                                                    else                                        // set simulated watts
                                                    {   
                                                        char value[8];                                                            
                                                        sf( value,8,"%d", (int) inp );
                                                        if( myp.gp.display & 2 ) setAnyMegParm( "display", value, &exbuf );
                                                        else                     setAnyEEParm ( "display", value, &exbuf ); 
                                                        myp.streamAll = true;  
                                                    }
                                                };
 
/* --------------------------------------- STREAMING OF MEASUREMENTS ---------------------------------------------
 * "reading" is updated by the DEVICE every N-seconds or so. The DEVICE updates the "myp.vap[]" (volts, amps, etc)
 *  Rotates one type of measurement at a time, i.e. all of them are done in two cycles
 * -------------------------------------------------------------------------------------------------------------- */
        thing["reading"] >> [](pson& ot )       // meter is read asynchronously by the loop() every N-seconds
        {   
            ot["volts"]     = myp.vap[myp.phase].volts;
            ot["amps" ]     = myp.vap[myp.phase].amps;
            ot["errscan"]   = myp.streamON ? myp.vap[myp.phase].error : "Streaming is OFF";
            ot["watts" ]    = myp.vap[0].watts/1000.0;
            ot["energy" ]   = myp.vap[0].kWh;           // total energy as read by the meter
                        
            ot["myTime" ]   = tmb.getTime();
            
//          ot["Vstats"]    = shortT.getVStats();       // get string of daily stats
//          ot["Wstats"]    = shortT.getWStats();       // get string of daily stats            
//
//          ot["simul"]     = (const char *)thg.simul;
//          ot["notify"]    = (const char *)thg.notify;
            ot["alarmLV"]   = (const char *)thg.alarmLV;
            ot["alarmOP"]   = (const char *)thg.alarmOP;
//          ot["dropboxOn"] = (const char *)thg.dropboxOn;
        };

/* --------------------------------------- STATISTICS (for the BUCKETS)------------------------------------------
 * Minute statistic are in shortT and Hourly statistics are in longT. Buckets call periodically these objects
 * and store respective stats in tables. The main loop uses 'ticS.ready()' and 'ticL.ready()' which trigger 
 * every hour and every day respectively to reset the statistics
 * -------------------------------------------------------------------------------------------------------------- */

        thing["MinuteStats"] >> [](pson& ot )       // Buckets (15min or Hourly) in POLLING MODE. Use short term averages
        {
            
            stShortT = shortT.getStats();                 // get short term stats
            ot["1:Vmin"]      = stShortT.Vmin;
            ot["2:Vavg"]      = stShortT.Vavg;
            ot["3:Wavg"]      = stShortT.Wavg/1000.0;
            ot["4:Wmax"]      = stShortT.Wmax/1000.0;
            ot["5:kWh" ]      = stShortT.kWh;
            shortT.resetStats();
            PFN("ShortT stats reset at %s by Thinger", tmb.getTime() );
        };
        thing["HourlyStats"] >> [](pson& ot )       // Buckets (15min or Hourly) in POLLING MODE. Use short term averages
        {
            stLongT = longT.getStats();                  // get long term stats
            ot["1:Vmin"]      = stLongT.Vmin;
            ot["2:Vavg"]      = stLongT.Vavg;
            ot["3:Wavg"]      = stLongT.Wavg/1000.0;
            ot["4:Wmax"]      = stLongT.Wmax/1000.0;
            ot["5:kWh" ]      = stLongT.kWh;
            longT.resetStats();
            PFN("LontT stats reset at %s by Thinger", tmb.getTime() );
        };

/* --------------------------------------- REPORTING HANDLERS -----------------------------------------------------
 * [TestMsg] sets the reporting type/action. 
 * When the control button [Notify] is pressed, the above reporting action is performed.
 * 
 * [pwrAssist] is called by from IFTTT with two ingredient, "value1" and "value2"
 * "value2" represent the reporting action in text.
 * -------------------------------------------------------------------------------------------------------------- */
        static int action;
        thing["TestMsg"] << [](pson &inp)
        {               
            if( inp.is_empty() ) 
            {
                inp=action;
                return;
            }
            action = inp;            
        };
        thing["Notify"] << [](pson &inp)
        {               
            if( inp.is_empty() ) 
            {
                inp=false;
                return;
            }
            switch( action )
            {
                default: exbuf.set("."); break;     // empty display
                case 1: sendByEmail(); break;
                case 2: sendByText(); break;                
                case 3: sendToDropbox(); break; 
                case 4: sendByPhone(); break;
            }
            thing.stream( thing["CliRsp"] );    // update Thinger notificatons  
        };
        
        thing["pwrAssist"] << [](pson &in)
        {   
            const char *methd = (const char *)in["value2"];
            PF("Received [%s] [%s]\r\n", (const char *)in["value1"], methd );
            
            if( !strcasecmp( methd, "by email" ) )
                sendByEmail();
            else if(!strcasecmp( methd, "by text" ))
                sendByText(); 
            else if(!strcasecmp( methd, "by phone" ))
                sendByPhone();
            else if(!strcasecmp( methd, "to dropbox" ))
                sendToDropbox();
            else
                sendByText();
                //sendByIFTTT();    
            thing.stream( thing["CliRsp"] );      // update Thinger notificatons  
        };
    }

// ------------------- called by loop() to save to Dropbox ------------------------------- 
    void timedToDropbox()
    {
        static uint32 count = 0;        // if zero, data will be written to Dropbox
        if( myp.gp.dbMinut <= 0 ) 
        {
            count = 0;
            return;                     // do nothing if not enabled
        }
        if( count == 0 )
            sendToDropbox();            // use minute statistics
    
        count += myp.gp.scansec;        // counter of seconds
    
        if( count > (myp.gp.dbMinut*60L ) )
            count = 0;
    }

/* ----------------------------- NOTIFICATION HANDLERS ---------------------------- 
 * Includes:
 *               sending to Dropbox     Thinger(Web hooks)->Dropbox
 *               sending an Email       Thinger(Email)
 *               sending a Text(SMS)    Thinger(HTTP Request)->ClickSend
 *               placing a Phone Call   Thinger(HTTP Request)->ClickSend
 * -------------------------------------------------------------------------------- */

BUF ntfy(256);  // Common buffer used for all notifications. Multiple pointers
                // to (const char*) are obtained on a rotating basis.
                // in heavy use some data can be overwritten!                                

void alarmViaIFTTT()
{
    char *formatUV="%s (%.0fV vs %.0fV)";
    char *formatOP="%s (%.1fkW vs %.1fkW)";
    char *formatAL="%s (%.0fV vs %.0fV), %s (%.1fkW vs %.1fkW)";
    
    ntfy.append();              // Advance to next free slot of ntfy buffer
    if( alm.triggeredUV() )
    {
        if( alm.triggeredOP() )
            ntfy.set( formatAL, alm.getUVAlarm(), myp.vap[0].volts, myp.gp.notifyV,
                                alm.getOPAlarm(), myp.vap[0].watts/1000.0, myp.gp.notifykW );
        else
            ntfy.set( formatUV, alm.getUVAlarm(), myp.vap[0].volts, myp.gp.notifyV );
    }
    else
    {
        if( alm.triggeredOP() )
            ntfy.set( formatOP, alm.getOPAlarm(), myp.vap[0].watts/1000.0, myp.gp.notifykW );
    }
    PFN( "ALARM %s", ntfy.c_str() );
    
    #ifdef INC_ALARM
        pson datn;
        datn["value1"] = (const char *)ntfy.c_str();
        datn["value2"] = "";        
        thing.call_endpoint( "CLICK_TEXT",    datn ); 
        thing.call_endpoint( "IFTTT_Dropbox", datn );       // ...and save to Dropbox
        exbuf.set( "ALARM Sent\r\n" ); 
    #endif
}

void sendToDropbox( ) 
{
    ntfy.append();              							// Advance to next free slot of ntfy buffer
    
    ntfy.set( "I=(%.1fA, %.1fA), P=%.2fkW, E=%.1fkWh, Mn=(avg:%.2fkW, max:%.2fkW), Hr=(avg:%.2fkW, max:%.2fkW)",
        myp.vap[1].amps,
        myp.vap[2].amps,
        myp.vap[0].watts/1000.0,
        myp.vap[0].kWh,
        stShortT.Wavg/1000.0, stShortT.Wmax/1000.0, 
        stLongT.Wavg/1000.0, stLongT.Wmax/1000.0 );											// use the previous buffers
        
    PF("DROPBOX: %s\r\n", ntfy.c_str() );
        
    #ifdef INC_DROPBOX
        pson datn;
        datn["value1"] = (const char *)ntfy.c_str();
        datn["value2"] = "";
        thing.call_endpoint( "IFTTT_Dropbox", datn );
        exbuf.set( "Sent to Dropbox\r\n" );  
     #endif
}
void sendByEmail()
{   
    IPAddress ip = WiFi.localIP();
    
    ntfy.append();              // Advance to next free slot of ntfy buffer
    char *subject = ntfy.set( "George's PowerMeter (IP:%d.%d.%d.%d, RSSI:%ddBm)\r\n", ip[0],ip[1],ip[2],ip[3], WiFi.RSSI()  );
    
    ntfy.append();              // Advance to next free slot of ntfy buffer
    char *body    = ntfy.set( "Voltage=%.1fV, Current=(%.1fA, %.1fA), Power=%.2fkW, Total Energy=%.2fkWh, %s, %s\r\n",
        myp.vap[0].volts, 
        myp.vap[1].amps,
        myp.vap[2].amps,
        myp.vap[0].watts/1000.0,
        myp.vap[0].kWh,
        thg.alarmLV, 
        thg.alarmOP );
    PF("EMAIL: %s", body );       

    #ifdef INC_EMAIL
        pson datn;
        datn["value1"]   = (const char *)subject;
        datn["value2"]   = (const char *)body;
        thing.call_endpoint( "gkemail", datn ); 
        exbuf.set( "Email sent\r\n" );      
    #endif
}
void sendByText( )
{
    ntfy.append();              // Advance to next free slot of ntfy buffer
    ntfy.set( "%.1fV, (%.1fA,%.1fA), %.2fkW, %.2fkWh",
        myp.vap[0].volts, 
        myp.vap[1].amps,
        myp.vap[2].amps,
        myp.vap[0].watts/1000.0,
        myp.vap[0].kWh);
    PFN("TEXT: %s", ntfy.c_str() );  
    
    #ifdef INC_TEXT
        pson datn;
        datn["value1"] = (const char *)ntfy.c_str();
        datn["value2"] = "";
        thing.call_endpoint( "CLICK_TEXT", datn );  
        exbuf.set( "Text (SMS) sent\r\n" );        
     #endif
}
void sendByPhone( )
{
    ntfy.append();              // Advance to next free slot of ntfy buffer
    ntfy.set( "You are currently consuming %.0f Watts. So far you have used %.0f kilo Watt hours of electricity",
        myp.vap[0].watts,
        myp.vap[0].kWh);
    PFN("PHONE: %s", ntfy.c_str() );  
    
    #ifdef INC_TEXT
        pson datn;
        datn["value1"] = (const char *)ntfy.c_str();
        datn["value2"] = "";
        thing.call_endpoint( "CLICK_CALL", datn );  
        exbuf.set( "Phonecall placed\r\n" );        
     #endif
}
void sendByIFTTT( )
{
    ntfy.append();              // Advance to next free slot of ntfy buffer
    ntfy.set( "%.1fV, (%.1f, %.1f)A, %.2fkW, %.2fkWh, %s, %s",
        myp.vap[0].volts, 
        myp.vap[1].amps,
        myp.vap[2].amps,
        myp.vap[0].watts/1000.0,
        myp.vap[0].kWh,
        thg.alarmLV,
        thg.alarmOP );
    PF("IFTT Ntfy: %s", ntfy.c_str() );
    #ifdef INC_NOTIFY
        pson datn;
        datn["value1"] = (const char *)ntfy.c_str();
        datn["value2"] = "";
        thing.call_endpoint( "IFTTT_Notify", datn );
        exbuf.set( "IFTTT Notification sent\r\n" );        
     #endif  
}

CMDTABLE thsTable[]=            // functions to test IFTTT endpoints
{
    #define PROTO []( int n, char **arg )
    { "notify",  "Send Measurements by IFTTT Notification", PROTO { sendByIFTTT(); } },
    { "text",    "Send Measurements by Text (SMS)",         PROTO { sendByText(); } },
    { "phone",   "Send Measurements by a Phone Call",       PROTO { sendByPhone(); } },
    { "email",   "Send Measurements by Email",              PROTO { sendByEmail(); } },
    { "dropbox", "Send Measurements to Drobox",             PROTO { sendToDropbox(); } },
    {NULL, NULL, NULL}
};

#endif // INC_THINGER
