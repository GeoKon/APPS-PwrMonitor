#include "myEngClasses.h"          // this also include myGlobals.h
#include "pz16Class.h"          // all PZEM-016 functions
#include "oledClass.h"

// ----------------------------------- global allocations -----------------------------
    
    CPU  cpu;                    // needed for Sec UART
    OLED oled;
    PZENG eng;    
    VWSTATS shortT;             // short time averages
    VWSTATS longT;              // long term averages
    ALARM alm;                  // alarms
        
    PZ16 pz( cpu, oled );       // allocation of the PZEM-016 Class

    TMBASE tmb;
    TOCsec ticMin(60);            // every min
    TOCsec ticHr (60*60);         // every hour

// ----------------------------- PZENG Class Definition ---------------------------------

// This could be optimized and read data directly to myp Globals. Kept as is for simplicity

    PZENG::PZENG()
    {
        VAP n;
        zerokWh();
        n.volts = n.amps = n.watts = n.kWh = 0.0;
        sim = ph[0] = ph[1] = ph[2] = n;
        myerror[0] = myerror[1] = myerror[2] = 0;
    }
    void PZENG::readPZM( int i, float scale )  // i must be 1 or 2                 
    {
        ASSERT( (i==1) || (i==2) );

        pz.measure( i );                        // read from respective slave address

        //PF("Ph#%d Z:%d Ref=%.2f ", i, doref[i], pz.getkWh() );
        //pz.print();
        
        if( (myerror[i] = pz.getError()) )      // in case of an error, zero measurements
        {
            ph[i].volts = ph[i].amps = ph[i].watts = 0.0;
            // no change to kWh
            ph[i].error = getErrorStr( i );
        }
        else
        {        
            ph[i].volts   = pz.getVolts();
            ph[i].amps    = pz.getAmps() * scale;
            ph[i].watts   = pz.getWatts() * scale;
            ph[i].error   = getErrorStr( i );
            if( doref[i] )
            {
                refkWh[i] = pz.getkWh() * scale;
                ph[i].kWh  = 0.0;
                doref[i] = false;                
            }
            else
                ph[i].kWh = pz.getkWh() * scale - refkWh[i];
        }
    }    
    void PZENG::readPhase( VAP *vp, int i, float scale )      
    {
        switch( i )
        {
            case 1:
                readPZM( 1, scale );    // this also defines myerror[1];
                *vp = ph[1];
                break;
            case 2:
                readPZM( 2, scale );    // this also defines myerror[2];
                *vp = ph[2]; 
                break;
            default:
            case 3:
                readPZM( 1, scale );    // this could result to an error
                readPZM( 2, scale );    // same for this
                addPhase12();           // this also defines myerror[0]
                *vp = ph[0];
                break;
        }
    } 
    void PZENG::addPhase12()
    {
        ph[0].volts = (ph[1].volts + ph[2].volts)/2.0;
        ph[0].amps  = (ph[1].amps  + ph[2].amps );
        ph[0].watts = (ph[1].watts + ph[2].watts);
        ph[0].kWh   = (ph[1].kWh   + ph[2].kWh);    
        myerror[0]  = myerror[1] + myerror[2];
        ph[0].error = getErrorStr(0);     
    }    
    VAP PZENG::getPhase( int i )  
    { 
        if( (i==1) || (i==2) )
            return ph[ i ];
        else
        {
            addPhase12();
            return ph[0];
        }
    }
    void PZENG::zerokWh()
    {
        doref[0] = doref[1] = doref[2] = true;
    }
    int PZENG::getError( int ph )
    {
        return ((ph==1) || (ph==2)) ? myerror[ph] : myerror[0];
    }
    const char *PZENG::getErrorStr( int ph )
    {
        int i = ((ph==1) || (ph==2) ) ? ph : 0;

        int e = myerror[ i ];
        if( e ) sf( errstr[i], 12, "Ph#%d E%d", ph, e );
        else    sf( errstr[i], 12, "Ph#%d OK", ph);

        return (const char *)errstr[i];
    }
// ----------------------------------- VWSTATS Class -------------------------------------

    VWSTATS::VWSTATS() 
    {
        resetStats();
    }        
    void VWSTATS::updateStats( VAP *n )
    {
        VAP *n0 = n++;
        VAP *n1 = n++;
        VAP *n2 = n++;
        
        if( n1->volts < stat.Vmin )         // compute min Volts
            stat.Vmin = n1->volts;
        if( n2->volts < stat.Vmin )  
            stat.Vmin = n2->volts;
    
        if( n0->watts > stat.Wmax )       // compute max watts
            stat.Wmax = n0->watts;

        if( count == 0 )
        {
            Vsum = n0->volts;
            Wsum = n0->watts;
            stat.Vavg = Vsum;
            stat.Wavg = Wsum;
            count = 1;
        }
        else
        {
            count++;
            Vsum += n0->volts;
            Wsum += n0->watts;
            stat.Vavg = Vsum/count;
            stat.Wavg = Wsum/count;
        }
        stat.kWh = n0->kWh;
    }
    STATS VWSTATS::getStats()
    {
        return stat;
    }
    void VWSTATS::resetStats()
    {
        stat.Vmin = 200.0; 
        stat.Vavg = 0.0;
        stat.Wmax = 0.0;
        stat.Wavg = 0.0;
        stat.kWh  = 0.0;
        Vsum = 0.0;
        Wsum = 0.0;
        count = 0;
    }
    const char *VWSTATS::getVStats()
    {
        static char temp[32];
        return (const char *) sf( temp, sizeof( temp ), 
            "avg:%.1fV, min:%.1fV", stat.Vavg, stat.Vmin);
    }
    const char *VWSTATS::getWStats()
    {
        static char temp[32];
        return (const char *) sf( temp, sizeof( temp ), 
            "avg:%.2fkW, max:%.2fkW", stat.Wavg/1000.0, stat.Wmax/1000.0 );
    }
