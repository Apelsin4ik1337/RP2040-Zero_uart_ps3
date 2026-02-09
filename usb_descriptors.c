#include "tusb.h"
#include "usb_descriptors.h"
#include "pico/unique_id.h"

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_CDC,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x2E8A,
    .idProduct          = 0x000A,
    .bcdDevice          = 0x0100,
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 1
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_CDC,
                       EPNUM_CDC_NOTIF, 8,
                       EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

static char serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
static bool serial_initialized = false;

static void init_serial(void) {
    if (!serial_initialized) {
        pico_unique_board_id_t id;
        pico_get_unique_board_id(&id);
        for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
            sprintf(serial_str + i * 2, "%02X", id.id[i]);
        }
        serial_initialized = true;
    }
}

static char const *string_desc_arr[] = {
    [STRID_LANGID]       = (const char[]){0x09, 0x04},
    [STRID_MANUFACTURER] = "PS3 Debug Tools",
    [STRID_PRODUCT]      = "PS3 SC UART Bridge",
    [STRID_SERIAL]       = "",
    [STRID_CDC]          = "PS3 SC Console",
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == STRID_LANGID) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;

        const char *str;
        if (index == STRID_SERIAL) {
            init_serial();
            str = serial_str;
        } else {
            str = string_desc_arr[index];
        }

        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}