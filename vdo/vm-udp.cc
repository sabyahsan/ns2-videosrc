//
//  vm-udp.cpp
//  
//
//  Created by Judy on 5/12/15.
//
//

#include "vm-udp.h"
#include "rtp.h"

int hdr_vm::offset_;

// Mulitmedia Header Class
static class MultimediaHeaderClass : public PacketHeaderClass {
public:
    MultimediaHeaderClass() : PacketHeaderClass("PacketHeader/Multimedia",
                                                sizeof(hdr_vm)) {
        bind_offset(&hdr_vm::offset_);
    }
} class_vmhdr;

// UdpVmAgent OTcl linkage class
static class UdpVmAgentClass : public TclClass {
public:
    UdpVmAgentClass() : TclClass("Agent/UDP/UDPvm") {}
    TclObject* create(int, const char*const*) {
        return (new UdpVmAgent());
    }
} class_udpvm_agent;

// Constructor (with no arg)
UdpVmAgent::UdpVmAgent() : UdpAgent()
{
    support_vm_ = 0;
    asm_info.seq = -1;
}

// Constructor (with arg)
UdpVmAgent::UdpVmAgent(packet_t type) : UdpAgent(type)
{
    support_vm_ = 0;
    asm_info.seq = -1;
}

// Add Support of Multimedia Application to UdpAgent::sendmsg
void UdpVmAgent::sendmsg(int nbytes, const char* flags)
{
    Packet *p;
    int n, remain;
    
    
    if (size_) { //Agent fixed packet size, send n times
        n = (nbytes/size_ + (nbytes%size_ ? 1 : 0));
        remain = nbytes%size_;
    }
    else
        printf("Error: UDPvm size = 0\n");
    
    if (nbytes == -1) {
        printf("Error:  sendmsg() for UDPvm should not be -1\n");
        return;
    }
    double local_time =Scheduler::instance().clock();
    while (n-- > 0) {
        p = allocpkt();
        if(n==0 && remain>0) hdr_cmn::access(p)->size() = remain;
        else hdr_cmn::access(p)->size() = size_;
        hdr_rtp* rh = hdr_rtp::access(p);
        rh->flags() = 0;
        rh->seqno() = ++seqno_;
        hdr_cmn::access(p)->timestamp() =
        (u_int32_t)(SAMPLERATE*local_time);
        // to eliminate recv to use VM fields for non VM packets
        hdr_vm* vh = hdr_vm::access(p);
        vh->ack = 0;
        vh->seq = 0;
        vh->nbytes = 0;
        vh->time = 0;
        vh->mdrate = 0;
        // vm udp packets are distinguished by setting the ip
        // priority bit to 15 (Max Priority).
        if(support_vm_) {
            hdr_ip* ih = hdr_ip::access(p);
            ih->prio_ = 15;
            if(flags) // VM Seq Num is passed as flags
                memcpy(vh, flags, sizeof(hdr_vm));
        }
        // add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
        if (flags && (0 ==strcmp(flags, "NEW_BURST")))
            rh->flags() |= RTP_M;
        target_->recv(p);  //target_ from Connector
    }
    idle();
}


// Support Packet Re-Assembly and Multimedia Application

void UdpVmAgent::recv(Packet* p, Handler*)
{
    hdr_ip* ih = hdr_ip::access(p);
    int bytes_to_deliver = hdr_cmn::access(p)->size();
    
    // if it is a VM packet (data or ack)
    if(ih->prio_ == 15) {
        if(app_) {  // if VM Application exists is attached, pass the data to the app
            // re-assemble VM Application packet if segmented
            hdr_vm* vh = hdr_vm::access(p);
            if(vh->seq == asm_info.seq)
                asm_info.rbytes += hdr_cmn::access(p)->size();
            else {
                asm_info.seq = vh->seq;
                asm_info.tbytes = vh->nbytes;
                asm_info.rbytes = hdr_cmn::access(p)->size();
            }
            // if fully reassembled, pass the packet to application
            if(asm_info.tbytes == asm_info.rbytes) {
                hdr_vm vh_buf;
                memcpy(&vh_buf, vh, sizeof(hdr_vm));
                app_->recv_msg(vh_buf.nbytes, (char*) &vh_buf);
            }
        }
        Packet::free(p);
    }
    // if it is a normal data packet (not VM data or ack packet)
    else { 
        if (app_) app_->recv(bytes_to_deliver);
        Packet::free(p);
    }
}



