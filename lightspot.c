#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "hidapi.h"

#define MAX_STR 255

hid_device *spotlight_device_connect() {
    int res;
    unsigned char buf[65];
    wchar_t wstr[MAX_STR];
    hid_device *handle;
    int i;

    // Initialize the hidapi library
    res = hid_init();

    // Enumerate and print the HID devices on the system
    struct hid_device_info *devs, *cur_dev, *target_dev;

    devs = hid_enumerate(0x046d, 0xc53e);
    cur_dev = devs;
    target_dev = NULL;
    while (cur_dev) {
        // target is interface 2 of Logitech USB receiver
        if (cur_dev->interface_number == 2) { target_dev = cur_dev; }
        cur_dev = cur_dev->next;
    }
    if (target_dev == NULL) { exit(1); }

    // Open the device using the VID, PID,
    // and optionally the Serial number.
    handle = hid_open_path(target_dev->path);
    if (handle == NULL) { exit(1); }

    res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    fprintf(stderr, "Connected to %ls", wstr);
    res = hid_get_product_string(handle, wstr, MAX_STR);
    fprintf(stderr, " %ls\n", wstr);
    hid_free_enumeration(devs);

    return handle;
}

int handle_ev(hid_device *handle) {
    int res;
    unsigned char buf[65];

    while (1) {
        res = hid_read_timeout(handle, buf, MAX_STR, 1000);
        if (res < 0) {
            // error
            return (0);
        } else if (res > 0) {
            // read data
            // TODO: implement me.
            // send event to effect controller
            fprintf(stderr, " Got pointer event\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int res;
    hid_device *handle;

    handle = spotlight_device_connect();
    if (handle == NULL) {
        return 0;
    }
    res = handle_ev(handle);
    if (!res) {
        return (1);
    }
    hid_close(handle);
    res = hid_exit();
    if (!res) {
        return (1);
    }

    return 0;
}