// --------------------------------- ALARMS ----------------------------

    ALARM::ALARM()
    {
        resetUVAlarm();
        resetOPAlarm();
        Vthrs = Wthrs = DISABLE_ALARM;
    }
    void ALARM::setUVthrs( float value )
    {
        Vthrs = value;
    }
    void ALARM::setOPthrs( float value )
    {
        Wthrs = value;
    }
    void ALARM::updateAlarms( VAP *n )
    {
        float n0watts = n->watts; n++;
        float n1volts = n->volts; n++;
        float n2volts = n->volts;
        
        if( Vthrs >=0.0 )
        {
            switch( stateV )
            {
                default:
                case 0:
                    alarmV = false;
                    alarmVT[0]=0;
                    triggerV = 0;
                    if( n1volts>90.1 && n2volts>90.1 && ((n1volts < Vthrs) || (n2volts < Vthrs)) )
                    {
                        stateV   = 1;
                        triggerV = 1;
                        alarmV   = true;
                        alarmVT0 = tmb.getSecCount();   // mark the beginning of alarm
                        
                        sf( alarmVT, sizeof( alarmVT ),"UV BG %s", tmb.getTime() );
                    }
                    break;
                case 1:                    
                    triggerV = 0;
                    sf( alarmVT, sizeof( alarmVT ),"UV CT %s", tmb.getTime() );
                    if( !((n1volts < Vthrs) || (n2volts < Vthrs)) )
                    {
                        stateV   = 0;
                        triggerV = 2;
                        sf( alarmVT, sizeof( alarmVT ),"UV DT %ds", tmb.getSecCount()-alarmVT0 );
                    }
                    break;
            }
        }
        if( Wthrs >=0.0 )
        {
//            PFN("Wthrs=%.0f, W=%.0f State=%d, alarmW=%d",
//                Wthrs, n0watts, stateW, alarmW );
            switch( stateW )
            {
                default:
                case 0:
                    alarmW = false;
                    alarmWT[0]=0;
                    triggerW = 0;
                    if( n0watts > Wthrs )
                    {
                        stateW = 1;
                        triggerW = 1;
                        alarmW = true;
                        alarmWT0 = tmb.getSecCount();
                        sf( alarmWT, sizeof( alarmWT ),"OP BG %s", tmb.getTime() );
                    }
                    break;
                case 1:
                    triggerW = 0;
                    sf( alarmWT, sizeof( alarmWT ),"OP CT %s", tmb.getTime() );
                    if( !(n0watts > Wthrs) )
                    {
                        stateW = 0;
                        triggerW = 2;
                        sf( alarmWT, sizeof( alarmWT ),"OP DT %ds", tmb.getSecCount()-alarmWT0 );
                    }
                    break;
            } 
        }  
    }
    int ALARM::triggeredUV()
    {
        return triggerV;
    }
    int ALARM::triggeredOP()
    {
        return triggerW;
    }
    const char *ALARM::getAlarms()
    {
        static char m[32];
        if( alarmV && alarmW )
            return (const char *)sf( m, 32, "%s %s", alarmVT, alarmWT );  
        else if( alarmV )
            return (const char *)alarmVT;
        else if( alarmW )
            return (const char *)alarmWT;
        else
            return "All OK";
    }
    const char *ALARM::getUVAlarm()
    {
        if( alarmV )
            return (const char *)alarmVT;
        else
            return "Voltage OK";
    }
    const char *ALARM::getOPAlarm()
    {
        if( alarmW )
            return (const char *)alarmWT;
        else
            return "Power OK";
    }
    void ALARM::resetUVAlarm() 
    {
        stateV      = 0;
        triggerV    = 0;
        alarmV      = false;
        alarmVT[0]  = 0;
    }
    void ALARM::resetOPAlarm() 
    {
        stateW      = 0;
        triggerW    = 0;
        alarmW      = false;
        alarmWT[0]  = 0;
    }
// --------------------------------- Time Base -------------------------

    TOCsec::TOCsec( uint32 sec )
    {
        _seccnt = 0;
        _secrdy = false;
        _secmax = sec;
    }
    void TOCsec::setModulo( uint32 sec )
    {
        _secmax = sec;
    }
    void TOCsec::update()       // to be executed every sec
    {
        _seccnt ++;
        if( _seccnt >= _secmax )
        {
            _seccnt = 0;
            _secrdy = true;
        }
    }
    void TOCsec::setSecCount( uint32 secref )
    {
        _seccnt = secref % _secmax;     // sets count to modulo
    }
    bool TOCsec::ready()
    {
        if( _secrdy )
            return !(_secrdy = false);
        return false;
    }    
// -------------------------------------------------------
    
    TMBASE::TMBASE()
    {
        setHMS();   // all zeros by default
    }        
    void TMBASE::setHMS( int hrs0, int min0, int sec0 )
    {
        _sec = sec0; _min=min0; _hrs=hrs0;
    }
    uint32 TMBASE::getSecCount()
    {        
        return 3600*_hrs + 60*_min + _sec;
    }
    void TMBASE::update()
    {
        _sec ++;
        if( _sec >= 60 )
        {
            _sec = 0;
            _min++;
            if( _min >= 60 )
            {
                _min = 0;
                _hrs++;
                if( _hrs >= 24 )
                {
                    _hrs = 0;
                }
            }
            
        }        
    }
    const char *TMBASE::getTime()
    {
        static char temp[32];   // must be static
        return (const char *)sf( temp, sizeof( temp ), "%02d:%02d:%02d", _hrs, _min, _sec );
    }
