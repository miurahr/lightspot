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
#include "hidapi.h"

#define MAX_STR 255

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
};

const unsigned char* config2_cmds[] = {
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x01\x06\x0e\x00\x00\x00",
};

const unsigned char* cmds3[] = {
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",
        (const unsigned char *) "\x10\xff\x80\x00\x00\x00\x00",
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


void lightspot_init(hid_device *dev) {
    int res;
    unsigned char buf[65];

    fprintf(stderr, "send reset_cmd:");
    hid_write(dev, reset_cmd, 7);
    res = hid_read(dev, buf, 65);
    printhex(buf, res);

    fprintf(stderr, "send start_cmd:");
    hid_write(dev, start_cmd, 7);
    res = hid_read(dev, buf, 65);
    printhex(buf, res);

    for (int i=0; i<2; i++) {
        fprintf(stderr, "send init_cmd:");
        hid_write(dev, init_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }
    res = hid_read(dev, buf, 65);
    printhex(buf, res);

    for(int i=0; i<4; i++) {
        fprintf(stderr, "send config_cmd:");
        hid_write(dev, config_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }
    for(int i=0; i<6; i++) {
        fprintf(stderr, "send config2_cmd:");
        hid_write(dev, config2_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }
}

void handle_linux_ev(unsigned char *buf, int len) {
    // TODO: implement me.
    printhex(buf, len);
}

int main(int argc, char *argv[]) {
    int res,i;
    hid_device *dev;
    unsigned char buf[65];

    res = hid_init();
    if (res < 0) {
        exit(1);
    }
    for (;;) {
        dev = lightspot_device_connect();
        if (dev) {
            lightspot_init(dev);
            for(int s=0; s<256; s++) {
                unsigned char cmd[7];
                memcpy((void *) cmd_poll, cmd, sizeof(cmd));
                cmd[4] = (unsigned char) (s & 0xFF);
                hid_write(dev, cmd, 7);
                res = hid_read(dev, buf, 65);
                handle_linux_ev(buf, res);
            }
            hid_close(dev);
        } else {
            usleep(3000*1000);
        }
    }
}

