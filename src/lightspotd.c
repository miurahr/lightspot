/*
 * LightSpotd -- spotlight presentation pointer user space driver
 *
 * Copyright 2018, Hiroshi Miura
 *
 *
 * Some part of source is borrowed from HIDAPI
 * Copyright 2009-2010, Alan Ott, Signal 11 Software
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "libusb.h"

const unsigned char* reset_cmd =
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00";

const unsigned char* start_cmd =
        (const unsigned char *) "\x10\xff\x80\x00\x00\x01\x00";

const unsigned char* init_cmds[] = {
        (const unsigned char *) "\x10\xff\x81\x02\x00\x00\x00",
        (const unsigned char *) "\x10\xff\x80\x02\x02\x00\x00",
};

const unsigned char* config_cmds[] = {
        (const unsigned char *) "\x10\xff\x00\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\xff\x81\xf1\x01\x00\x00",
        (const unsigned char *) "\x10\xff\x81\xf1\x02\x00\x00",
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x01\x06\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",
        (const unsigned char *) "\x10\xff\x80\x00\x00\x00\x00",
};

const unsigned char* cmds3[] = {
        (const unsigned char *) "\x10\x01\x00\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x00\x1e\x00\x00\xc0",
        (const unsigned char *) "\x10\x01\x00\x0e\x00\x30\x00",



        (const unsigned char *) "\x10\x01\x00\x0e\x1d\x4b\x00",
        (const unsigned char *) "\x10\x01\x00\x0e\x00\x10\x00",

        (const unsigned char *) "\x10\x01\x00\x0e\x00\x01\x00",
        (const unsigned char *) "\x10\x01\x01\x0e\x00\x00\x00",
};

const unsigned char* cmd_poll =
        (const unsigned char *) "\x10\x01\x01\x1e\x00\x00\x00";

void printhex(unsigned char *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        fprintf(stderr, " %02x", buf[i]);
    }
    fprintf(stderr, "\n");
}

typedef struct lightspot_device_ {
    libusb_device *device;
    libusb_device_handle *handle;
} lightspot_device;

static lightspot_device *new_device() {
    lightspot_device *dev = (lightspot_device *)(calloc(1, sizeof(lightspot_device)));
    return dev;
}

static void free_device(lightspot_device *dev) {
    free(dev);
}

lightspot_device *lightspot_find_device(libusb_context *ctx) {
    libusb_device **list;
    struct libusb_device_descriptor desc;
    ssize_t cnt;
    int res=0;
    lightspot_device *dev;

    cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) {
        return NULL;
    }

    for (int i = 0; i < cnt; i++) {
        int r = libusb_get_device_descriptor(list[i], &desc);
        if (r < 0) {
            break;
        }
        if (desc.idVendor == 0x046d &&
            desc.idProduct == 0xc53e) {

            // Check device
            struct libusb_config_descriptor *config;
            libusb_get_config_descriptor(list[i], 0, &config);
            if ((int) config->bNumInterfaces == 3) {
                const struct libusb_interface *inter;
                const struct libusb_interface_descriptor *interdesc;
                const struct libusb_endpoint_descriptor *epdesc;

                inter = &config->interface[2];
                interdesc = &inter->altsetting[0];
                epdesc = &interdesc->endpoint[0];
                if ((int) interdesc->bInterfaceNumber == 2 &&
                    (int) interdesc->bInterfaceClass == LIBUSB_CLASS_HID &&
                    (int) interdesc->bInterfaceSubClass == 0 &&
                    (int) epdesc->bDescriptorType == 5 &&
                    (int) epdesc->bEndpointAddress == 0x83) {
                    fprintf(stderr, "Found device.\n");
                    dev = new_device();
                    dev->device = list[i];
                    res=1;
                }
            }
            libusb_free_config_descriptor(config);
        }
    }
    libusb_free_device_list(list, 1);

    if (res) {
        return dev;
    } else {
        return NULL;
    }
}

int lightspot_device_connect(lightspot_device *dev) {
    int res;
    libusb_device *device = dev->device;
    libusb_device_handle *handle;

    res = libusb_open(device, &handle);
    if (res == 0) {
        res = libusb_kernel_driver_active(handle, 2);
        if (res == 1) {
            res = libusb_detach_kernel_driver(handle, 2);
			if( res != 0 )
			{
				return res;
			}
        }
        fprintf(stderr, "Successfully open the device.\n");
        dev->handle = handle;
    }
    return res;
}

int lightspot_write(lightspot_device *dev, const unsigned char *data, unsigned int length) {
    int res;
    unsigned int report_number = data[0];
    int skipped_report_id = 0;

	if (report_number == 0x0) {
		data++;
		length--;
		skipped_report_id = 1;
	}

    res = libusb_control_transfer(dev->handle,
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID Set_Report*/,
                                  (2/*HID output*/ << 8) | report_number,
                                  2,
                                  (unsigned char*)data,
                                  (uint16_t) length,
                                  1000/*timeout millis*/);
    if (res < 0)
        return -1;

    if (skipped_report_id)
	    length++;

    return length;
}

