#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "hidapi.h"

#define MAX_STR 255

const unsigned char* reset_cmd =
        (const unsigned char *) "\x10\x10\xff\x81\x00\x00\x00\x00";

const unsigned char* start_cmd =
        (const unsigned char *) "\x10\x10\xff\x80\x00\x00\x01\x00";

const unsigned char* init_cmds[] = {
        (const unsigned char *) "\x10\x10\xff\x81\x02\x00\x00\x00",
        (const unsigned char *) "\x10\x10\xff\x80\x02\x02\x00\x00",
};

const unsigned char* config_cmds[] = {
        (const unsigned char *) "\x10\x10\xff\x00\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x10\xff\x81\xf1\x01\x00\x00",
        (const unsigned char *) "\x10\x10\xff\x81\xf1\x02\x00\x00",
        (const unsigned char *) "\x10\x10\xff\x81\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x02\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x06\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x10\xff\x81\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x10\xff\x80\x00\x00\x00\x00",
};

const unsigned char* cmds3[] = {
        (const unsigned char *) "\x10\x10\x01\x00\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x10\x01\x00\x1e\x00\x00\xc0",
        (const unsigned char *) "\x10\x10\x01\x00\x0e\x00\x30\x00",



        (const unsigned char *) "\x10\x10\x01\x00\x0e\x1d\x4b\x00",
        (const unsigned char *) "\x10\x10\x01\x00\x0e\x00\x10\x00",

        (const unsigned char *) "\x10\x10\x01\x00\x0e\x00\x01\x00",
        (const unsigned char *) "\x10\x10\x01\x01\x0e\x00\x00\x00",
};

unsigned char* cmd_poll = (unsigned char *) "\x10\x10\x01\x01\x1e\x00\x00\x00";

void printhex(unsigned char *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        fprintf(stderr, " %02x", buf[i]);
    }

}

hid_device *lightspot_device_connect() {
    int res;
    wchar_t wstr[MAX_STR];
    hid_device *handle;

    struct hid_device_info *devs, *cur_dev, *target_dev;

    devs = hid_enumerate(0x046d, 0xc53e);
    cur_dev = devs;
    target_dev = NULL;
    while (cur_dev) {
        if (cur_dev->interface_number == 2) {
            target_dev = cur_dev;
        }
        cur_dev = cur_dev->next;
    }
    if (target_dev == NULL) {
        return NULL;
    }

    handle = hid_open_path(target_dev->path);
    if (handle == NULL) {
        fprintf(stderr, "Fail to open a Spotlight USB Receiver device.\n");
        return NULL;
    }
    res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    fprintf(stderr, "Connected to %ls", wstr);
    res = hid_get_product_string(handle, wstr, MAX_STR);
    fprintf(stderr, " %ls\n", wstr);
    hid_free_enumeration(devs);

    return handle;
}

void lightspot_init_device(hid_device *handle) {
    int i,res;
    unsigned char buf[65];

    hid_write(handle, (const unsigned char*)"\x00\x00", 1);
    res = hid_read_timeout(handle, buf, 65, 1000);

    hid_write(handle, reset_cmd, 7);
    res = hid_read_timeout(handle, buf, 65, 1000);
    printhex(buf, res);

    hid_write(handle, start_cmd, 7);
    res = hid_read_timeout(handle, buf, 65, 1000);
    printhex(buf, res);

    for(i=0; i++; i<2) {
        hid_write(handle, init_cmds[i], 7);
        res = hid_read_timeout(handle, buf, 65, 1000);
        printhex(buf, res);
    }

    for(i=0; i++; i<12) {
        hid_write(handle, config_cmds[i], 7);
        res = hid_read_timeout(handle, buf, 65, 1000);
        printhex(buf, res);
    }

}

void handle_linux_ev(hid_device *handle) {
    int res;
    unsigned char buf[65];

    for (;;)  {
        hid_write(handle, cmd_poll, 7);
        res = hid_read_timeout(handle, buf, MAX_STR, 1000);
        if (res < 0) {
            // error, probably removed an usb device.
            return;
        } else if (res > 0) {
            // read data
            // TODO: implement me.
            fprintf(stderr, " Got pointer event. len %d\n", res);
            printhex(buf, res);
        }
    }
}

int main(int argc, char *argv[]) {
    int res;
    hid_device *handle;

    res = hid_init();
    if (res < 0) {
        exit(1);
    }
    for (;;) {
        handle = lightspot_device_connect();
        if (handle) {
            lightspot_init_device(handle);
            handle_linux_ev(handle);
            hid_close(handle);
        } else {
#ifdef WIN32
            sleep(5000);
#else
            usleep(5000*1000);
#endif
        }
    }
    res = hid_exit();
    if (res < 0) {
        exit(1);
    }
}
