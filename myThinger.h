#pragma once
#include <cliClass.h>

    #define INC_THINGER                 // select this to enable THINGER interface
  
    #define INC_DROPBOX                 // select this to inclue Thinger Dropbox via IFTTT
    #define INC_EMAIL                   // select this to inclue Thinger EMail
    #define INC_TEXT                    // select this to inclue Thinger ClickSend
    #define INC_NOTIFY              //     select this to inclue Thinger IFTTT notifications. DOES NOT WORK -- GETS STUCK
    #define INC_ALARM
  
    #ifdef INC_THINGER
        void setupThinger();
        void thingHandle();
        void thingStream();
        
        #define SETUP_THINGER() setupThinger()
        #define THING_HANDLE()  thingHandle()
        #define THING_STREAM()  thingStream()
        
        void alarmViaIFTTT();   
        
        void sendToDropbox();    
        void sendByText();
        void sendByPhone();
        void sendByEmail();
        void sendByIFTTT();
        
        void timedToDropbox();
        extern CMDTABLE thsTable[];
    #else
        #define SETUP_THINGER() 
        #define THING_HANDLE()  
        #define THING_STREAM()  
    #endif
