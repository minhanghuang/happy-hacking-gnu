#pragma once
#include "hidcomm.h"

// Debug logging flag
extern int verbose_log;

// Command IDs (set in buffer[3])
enum {
	NOTIFY_APPLICATION_STATE = 1,
	GET_KEYBOARD_INFO = 2,
	RESET_FACTORY_DEFAULTS = 3,
	CONFIRM_KEYMAP = 4,
	GET_DIP_STATE = 5,
	GET_KEYBOARD_MODE = 6,
	RESET_DIPSW = 7,
	WRITE_KEYMAP = 134,
	GET_KEYMAP = 135
};

static void hhkb_notify_application_state(hid_device *handle, unsigned char open)
{
	// Allocate buffer for communications
	unsigned char *buffer;
	buffer = (unsigned char *)malloc(USB_BUFFER_SIZE);
	memset(buffer, 0x0, USB_BUFFER_SIZE);

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Command ID
	buffer[3] = NOTIFY_APPLICATION_STATE;

	// Unknown
	buffer[4] = 0;
	buffer[5] = 1;

	// Application state (0 = open, 1 = closed)
	buffer[6] = open;

	// Write to HID device and discard buffer
	hhkb_write_buf(handle, buffer);
	free(buffer);

	// Read response from device
	buffer = hhkb_read(handle);

	// Debug log
	if (verbose_log) {
		printf("debug: NotifyApplicationState ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}
}

static void hhkb_print_dip_switch_state(hid_device *handle)
{
	unsigned char *buffer;
	int i;

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_DIP_STATE);
	buffer = hhkb_read(handle);

	// Debug log
	if (verbose_log) {
		printf("debug: GET_DIP_STATE ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	// Loop through results
	for (i = 1; i <= 6; i++) {
		printf("Dip switch %i state: %s\n", i, buffer[i + 5] ? "On" : "Off");
	}

	free(buffer);
}

static unsigned char hhkb_get_keyboard_mode(hid_device *handle)
{
	unsigned char *buffer;
	unsigned char ret;

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_KEYBOARD_MODE);
	buffer = hhkb_read(handle);

	ret = buffer[6];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYBOARD_MODE ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	// Free read buffer
	free(buffer);

	return ret;
}

static void hhkb_print_keyboard_mode(hid_device *handle)
{
	unsigned char mode;

	// Get keyboard mode
	mode = hhkb_get_keyboard_mode(handle);

	// Print result
	switch (mode) {
	case 0:
		printf("HHK Mode\n");
		break;
	case 1:
		printf("Mac Mode\n");
		break;
	case 2:
		printf("Lite Mode\n");
		break;
	case 3:
		printf("Secret Mode\n");
		break;
	}
}

static void hhkb_print_info(hid_device *handle)
{
	unsigned char *buffer;

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_KEYBOARD_INFO);
	buffer = hhkb_read(handle);

	if (verbose_log) {
		// Debug log
		printf("debug: GET_KEYBOARD_INFO ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Print type_number
	char type_number[64];
	memcpy(type_number, buffer + 6, 20);
	printf("TypeNumber: %s\n", type_number);

	// Print revision
	char revision[64];
	memcpy(revision, buffer + 26, 4);
	printf("Revision: %s\n", revision);

	// Print serial
	char serial[64];
	memcpy(serial, buffer + 30, 16);
	printf("Serial: %s\n", serial);

	// This is the 'primary' or 'A' version of the firmware, running on bank 2
	char appfirmversion[64];
	memcpy(appfirmversion, buffer + 46, 8);
	printf("AppFirmVersion: %X%d.%d%d\n", appfirmversion[0], appfirmversion[1],
		appfirmversion[2], appfirmversion[3]);

	// This is the 'backup' or 'B' version of the firmware, running on bank 1
	// which will boot instead of AppFirm in case the primary firmware is corrupt
	char bootfirmversion[64];
	memcpy(bootfirmversion, buffer + 54, 8);
	printf("BootFirmVersion: %X%d.%d%d\n", bootfirmversion[0], bootfirmversion[1],
		bootfirmversion[2], bootfirmversion[3]);

	// This value is zero if running on AppFirm, and one if
	// the board is using BootFirm
	char runningfirmware;
	memcpy(&runningfirmware, buffer + 62, 1);
	printf("RunningFirmware: %d\n", runningfirmware);

	// Free read buffer
	free(buffer);
}

static int hhkb_is_jis(hid_device *handle)
{
	unsigned char *buffer;
	char model[64];

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_KEYBOARD_INFO);
	buffer = hhkb_read(handle);

	if (verbose_log) {
		// Debug log
		printf("debug: GET_KEYBOARD_INFO ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Get model string from buffer
	memcpy(model, buffer + 6, 20);

	// Free read buffer
	free(buffer);

	// All japanese models are PD-KBx20xx
	return !!strstr(model, "20");
}

static int hhkb_is_hybrid(hid_device *handle)
{
	unsigned char *buffer;
	char model[64];

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_KEYBOARD_INFO);
	buffer = hhkb_read(handle);

	if (verbose_log) {
		// Debug log
		printf("debug: GET_KEYBOARD_INFO ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Get model string from buffer
	memcpy(model, buffer + 6, 20);

	// Free read buffer
	free(buffer);

	// Hybrid models are PD-KB800xx, PD-KB820xx depending on exact model
	return !!strstr(model, "KB8");
}

static unsigned char *hhkb_get_layout(hid_device *handle, unsigned char with_fn)
{
	unsigned char *buffer;
	unsigned char *layout;
	int res;
	int i;

	// Allocate buffer for communication
	buffer = (unsigned char *)malloc(USB_BUFFER_SIZE);
	memset(buffer, 0x0, USB_BUFFER_SIZE);

	// Allocate layout array
	layout = (unsigned char *)malloc(128);
	memset(layout, 0x0, 128);

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Request index
	buffer[3] = GET_KEYMAP;

	// Application state (maybe?)
	buffer[5] = 2;

	// Keyboard mode (mac/hhk/lite)
	buffer[6] = hhkb_get_keyboard_mode(handle);

	// Fn layer
	buffer[7] = with_fn;

	// Write to HID device
	hhkb_write_buf(handle, buffer);
	free(buffer);

	// First read
	buffer = hhkb_read(handle);
	for (i = 0; i < 58; i++)
		layout[i] = buffer[6 + i];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYMAP(1) ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Free read buffer
	free(buffer);

	// Second read
	buffer = hhkb_read(handle);
	for (i = 0; i < 58; i++)
		layout[i + 58] = buffer[6 + i];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYMAP(2) ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Free read buffer
	free(buffer);

	// Third read
	buffer = hhkb_read(handle);
	for (i = 0; i < 12; i++)
		layout[i + 116] = buffer[6 + i];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYMAP(3) ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Free read buffer
	free(buffer);

	// Return complete array
	return layout;
}

static void hhkb_reset_to_factory_default(hid_device *handle)
{
	unsigned char *buffer;

	// Write to HID device and save response to buffer
	hhkb_write(handle, RESET_FACTORY_DEFAULTS);
	buffer = hhkb_read(handle);

	// Debug log
	if (verbose_log) {
		printf("debug: RESET_FACTORY_DEFAULTS ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Verify if device responded with the correct
	// sequence of bytes
	if (buffer[0] == 85 && buffer[1] == 85 && buffer[2] == 3 && buffer[3] == 0) {
		printf("Success\n");
	} else {
		printf("error: did not get expected response for RESET_FACTORY_DEFAULTS\nerror: ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Free read buffer
	free(buffer);
}

static void hhkb_reset_dipsw(hid_device *handle)
{
	// Allocate buffer for communications
	unsigned char *buffer;
	buffer = (unsigned char *)malloc(USB_BUFFER_SIZE);
	memset(buffer, 0x0, USB_BUFFER_SIZE);

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Command ID
	buffer[3] = RESET_DIPSW;

	// Unknown
	buffer[4] = 0;
	buffer[5] = 1;

	hhkb_write_buf(handle, buffer);

	// Free write buffer
	free(buffer);

	// Read response
	buffer = hhkb_read(handle);

	if (verbose_log) {
		// Print result
		printf("debug: RESET_DIPSW ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Free read buffer
	free(buffer);
}

static void hhkb_write_keymap(hid_device *handle, unsigned char *layout, char fn)
{
	int i;

	// Allocate buffer for communications
	unsigned char *buffer;
	buffer = (unsigned char *)malloc(USB_BUFFER_SIZE);
	memset(buffer, 0x0, USB_BUFFER_SIZE);

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Command ID
	buffer[3] = WRITE_KEYMAP;

	// Unknown
	buffer[4] = 65;
	buffer[5] = 59;

	// Keyboard mode
	buffer[6] = hhkb_get_keyboard_mode(handle);

	// FN-layer
	buffer[7] = fn;

	// First pass
	for (i = 0; i < 57; i++)
		buffer[8 + i] = layout[i];

	// Write first pass
	hhkb_write_buf(handle, buffer);
	free(buffer);

	buffer = hhkb_read(handle);

	if (verbose_log) {
		printf("debug: WRITE_KEYMAP(1) ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	free(buffer);

	// Second pass
	buffer = (unsigned char *)malloc(USB_BUFFER_SIZE);
	memset(buffer, 0x0, USB_BUFFER_SIZE);

	buffer[0] = 0;
	buffer[1] = 170;
	buffer[2] = 170;
	buffer[3] = WRITE_KEYMAP;
	buffer[4] = 130;
	buffer[5] = 59;

	for (i = 0; i < 59; i++)
		buffer[6 + i] = layout[i + 57];

	// Write second pass
	hhkb_write_buf(handle, buffer);
	free(buffer);

	buffer = hhkb_read(handle);

	if (verbose_log) {
		printf("debug: WRITE_KEYMAP(2) ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	free(buffer);

	// Third pass
	buffer = (unsigned char *)malloc(USB_BUFFER_SIZE);
	memset(buffer, 0x0, USB_BUFFER_SIZE);

	buffer[0] = 0;
	buffer[1] = 170;
	buffer[2] = 170;
	buffer[3] = WRITE_KEYMAP;
	buffer[4] = 195;
	buffer[5] = 12;

	for (i = 0; i < 12; i++)
		buffer[6 + i] = layout[i + 116];

	// Write third pass
	hhkb_write_buf(handle, buffer);
	free(buffer);

	buffer = hhkb_read(handle);

	if (verbose_log) {
		printf("debug: WRITE_KEYMAP(3) ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	free(buffer);
}

static void hhkb_confirm_keymap(hid_device *handle)
{
	unsigned char *buffer;

	// Confirm keymap
	hhkb_write(handle, CONFIRM_KEYMAP);
	buffer = hhkb_read(handle);

	if (verbose_log) {
		printf("debug: CONFIRM_KEYMAP ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	free(buffer);
}

static void hhkb_remap_key(hid_device *handle, unsigned char remap_key, unsigned char remap_code, char fn)
{
	unsigned char *buffer;
	unsigned char *layout;
	int res;
	int i;

	// Notify the device that the Keymap Tool is running
	hhkb_notify_application_state(handle, 0);

	// Grab current layout
	layout = hhkb_get_layout(handle, fn);

	// Remap key
	layout[remap_key] = remap_code;

	// Write layout
	hhkb_write_keymap(handle, layout, fn);

	// Confirm keymap
	hhkb_confirm_keymap(handle);

	// Reset dipswitch state
	hhkb_reset_dipsw(handle);

	// Notify the device that the Keymap Tool is closed
	hhkb_notify_application_state(handle, 1);

	printf("Success\n");
}

static void hhkb_print_layout_ansi(hid_device *handle, int fn_layer)
{
	unsigned char *layout;
	int i;

	// Get layout array
	layout = hhkb_get_layout(handle, fn_layer);

	// Print first row
	printf("----------------------------------------------------------------------------\n|");
	for (i = 60; i > 45; i--) {
		printf(" %02d |", i);
	}
	printf("\n|");
	for (i = 60; i > 45; i--) {
		printf(" %02x |", layout[i]);
	}
	printf("\n----------------------------------------------------------------------------\n|");

	// Print second row
	for (i = 45; i > 31; i--) {
		if (i == 45) {
			printf("  %02d  |", i);
		} else if (i == 32) {
			printf("  %02d   |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n|");

	for (i = 45; i > 31; i--) {
		if (i == 45) {
			printf("  %02x  |", layout[i]);
		} else if (i == 32) {
			printf("  %02x   |", layout[i]);
		} else {

			printf(" %02x |", layout[i]);
		}
	}

	printf("\n----------------------------------------------------------------------------\n|");

	// Print third row
	for (i = 31; i > 18; i--) {
		if (i == 31) {
			printf("  %02d   |", i);
		} else if (i == 19) {
			printf("    %02d     |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n|");

	for (i = 31; i > 18; i--) {
		if (i == 31) {
			printf("  %02x   |", layout[i]);
		} else if (i == 19) {
			printf("    %02x     |", layout[i]);
		} else {

			printf(" %02x |", layout[i]);
		}
	}
	printf("\n----------------------------------------------------------------------------\n|");

	// Print fourth row
	for (i = 18; i > 5; i--) {
		if (i == 18) {
			printf("   %02d     |", i);
		} else if (i == 7) {
			printf("   %02d   |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n|");

	for (i = 18; i > 5; i--) {
		if (i == 18) {
			printf("   %02x     |", layout[i]);
		} else if (i == 7) {
			printf("   %02x   |", layout[i]);
		} else {

			printf(" %02x |", layout[i]);
		}
	}

	printf("\n----------------------------------------------------------------------------\n        |");

	// Print bottom row
	for (i = 5; i > 0; i--) {
		if (i == 4 || i == 2) {
			printf("  %02d   |", i);
		} else if (i == 3) {
			printf("               %02d               |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n        |");

	for (i = 5; i > 0; i--) {
		if (i == 4 || i == 2) {
			printf("  %02x   |", layout[i]);
		} else if (i == 3) {
			printf("               %02x               |", layout[i]);
		} else {

			printf(" %02x |", layout[i]);
		}
	}
	printf("\n        ------------------------------------------------------------");

	// Free layout array
	free(layout);

	printf("\n\n");
}
