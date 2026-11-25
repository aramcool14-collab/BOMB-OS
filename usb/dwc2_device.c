#include "dwc2_device.h"

/*
 * BOMB OS USB device driver for the BCM2835 DWC2 controller.
 * Polling-mode, full-speed CDC-ACM device. No DMA and no IRQ setup required.
 * The host sees a standard USB serial device (Windows usbser.sys).
 */
#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define BASE 0x20980000u
#define R(off) REG(BASE + (off))
#define GAHBCFG R(0x008)
#define GUSBCFG R(0x00C)
#define GRSTCTL R(0x010)
#define GINTSTS R(0x014)
#define GINTMSK R(0x018)
#define GRXSTSP R(0x020)
#define GRXFSIZ R(0x024)
#define GNPTXFSIZ R(0x028)
#define DCFG R(0x800)
#define DCTL R(0x804)
#define DSTS R(0x808)
#define DIEPMSK R(0x810)
#define DOEPMSK R(0x814)
#define DAINT R(0x818)
#define DAINTMSK R(0x81C)
#define DIEPCTL(n) REG(BASE+0x900u+(n)*0x20u)
#define DIEPINT(n) REG(BASE+0x908u+(n)*0x20u)
#define DIEPTSIZ(n) REG(BASE+0x910u+(n)*0x20u)
#define DTXFSTS(n) REG(BASE+0x918u+(n)*0x20u)
#define DOEPCTL(n) REG(BASE+0xB00u+(n)*0x20u)
#define DOEPINT(n) REG(BASE+0xB08u+(n)*0x20u)
#define DOEPTSIZ(n) REG(BASE+0xB10u+(n)*0x20u)
#define FIFO(n) REG(BASE+0x1000u+(n)*0x1000u)

#define GINT_RXFLVL (1u<<4)
#define GINT_USBRST (1u<<12)
#define GINT_ENUMDNE (1u<<13)
#define GINT_IEPINT (1u<<18)
#define GINT_OEPINT (1u<<19)
#define EP_USBAEP (1u<<15)
#define EP_EPTYPE_BULK (2u<<18)
#define EP_TXFNUM(n) ((uint32_t)(n)<<22)
#define EP_CNAK (1u<<26)
#define EP_SNAK (1u<<27)
#define EP_EPENA (1u<<31)
#define EP_XFRC (1u<<0)
#define DCTL_SDIS (1u<<1)

#define USB_REQ_GET_STATUS 0u
#define USB_REQ_CLEAR_FEATURE 1u
#define USB_REQ_SET_FEATURE 3u
#define USB_REQ_SET_ADDRESS 5u
#define USB_REQ_GET_DESCRIPTOR 6u
#define USB_REQ_GET_CONFIGURATION 8u
#define USB_REQ_SET_CONFIGURATION 9u
#define USB_REQ_GET_INTERFACE 10u
#define USB_REQ_SET_INTERFACE 11u
#define USB_REQ_SET_LINE_CODING 0x20u
#define USB_REQ_GET_LINE_CODING 0x21u
#define USB_REQ_SET_CONTROL_LINE_STATE 0x22u

static const uint8_t device_desc[] = {
    18,1,0x00,0x02,0,0,0,64, 0x09,0x12, 0x01,0xB0, 0x00,0x01, 1,2,3,1
};
static const uint8_t config_desc[] = {
    9,2,67,0,2,1,0,0x80,50,
    9,4,0,0,1,2,2,1,0,
    5,0x24,0,0x10,0x01,
    5,0x24,1,0,1,
    4,0x24,2,2,
    5,0x24,6,0,1,
    7,5,0x81,3,8,0,16,
    9,4,1,0,2,0x0A,0,0,0,
    7,5,0x02,2,64,0,0,
    7,5,0x82,2,64,0,0
};
static const uint8_t lang_desc[] = {4,3,0x09,0x04};
static const uint8_t mfg_desc[] = {16,3,'B',0,'O',0,'M',0,'B',0,' ',0,'O',0,'S',0};
static const uint8_t product_desc[] = {22,3,'B',0,'O',0,'M',0,'B',0,' ',0,'O',0,'S',0,' ',0,'P',0,'i',0};
static const uint8_t serial_desc[] = {10,3,'0',0,'0',0,'0',0,'1',0};