int lightspot_read(lightspot_device *dev, unsigned char *buf, int length)
{
	int actual;

	libusb_interrupt_transfer(dev->handle, 0x83, buf, length, &actual, 5000);
	return actual;
}

void lightspot_init(lightspot_device *dev) {
    int res;
    unsigned char buf[65];

    fprintf(stderr, "send reset_cmd:");
    lightspot_write(dev, reset_cmd, 7);
    res = lightspot_read(dev, buf, 65);
    printhex(buf, res);

    fprintf(stderr, "send start_cmd:");
    lightspot_write(dev, start_cmd, 7);
    res = lightspot_read(dev, buf, 65);
    printhex(buf, res);

    for (int i=0; i<2; i++) {
        fprintf(stderr, "send init_cmd:");
        lightspot_write(dev, init_cmds[i], 7);
        res = lightspot_read(dev, buf, 65);
        printhex(buf, res);
    }

    for(int i=0; i<12; i++) {
        fprintf(stderr, "send config_cmd:");
        lightspot_write(dev, config_cmds[i], 7);
        res = lightspot_read(dev, buf, 65);
        printhex(buf, res);
    }

}

void handle_linux_ev(unsigned char *buf, int len) {
    // TODO: implement me.
    printhex(buf, len);
}

void lightspot_device_disconnect(lightspot_device *dev) {
    libusb_close(dev->handle);
    free_device(dev);
}

int main(int argc, char *argv[]) {
    int res,i;
    libusb_context *ctx;
    lightspot_device *dev;
    unsigned char buf[65];

    res = libusb_init(&ctx);
    if (res < 0) {
        exit(1);
    }

    for(i=0; i<65536; i++) { // fixme: exit with signal
        dev = lightspot_find_device(ctx);
        if (dev) {
            res =lightspot_device_connect(dev);
            if (res== 0) {
                lightspot_init(dev);
                /*
                for (uint16_t s = 0; s < 256; s++) {
                    unsigned char cmd[7];
                    memcpy((void *)cmd_poll, cmd, sizeof(cmd));
                    cmd[4] = (unsigned char) (s & 0xFF);
                    lightspot_write(dev, cmd, 7);
                    res = lightspot_read(dev, buf, 65);
                    if (res < 0) {
                        // error, probably removed an usb device.
                        break;
                    } else if (res > 0) {
                        handle_linux_ev(buf, res);
                    }
                    usleep(20000);
                }
                */
                lightspot_device_disconnect(dev);
                break;
            } else {
                fprintf(stderr, "Device open error: %d.\n", res);
                break;
            }
        } else {
            usleep(3000 * 1000);
        }
    }
    libusb_exit(ctx);
}
