//
//  vm-app.cc
//  
//
//  Created by Judy on 5/7/15.
//
//

#include "vm-app.h"

// VmApp OTcl linkage class
static class VmAppClass : public TclClass {
public:
    VmAppClass() : TclClass("Application/VmApp") {}
    TclObject* create(int, const char*const*) {
        return (new VmApp);
    }
} class_app_vm;


// When snd_timer_ expires call VmApp:send_vm_pkt()
void SendTimer::expire(Event*)
{
    t_->send_vm_pkt();
}

// When ack_timer_ expires call VmApp:send_ack_pkt()
void AckTimer::expire(Event*)
{
    t_->send_ack_pkt();
}

// When ip_timer_ expires call handle_input_med()
void InputTimer::expire(Event*)
{
    t_->handle_input_med();
}

// When tarrate_timer_ expires call handle_input_tar()
void TarRateTimer::expire(Event*){
    t_->handle_input_tar();
}

// Constructor (initialize instances of timers and bind variables)
VmApp::VmApp() : running_(0), snd_timer_(this), ack_timer_(this), ip_timer_(this), tar_timer_(this)
{
    bind("frame_rate_",&frame_rate_);
}


// OTcl command
int VmApp::command(int argc, const char*const* argv){
    
    Tcl& tcl = Tcl::instance();
    
    if (argc == 3) {
        
        if (strcmp(argv[1], "input-rate") == 0) {   //input mediate rate file name
            strcpy(filename_med_,argv[2]);
            return(TCL_OK);
        }
        
        if (strcmp(argv[1], "input-target") == 0) {   //input target rate file name
            strcpy(filename_tar_,argv[2]);
            return(TCL_OK);
        }

        
        if (strcmp(argv[1], "attach-agent") == 0) {
            agent_ = (Agent*) TclObject::lookup(argv[2]); //Agent *agent_; in app.h
            if (agent_ == 0) {
                tcl.resultf("no such agent %s", argv[2]);
                return(TCL_ERROR);
            }
            
            // Make sure the underlying agent support VM
            if(agent_->supportVM()) {
                agent_->enableVM();
            }
            else {
                tcl.resultf("agent \"%s\" does not support VM Application", argv[2]);
                return(TCL_ERROR);
            }
            
            agent_->attachApp(this);
            return(TCL_OK);
        }
    }
    return (Application::command(argc, argv));
}

bool VmApp::set_cumrate(){
    //std::cout<<"reading the cumrate ";
    cumrate_=0;
    if( iFile_med_.is_open( ) )
    {
        std::string strLine;
        double para[7];
        while(getline(iFile_med_, strLine)) {
            
            int i=0;
            std::size_t start = 0U;
            std::size_t end = strLine.find(delimiter_);
            while (end != std::string::npos)
            {
                std::string tmp = strLine.substr(start, end - start);
                para[i++] = atoi(tmp.c_str());
                start = end + delimiter_.length();
                end = strLine.find(delimiter_, start);
            }
            std::string tmp = strLine.substr(start, end);
            para[i] = atoi(tmp.c_str());
            
            cumrate_ += para[3]; //bytes
            
            // schedul next Input time
            ip_timer_.resched(input_intervel);
        }
        cumrate_=cumrate_*8/para[6]; //para[6] is the timestamp in milliseconds
        iFile_med_.clear();
        iFile_med_.seekg(0, ios::beg);
        //std::cout<<cumrate_<<endl;
        return TRUE;
    }
    else{
        return FALSE;
    }
    
}


void VmApp::handle_input_med(){
    
    if( iFile_med_.is_open( ) )
    {
        std::string strLine;
        
        if(getline(iFile_med_, strLine)) {
            
            int i=0;
            double para[7];

            std::size_t start = 0U;
            std::size_t end = strLine.find(delimiter_);
            while (end != std::string::npos)
            {
                std::string tmp = strLine.substr(start, end - start);
                para[i++] = atoi(tmp.c_str());
                start = end + delimiter_.length();
                end = strLine.find(delimiter_, start);
            }
            std::string tmp = strLine.substr(start, end);
            para[i] = atoi(tmp.c_str());
            
            mediarate_ = para[2];   //kbit/s
            
            // schedul next Input time
            ip_timer_.resched(input_intervel);
        }
        else{
            std::cout << "media file is ended" << endl;
        }
    }
    else{
        std::cout << "media file is close" << endl;
    }

}

