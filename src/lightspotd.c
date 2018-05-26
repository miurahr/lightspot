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

const unsigned char* init_cmds[] = {
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",/* reset return 1 */
        (const unsigned char *) "\x10\xff\x80\x00\x00\x01\x00",/* start return 1 */
        (const unsigned char *) "\x10\xff\x81\x02\x00\x00\x00",/* init return 1 */
        (const unsigned char *) "\x10\xff\x80\x02\x02\x00\x00",/* init return 2 */
        (const unsigned char *) "\x10\xff\x00\x1e\x00\x00\x00",/* config */
        (const unsigned char *) "\x10\xff\x81\xf1\x01\x00\x00",/* config */
        (const unsigned char *) "\x10\xff\x81\xf1\x02\x00\x00",/* config */

        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",/* reset return 2 */
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",/* config return 2*/
        (const unsigned char *) "\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",/* config return 1*/
        (const unsigned char *) "\x10\x01\x02\x1e\x01\x00\x00",
        (const unsigned char *) "\x10\x01\x06\x0e\x00\x00\x00",/* return 20(7+13)byte */
        (const unsigned char *) "\x10\x01\x06\x0e\x00\x00\x00",/* return 2, 20(7+13)byte */
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x05\x0e\x00\x00\x00",/* return 20(7+13)byte */
        (const unsigned char *) "\x10\x01\x05\x1e\x1b\x31\x00",/* return 20(7+13)byte */
};

const unsigned char* long_cmds[] = {
        (const unsigned char *) "\x10\x01\x07\x3e\x00\xd8\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00x\00x\00x\00",
        (const unsigned char *) "\x11\x01\x07\x3e\x00\xdf\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00x\00x\00x\00",
        (const unsigned char *) "\x11\x01\x07\x3e\x00\xda\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00x\00x\00x\00",
        (const unsigned char *) "\x11\x01\x07\x3e\x00\xdc\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00x\00x\00x\00",
        (const unsigned char *) "\x10\x01\x08\x1e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x09\x2e\x08\x00\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x09\x2e\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x09\x2e\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x0a\x1e\x14\x00\x00",
        (const unsigned char *) "\x10\x01\x06\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\x01\x02\x0e\x00\x00\x00",
        (const unsigned char *) "\x10\xff\x81\x00\x00\x00\x00",

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

void lightspot_init_reset(hid_device *dev) {
    int res;
    unsigned char buf[65];

    hid_write(dev, init_cmds[0], 7);
    res = hid_read(dev, buf, 65);
    printhex(buf, res);
}

void lightspot_init_start(hid_device *dev) {
    int res;
    unsigned char buf[65];

    fprintf(stderr, "send start_cmd:");
    hid_write(dev, init_cmds[1], 7);
    res = hid_read(dev, buf, 65);
    printhex(buf, res);
}

void lightspot_init_init(hid_device *dev) {
    int res;
    unsigned char buf[65];

    for (int i=2; i<4; i++) {
        fprintf(stderr, "send init_cmd:");
        hid_write(dev, init_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }
    res = hid_read(dev, buf, 65);
    printhex(buf, res);
}

void lightspot_init_config(hid_device *dev) {
    int res;
    unsigned char buf[65];

    for(int i=4; i<8; i++) {
        fprintf(stderr, "send config_cmd:");
        hid_write(dev, init_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }
    for(int i=8; i<12; i++) {
        fprintf(stderr, "send config2_cmd:");
        hid_write(dev, init_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }
    res = hid_read(dev, buf, 65);
    printhex(buf, res);
    for(int i=12; i<13; i++) {
        fprintf(stderr, "send config3_cmd:");
        hid_write(dev, init_cmds[i], 7);
        res = hid_read(dev, buf, 65);
        printhex(buf, res);
    }

}

void lightspot_init(hid_device *dev) {
    lightspot_init_reset(dev);
    lightspot_init_start(dev);
    lightspot_init_init(dev);
    lightspot_init_config(dev);
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

