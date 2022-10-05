#pragma once
#include <hidapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB_BUFFER_SIZE 65

static hid_device *hhkb_get_programming_interface()
{
	struct hid_device_info *devices;
	hid_device *ret;
	// Enumerate hid devices in order to find the programming interface
	devices = hid_enumerate(0x04fe, 0x0);
	ret = 0;

	for (struct hid_device_info *device = devices; device; device = device->next) {
		// Ignore devices if the product ID is out of the HHKB range
		// Select current path if third interface (used by Keymap Tool)
		if (device->product_id >= 0x0020 && device->product_id <= 0x22
				&& device->interface_number == 2) {
			ret = hid_open_path(device->path);
			break;
		}
	}

	// Quit if interface is not found
	if (!ret) {
		printf("error: no keyboard connected\n");
		exit(EXIT_FAILURE);
	}

	hid_free_enumeration(devices);
	return ret;
}

static hid_device *hhkb_init()
{
	hid_device *handle;
	int res;

	// Initialize hidapi library
	if (hid_init() < 0) {
		printf("error: failed to run hid_init() (%ls)\n", hid_error(NULL));
		exit(-1);
	}

	// Open handle to the remapping HID device
	handle = hhkb_get_programming_interface();

	if (!handle) {
		printf("error: unable to open handle (%ls)\n", hid_error(NULL));
		exit(-1);
	}

	return handle;
}

static void hhkb_quit(hid_device *handle)
{
	// Something failed, cleanup and exit
	hid_close(handle);
	exit(EXIT_FAILURE);
}

static void hhkb_print_product_info(hid_device *handle)
{
	wchar_t product[255];
	wchar_t manufacturer[255];

	// Get product name
	if (hid_get_product_string(handle, product, 255) < 0) {
		printf("error: unable to read product string (%ls)\n", hid_error(handle));
		hhkb_quit(handle);
	}

	// Get manufacturer name
	if (hid_get_manufacturer_string(handle, manufacturer, 255) < 0)
		printf("Unable to read manufacturer string\n");

	// Print debug message
	printf("debug: %ls %ls\n", manufacturer, product);
}

static void hhkb_write(hid_device *handle, int idx)
{
	// The USB buffer is defined as 64 bytes, however when writing to the device
	// an additional zero value is added at buffer[0], and OutputReportByteLength
	// is used (65 bytes)
	unsigned char buffer[USB_BUFFER_SIZE];

	// Clean buffer
	memset(buffer, 0x0, sizeof(buffer));

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Index used for function
	buffer[3] = idx;

	if (hid_write(handle, buffer, USB_BUFFER_SIZE) < 0) {
		printf("error: unable to write to HID device (%ls)\n", hid_error(handle));
		hhkb_quit(handle);
	}
}

static void hhkb_write_buf(hid_device *handle, unsigned char *buffer)
{
	// Write passed buffer to device
	if (hid_write(handle, buffer, USB_BUFFER_SIZE) < 0) {
		printf("error: unable to write to HID device (%ls)\n", hid_error(handle));
		hhkb_quit(handle);
	}
}

static unsigned char *hhkb_read(hid_device *handle)
{
	unsigned char *buffer;

	// Allocate read buffer
	buffer = (unsigned char *)malloc(65);

	// Read from device
	if (hid_read(handle, buffer, 65) < 0) {
		printf("error: unable to read from HID device (%ls)\n", hid_error(handle));
		hhkb_quit(handle);
	}

	return buffer;
}