void VmApp::handle_input_tar(){
    
    if(iFile_tar_.is_open()){
        string strLine2;
        
        if(getline(iFile_tar_, strLine2)) {
            double para;
            para = atoi(strLine2.c_str());
            target_rate_ = para ;  //kbps
            tarrate_ = target_rate_ ;
            std::cout << "handle target_rate_ tarrate" <<tarrate_<<endl;
            tar_timer_.resched(target_rate_intervel);
        }
        else{
            std::cout << "target rate file is finished" << endl;
        }
    }
        else{
            std::cout << "target rate file is closed" <<endl;
        }

}

void VmApp::start(){

    delimiter_ = " ";
    iFile_med_.open(filename_med_); //open file
    iFile_tar_.open(filename_tar_);
    
    seq_ = 0;
    running_ = 1;
   // tarrate_ = target_rate_*1000;
    if(!set_cumrate())
    {
        std::cout << "Cumulative rate couldn't be set. Check that media file exists and is in the right format"<<endl;
        return;
    }
    handle_input_med();
    handle_input_tar();
    tarrate_ = target_rate_;
    std::cout << "target rate" <<target_rate_ <<" tarrate_"<< tarrate_ <<endl;
    
    send_vm_pkt();       //send data
    
}

void VmApp::stop(){
    running_ = 0;
    iFile_tar_.close( );
    iFile_med_.close( );  //close file
}

//Send message to underlying agent
void VmApp::send_vm_pkt(){
    hdr_vm vmh_buff;
    
    if (running_) {
        // the below info is passed to UDPvm agent, which will write it
        // to VM header after packet creation.
        vmh_buff.ack = 0;            // This is a VM packet
        vmh_buff.seq = seq_++;         // VM sequece number
        vmh_buff.time = Scheduler::instance().clock(); // Current time
        
        
//        if (mediarate_>tarrate_) {
//            vmh_buff.mdrate = pow(tarrate_,2) / cumrate_; //Media rate kbps
//        }
//        else{
            vmh_buff.mdrate = (tarrate_ * mediarate_)/cumrate_;    //Media rate kbps
//        }
    
        vmh_buff.nbytes = vmh_buff.mdrate * 1000 / (8 * frame_rate_);  // Size of VM frame (NOT UDP packet size)
        std::cout<<"send_vm_pkg mdrate "<<vmh_buff.mdrate<<" seq "<<vmh_buff.seq<<" size "<<vmh_buff.nbytes<<endl;
        
        agent_->sendmsg(vmh_buff.nbytes, (char*) &vmh_buff);  // send to UDP
        //Applications can access UDP agents via the sendmsg() function in C++
        
        // Reschedule the send_pkt timer
        double next_time_ = next_snd_time();
        if(next_time_ > 0) snd_timer_.resched(next_time_);//resched() from TimeHandler, reschedule a timer

    }
}

// Schedule next data packet transmission time
double VmApp::next_snd_time()
{
    // Recompute interval
    interval_ = (double)1/frame_rate_;
    
    double next_time_ = interval_;
  
    return next_time_;
}

// Receive message from underlying agent
void VmApp::recv_msg(int nbytes, const char *msg)
{
    if(msg) {
        hdr_vm* vmh_buff = (hdr_vm*) msg;
        
        if(vmh_buff->ack == 1) {
            // If received packet is ACK packet
            //set_target(vmh_buff);
        }
        else {
            // If received packet is VM packet
             send_ack_pkt();
        }
    }
}

//// Sender sets the target scale to what reciver notifies
//void VmApp::set_target(const hdr_vm *vmh_buff){
// //   tarrate_ = vmh_buff->mdrate;
//    tarrate_ = target_rate_;
//}

void VmApp::send_ack_pkt(void)
{
    double local_time = Scheduler::instance().clock();
    
    // send ack message
    hdr_vm ack_buf;
    ack_buf.ack = 1;  // this packet is ack packet
    ack_buf.time = local_time;
    ack_buf.nbytes = sizeof(hdr_vm);
   // ack_buf.mdrate = target_rate_;
    

    agent_->sendmsg(ack_buf.nbytes, (char*) &ack_buf); //send to UDP
    
    // schedul next ACK time
    ack_timer_.resched(ack_intervel);
}