static volatile int usb_ready, configured, ep2_in_done;
static uint8_t pending_address, pending_action;
static uint8_t setup_packet[8], out_packet[64];
static uint8_t line_coding[7] = {0x00,0xC2,0x01,0x00,0,0,8};
static const uint8_t *ctrl_data;
static uint16_t ctrl_remaining;
static uint8_t ctrl_need_zlp, ctrl_data_active;
static uint8_t ep0_out_expected, ep0_out_received;

#define SYST_CLO REG(0x20003004u)
static void delay_us(uint32_t us) {
    uint32_t start=SYST_CLO;
    while((uint32_t)(SYST_CLO-start)<us) { }
}
static int wait_reg(uint32_t off,uint32_t mask,uint32_t val) {
    unsigned n=3000000u;
    while(n--) if((R(off)&mask)==val) return 1;
    return 0;
}
static void fifo_write(uint8_t ep,const uint8_t *p,uint32_t n) {
    while(n) {
        uint32_t v=0, take=n>4?4:n;
        for(uint32_t i=0;i<take;i++) v|=(uint32_t)p[i]<<(8*i);
        FIFO(ep)=v; p+=take; n-=take;
    }
}
static void fifo_read(uint8_t ep,uint8_t *p,uint32_t n) {
    while(n) {
        uint32_t v=FIFO(ep), take=n>4?4:n;
        for(uint32_t i=0;i<take;i++) p[i]=(uint8_t)(v>>(8*i));
        p+=take; n-=take;
    }
}
static void arm_setup(void) {
    DOEPTSIZ(0)=(3u<<29)|(1u<<19)|24u;
    DOEPCTL(0)|=EP_CNAK|EP_EPENA;
}
static void arm_ep0_status_out(void) {
    DOEPTSIZ(0)=(1u<<19)|1u;
    DOEPCTL(0)|=EP_CNAK|EP_EPENA;
}
static void ep0_send_packet(const uint8_t *p,uint32_t n) {
    if(n>64)n=64;
    DIEPTSIZ(0)=(1u<<19)|n;
    DIEPCTL(0)|=EP_CNAK|EP_EPENA;
    if(n) fifo_write(0,p,n);
}
static void ep0_status_in(void) { ep0_send_packet(0,0); }
static void ep0_send_next(void) {
    uint32_t n=ctrl_remaining>64?64:ctrl_remaining;
    if(n) {
        ep0_send_packet(ctrl_data,n);
        ctrl_data+=n; ctrl_remaining=(uint16_t)(ctrl_remaining-n);
        ctrl_data_active=1;
    } else if(ctrl_need_zlp) {
        ctrl_need_zlp=0; ep0_send_packet(0,0); ctrl_data_active=1;
    } else {
        ctrl_data_active=0; arm_ep0_status_out();
    }
}
static void ep0_stall(void) {
    DIEPCTL(0)|=(1u<<21);
    DOEPCTL(0)|=(1u<<21);
    arm_setup();
}
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|((uint16_t)p[1]<<8)); }
static void start_control_data(const uint8_t *p,uint16_t length,uint16_t requested) {
    uint16_t n=length<requested?length:requested;
    ctrl_data=p; ctrl_remaining=n;
    ctrl_need_zlp=(n!=0 && n<requested && (n%64u)==0);
    ep0_send_next();
}
static void activate_data_endpoints(void) {
    static const uint8_t serial_state[10] = {
        0xA1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00
    };
    /* Dedicated FIFO 2 carries the BOMB-LINK byte stream. */
    R(0x104)=(16u<<16)|256u;
    R(0x108)=(128u<<16)|272u;
    DIEPCTL(1)=EP_USBAEP|(3u<<18)|EP_TXFNUM(1)|8u;
    DIEPCTL(2)=EP_USBAEP|EP_EPTYPE_BULK|EP_TXFNUM(2)|64u;
    DOEPCTL(2)=EP_USBAEP|EP_EPTYPE_BULK|64u;
    DIEPTSIZ(2)=0; DOEPTSIZ(2)=64u|(1u<<19);
    DOEPCTL(2)|=EP_CNAK|EP_EPENA;
    DAINTMSK|=(1u<<1)|(1u<<2)|(1u<<(16+2));
    DIEPTSIZ(1)=(2u<<19)|sizeof(serial_state);
    DIEPCTL(1)|=EP_CNAK|EP_EPENA;
    fifo_write(1,serial_state,sizeof(serial_state));
}
static void set_configuration(uint8_t value) {
    configured=(value==1);
    if(configured) activate_data_endpoints();
}
static void handle_setup(void) {
    uint8_t bm=setup_packet[0], req=setup_packet[1];
    uint16_t value=u16(&setup_packet[2]), index=u16(&setup_packet[4]), length=u16(&setup_packet[6]);
    pending_action=0;
    ctrl_data_active=0;
    switch(req) {
    case USB_REQ_GET_DESCRIPTOR: {
        const uint8_t *p=0; uint16_t n=0;
        uint8_t type=(uint8_t)(value>>8), id=(uint8_t)value;
        if(type==1){p=device_desc;n=sizeof(device_desc);}
        else if(type==2){p=config_desc;n=sizeof(config_desc);}
        else if(type==3){
            if(id==0){p=lang_desc;n=sizeof(lang_desc);}
            else if(id==1){p=mfg_desc;n=sizeof(mfg_desc);}
            else if(id==2){p=product_desc;n=sizeof(product_desc);}
            else if(id==3){p=serial_desc;n=sizeof(serial_desc);}
        }
        if(p) start_control_data(p,n,length); else ep0_stall();
        break;
    }
    case USB_REQ_SET_ADDRESS:
        pending_address=(uint8_t)(value&0x7F); pending_action=1; ep0_status_in(); break;
    case USB_REQ_SET_CONFIGURATION:
        pending_address=(uint8_t)value; pending_action=2; ep0_status_in(); break;
    case USB_REQ_GET_CONFIGURATION: {
        static uint8_t cfg; cfg=(uint8_t)configured; start_control_data(&cfg,1,length); break;
    }
    case USB_REQ_GET_STATUS: {
        static const uint8_t zero[2]={0,0}; start_control_data(zero,2,length); break;
    }
    case USB_REQ_GET_INTERFACE: {
        static const uint8_t alt=0; start_control_data(&alt,1,length); break;
    }
    case USB_REQ_SET_INTERFACE:
    case USB_REQ_CLEAR_FEATURE:
    case USB_REQ_SET_FEATURE:
    case USB_REQ_SET_CONTROL_LINE_STATE:
        ep0_status_in(); break;
    case USB_REQ_SET_LINE_CODING:
        if(length==7) { ep0_out_expected=7; ep0_out_received=0; DOEPTSIZ(0)=(1u<<19)|7u; DOEPCTL(0)|=EP_CNAK|EP_EPENA; }
        else ep0_stall();
        break;
    case USB_REQ_GET_LINE_CODING:
        start_control_data(line_coding,7,length); break;
    default:
        (void)bm; (void)index; ep0_stall(); break;
    }
}
static void process_rx(void) {
    uint32_t st=GRXSTSP;
    uint8_t ep=(uint8_t)(st&0xFu);
    uint32_t count=(st>>4)&0x7FFu;
    uint32_t pkt=(st>>17)&0xFu;
    if(pkt==6u && ep==0u) {
        uint8_t b[8]={0}; if(count>=8) fifo_read(0,b,8); else if(count) fifo_read(0,b,count);
        for(unsigned i=0;i<8;i++) setup_packet[i]=b[i];
        handle_setup(); if(!ep0_out_expected) arm_setup();
    } else if(pkt==2u) {
        if(ep==0u) {
            uint32_t n=count>sizeof(out_packet)?sizeof(out_packet):count;
            if(n) fifo_read(0,out_packet,n);
            if(ep0_out_expected && ep0_out_received+n<=7u) {
                for(uint32_t i=0;i<n;i++) line_coding[ep0_out_received+i]=out_packet[i];
                ep0_out_received=(uint8_t)(ep0_out_received+n);
                if(ep0_out_received>=ep0_out_expected) { ep0_out_expected=0; ep0_status_in(); }
            }
        } else {
            uint8_t sink[64]; uint32_t n=count>64?64:count; if(n) fifo_read(0,sink,n);
            DOEPTSIZ(2)=64u|(1u<<19); DOEPCTL(2)|=EP_CNAK|EP_EPENA;
        }
    } else if(count) {
        uint8_t sink[64]; uint32_t n=count>64?64:count; fifo_read(0,sink,n);
    }
}
static void service_interrupts(void) {
    uint32_t pending=GINTSTS&GINTMSK;
    if(pending&GINT_USBRST) {
        GINTSTS=GINT_USBRST; DCFG&=~(0x7Fu<<4); configured=0;
        DIEPCTL(0)=EP_USBAEP|64u; DOEPCTL(0)=EP_USBAEP|64u;
        arm_setup();
    }
    if(pending&GINT_ENUMDNE) GINTSTS=GINT_ENUMDNE;
    if(pending&GINT_RXFLVL) {
        while(GINTSTS&GINT_RXFLVL) process_rx();
    }
    if(pending&GINT_IEPINT) {
        uint32_t da=DAINT&DAINTMSK&0xFFFFu;
        for(unsigned ep=0;ep<4;ep++) if(da&(1u<<ep)) {
            uint32_t flags=DIEPINT(ep); DIEPINT(ep)=flags;
            if(flags&EP_XFRC) {
                if(ep==0) {
                    if(pending_action==1) { DCFG=(DCFG&~(0x7Fu<<4))|((uint32_t)pending_address<<4); pending_action=0; arm_setup(); }
                    else if(pending_action==2) { set_configuration((uint8_t)pending_address); pending_action=0; arm_setup(); }
                    else if(ctrl_data_active) ep0_send_next();
                    else arm_setup();
                } else if(ep==2) ep2_in_done=1;
            }
        }
    }
    if(pending&GINT_OEPINT) {
        uint32_t da=(DAINT&DAINTMSK)>>16;
        for(unsigned ep=0;ep<4;ep++) if(da&(1u<<ep)) {
            uint32_t flags=DOEPINT(ep); DOEPINT(ep)=flags;
            if(ep==0 && (flags&EP_XFRC) && !ep0_out_expected) arm_setup();
            if(ep==2 && (flags&EP_XFRC)) { DOEPTSIZ(2)=64u|(1u<<19); DOEPCTL(2)|=EP_CNAK|EP_EPENA; }
        }
    }
}
int bomb_usb_init(void) {
    usb_ready=0; configured=0; ep2_in_done=0; pending_action=0;
    DCTL|=DCTL_SDIS;
    if(!wait_reg(0x010,1u<<31,1u<<31)) return 0;
    GRSTCTL|=1u;
    if(!wait_reg(0x010,1u,0)) return 0;
    delay_us(1000u);
    /*
     * Force peripheral mode after the core reset.  Do not assume the
     * mode switch completed just because a delay elapsed: GINTSTS.CMOD
     * reports the mode actually selected by the DWC2 core.
     */
    GUSBCFG=(GUSBCFG&~((1u<<29)|(1u<<30)))|(1u<<30);
    delay_us(25000u);
    if(!wait_reg(0x014,1u,0u)) return 0; /* GINTSTS.CMOD: 0=device */
    GRSTCTL=(1u<<5); if(!wait_reg(0x010,1u<<5,0)) return 0;
    GRSTCTL=(1u<<4); if(!wait_reg(0x010,1u<<4,0)) return 0;
    DCFG=(DCFG&~7u)|3u;
    GRXFSIZ=128u; GNPTXFSIZ=(128u<<16)|128u;
    DIEPMSK=EP_XFRC; DOEPMSK=EP_XFRC;
    DAINTMSK=1u|(1u<<16);
    GINTSTS=0xFFFFFFFFu;
    GINTMSK=GINT_RXFLVL|GINT_USBRST|GINT_ENUMDNE|GINT_IEPINT|GINT_OEPINT;
    DIEPCTL(0)=EP_USBAEP|64u; DOEPCTL(0)=EP_USBAEP|64u;
    arm_setup();
    usb_ready=1;
    DCTL&=~DCTL_SDIS;
    GAHBCFG|=1u;
    return 1;
}
void bomb_usb_poll(void) { if(usb_ready) service_interrupts(); }
int bomb_usb_configured(void) { return configured; }
int bomb_usb_write(const uint8_t *data,size_t length) {
    if(!configured || !data) return -1;
    while(length) {
        uint32_t n=length>64u?64u:(uint32_t)length;
        unsigned timeout=5000000u;
        ep2_in_done=0;
        DIEPTSIZ(2)=(1u<<19)|n;
        DIEPCTL(2)|=EP_CNAK|EP_EPENA;
        fifo_write(2,data,n);
        while(!ep2_in_done && timeout--) bomb_usb_poll();
        if(!timeout) return -2;
        data+=n; length-=n;
    }
    return 0;
}
