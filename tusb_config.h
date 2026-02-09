#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU                OPT_MCU_RP2040
#define CFG_TUSB_RHPORT0_MODE      OPT_MODE_DEVICE

#define CFG_TUD_ENDPOINT0_SIZE     64

#define CFG_TUD_CDC                1
#define CFG_TUD_MSC                0
#define CFG_TUD_HID                0
#define CFG_TUD_MIDI               0
#define CFG_TUD_VENDOR             0

#define CFG_TUD_CDC_RX_BUFSIZE     1024
#define CFG_TUD_CDC_TX_BUFSIZE     1024

#ifdef __cplusplus
}
#endif

#endif