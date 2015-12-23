//Author: Zhu Di
//vm-app.h
//  Created by Judy on 5/7/15.
//
//

#include <iostream>
#include <fstream>
#include <stdlib.h>     /* atoi */
#include <math.h>    /*pow*/
#include <string>


#include "timer-handler.h"
#include "packet.h"
#include "app.h"
#include "vm-udp.h"

#define ack_intervel 1.0  // 1s
#define input_intervel 1.0  // 1s
#define target_rate_intervel 1.0 //1s
#define alpha 0.7

class VmApp;


// Sender uses this timer to
// schedule next app data packet transmission time
class SendTimer : public TimerHandler {
public:
    SendTimer(VmApp* t) : TimerHandler(), t_(t) {}
    inline virtual void expire(Event*);
protected:
    VmApp* t_;
};


// Reciver uses this timer to schedule
// next ack packet transmission time
class AckTimer : public TimerHandler {
public:
    AckTimer(VmApp* t) : TimerHandler(), t_(t) {}
    inline virtual void expire(Event*);
protected:
    VmApp* t_;
};

class InputTimer : public TimerHandler {
public:
    InputTimer(VmApp* t) : TimerHandler(), t_(t) {}
    inline virtual void expire(Event*);
protected:
    VmApp* t_;
};

class TarRateTimer : public TimerHandler {
public:
    TarRateTimer(VmApp* t) : TimerHandler(), t_(t) {}
    inline virtual void expire(Event*);
protected:
    VmApp* t_;
};


class VmApp : public Application{
public:
    VmApp();
    void send_vm_pkt();  // called by SendTimer:expire (Sender)
    void handle_input_med(); // called by InputTimer:expire (Sender)
    void handle_input_tar(); //called by InputTarTimer:expire (Receiver)
    void send_ack_pkt(); // called by AckTimer:expire (Receiver)
protected:
    int command(int argc, const char*const* argv);
    void start();       // Start sending data packets (Sender)
    void stop();        // Stop sending data packets (Sender)
private:

    inline double next_snd_time();                          // (Sender)
    virtual void recv_msg(int nbytes, const char *msg = 0); // (Sender/Receiver)
    void set_target(const hdr_vm *);                                // (Sender)
    bool set_cumrate(); //estimate cumulative rate of the input media; called by start
    
    double timestamp_;     //from file
    double mediarate_;     //from file
    double cumrate_;     //from file
    
    
    std::string delimiter_ ; //delimiter to parse the input
    std::ifstream iFile_med_;
    std::ifstream iFile_tar_;
    
    char filename_med_[100];      //input mediate rate filename
    char filename_tar_[100];     //input target rate filename
    
    double interval_;      // Application data packet transmission interval
    int frame_rate_;        // Frame rate
    int running_;          // If 1 application is running
    int seq_;              // sequence number
    double tarrate_;            //Sender media rate parameter
    double target_rate_;       //Receiver target rate
    
    
    
    SendTimer snd_timer_;  // SendTimer
    AckTimer  ack_timer_;  // AckTimer
    InputTimer ip_timer_;  //InputTimer
    TarRateTimer tar_timer_; //Target Rate Timer
};
