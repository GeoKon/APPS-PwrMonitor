/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#pragma once

// ----------------------------------- Structures -------------------------------------

typedef struct eng_t
{
    float volts;        // as read by the PZEM meter
    float amps;
    float watts;
    float kWh;       
    const char *error;
} VAP;

#include "IGlobal.h"        // in GKE-Lw. Includes and externs of cpu,...,eep
#include <nmpClass.h>

// ----------------- Exported by this module ---------------------------------------

    extern NMP nmp;             // allocated in myGlobals.cpp; used only by this                             

    #define BUTTON       0      // INPUTS
    #define RFINP        0      // 13 is D3 (aka RXD2)
    #define MYLED       16      // 2 for old MCU, 16 for new ones
    #define RELAY       12      // this is D6 (aka MISO)
    #define SWINP        0      // 14 is D5 
                                // GPIO-4 (aka D2) is SDA used by OLED
                                // GPIO-5  (aka D3) is SCL used by OLED        
#define MAGIC_CODE 0x1457

    #define NO_NOTFYV 90.0
    #define NO_NOTFYW 0.0
/*
 * If slider goes above NO_NOTFY, then my.notfyON becomes true IFTTT is activated if an alarm condition occurs
 * If the slader is below NO_NOTFY, then my.notfyON is off and no checking occurs
 */
// --------------- Definition of Global parameters ---------------------------------

    class Global: public IGlobal
    {
      public:												// ======= A1. Add here all volatile parameters 
        
        int phase;              // 1 or 2 indicate phase read.
        VAP vap[3];             // [1, 2] measurements Ph#1, Ph#2 as read by the PZEM, scaled and kWh adjusted
                                // [0] computed Ph#1 + Ph#2
        VAP  simValue;          // simulated V, A, W. Watts is TOTAL. Amps is computed as W/V
                                
        bool streamON;          // streaming enabled or disabled. Used by Thinger.io o
        bool streamAll;         // state was changed (used by Thinker only)
                
		void initVolatile()                                 // ======= A2. Initialize here the volatile parameters
		{
            phase = 1;          // phase to read
            vap[0].volts = vap[0].amps = vap[0].watts = vap[0].kWh = 0.0;
            vap[0].error = "";
            vap[1] = vap[2] = vap[0];
 
            simValue    = vap[0];
            streamON    = false;
            streamAll   = true;
		}    
		void printVolatile( char *prompt="",                // ======= A3. Add to buffer (or print) all volatile parms
		                    BUF *bp=NULL ){;}
		struct gp_t                                         // ======= B1. Add here all non-volatile parameters into a structure
		{                           
            int scanON;             // enables or disables scanning
            int scansec;            // n>0 is number of seconds per scan
            
            int display;            // 1=show console, 2=show Meguno every scan, 4=OLED
            int obright;			// oled brightness 
            float ampscale;         // scale factor for AMPs
            
            float notifyV;          // no undervoltage notification
            float notifykW;         // no overwattage notification
           
            int dbMinut;            // how often to save to dropbox
 		} gp;
		
		void initMyEEParms()                                // ======= B2. Initialize here the non-volatile parameters
		{
            gp.scanON       = 1;
            gp.scansec      = 2;
            gp.display      = 1;
            gp.obright      = 1;
            gp.ampscale     = 4.0;
            
            gp.notifyV      = NO_NOTFYV;                    // no undervoltage notification
            gp.notifykW     = NO_NOTFYW;                    // no overwattage notification
            
            gp.dbMinut      = 20;                           // how often to update the dropbox
		}		
        void registerMyEEParms()                           // ======= B3. Register parameters by name
        {
            nmp.resetRegistry();
            nmp.registerParm( "scanON",    'd', &gp.scanON,     "0=no scanning, 1=scanning ON" );
            nmp.registerParm( "scanSec",   'd', &gp.scansec,    "Number of seconds per scan" );
            nmp.registerParm( "display",   'd', &gp.display,    "Stream to 1=console, 2=Meguno" );
            nmp.registerParm( "obright",   'd', &gp.obright,    "OLED brightness 1...255" );
            nmp.registerParm( "ampscale",  'f', &gp.ampscale,   "Scale factor for Amp-meter" );

            nmp.registerParm( "notifyV",   'f', &gp.notifyV,    "UV threshold. <90 to turn off" );
            nmp.registerParm( "notifykW",  'f', &gp.notifykW,   "OP threshold. 0 to turn off" );
            
            nmp.registerParm( "dbMinut",   'd', &gp.dbMinut,    "Frequency of Dropbox in min" );

			PF("%d named parameters registed\r\n", nmp.getParmCount() );
			ASSERT( nmp.getSize() == sizeof( gp_t ) );                             
        }
        void printMyEEParms( char *prompt="",               // ======= B4. Add to buffer (or print) all volatile parms
                             BUF *bp=NULL ) 
		{
			nmp.printAllParms( prompt );
		}
        void initAllParms()
        {
            initTheseParms( MAGIC_CODE, (byte *)&gp, sizeof( gp ) );
        }
	};
    
    extern Global myp;                                      // exported class by this module
