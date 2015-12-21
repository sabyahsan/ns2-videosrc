//
//  vm-udp.h
//  
//
//  Created by Judy on 5/11/15.
//
//

#ifndef _vm_udp_h
#define _vm_udp_h

#include "udp.h"
#include "ip.h"

//Multimedia Header Structure
struct hdr_vm{
    int ack;     // is it ack packet?
    int seq;     // vm sequence number
    int nbytes;  // bytes for vm pkt
    double time; // current time
    double mdrate; //mediate rate
    
    // Packet header access functions
    static int offset_;
    inline static int& offset() { return offset_; }
    inline static hdr_vm* access(const Packet* p) {
        return (hdr_vm*) p->access(offset_);
    }

};


// Used for Re-assemble segmented (by UDP) VM packet
struct asm_vm {
    int seq;     // vm sequence number
    int rbytes;  // currently received bytes
    int tbytes;  // total bytes to receive for VM packet
};

// UdpVmAgent Class definition
class UdpVmAgent : public UdpAgent {
public:
    UdpVmAgent();
    UdpVmAgent(packet_t);
    virtual int supportVM() { return 1; }
    virtual void enableVM() { support_vm_ = 1; }
    virtual void sendmsg(int nbytes, const char *flags = 0);
    void recv(Packet*, Handler*);
protected:
    int support_vm_; // set to 1 if above is VmApp
private:
    asm_vm asm_info; // packet re-assembly information
};

#endif
