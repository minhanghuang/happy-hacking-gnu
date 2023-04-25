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
	GET_KEYMAP = 135,
	DUMP_FIRMWARE = 208,
	FIRMUP_MODE_CHANGE = 224,
	FIRMUP_START = 225,
	FIRMUP_SEND = 226,
	FIRMUP_END = 227,
	// Warning: These are scary as they can modify bank 1, which could
	// completely brick your controller, use at your own risk.
	UPDATEBOOT_MODE_CHANGE = 228,
	UPDATEBOOT_START = 229,
	UPDATEBOOT_SEND = 230,
	UPDATEBOOT_END = 231
};

// Struct for GET_KEYBOARD_INFO
typedef struct {
	// Model number
	char type_number[20];
	// Model revision
	char revision[4];
	// Serial number
	char serial[16];
	// This is the 'primary' or 'A' version of the firmware, running on bank 2
	char appfirmversion[8];
	// This is the 'backup' or 'B' version of the firmware, running on bank 1
	// which will boot instead of AppFirm in case the primary firmware is corrupt
	char bootfirmversion[8];
	// This value is zero if running on AppFirm, and one if the board is using BootFirm
	char runningfirmware;
} hhkb_info;

// Struct for firmware data
typedef struct {
	char crc16[2];
	unsigned char *raw_data;
	int file_size;
} hhkb_firmware;

static void hhkb_notify_application_state(hid_device *handle, unsigned char open)
{
	// Initialize buffer for communications
	unsigned char buffer[USB_BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

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

	// Write to HID device and save response to buffer
	hhkb_write_buf(handle, buffer);
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: NOTIFY_APPLICATION_STATE ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}
}

static unsigned char *hhkb_get_dip_switch_state(hid_device *handle)
{
	unsigned char buffer[USB_BUFFER_SIZE];
	unsigned char *ret;

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_DIP_STATE);
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: GET_DIP_STATE ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	// Copy result from buffer, so ReadCheck data isn't returned
	ret = malloc(6);
	memcpy(ret, buffer + 6, 6);
	return ret;
}

static void hhkb_print_dip_switch_state(hid_device *handle)
{
	unsigned char *buffer;
	int i;

	// Get DIP switch state
	buffer = hhkb_get_dip_switch_state(handle);

	// Loop through results
	for (i = 0; i < 6; i++) {
		printf("Dip switch %i state: %s\n", i, buffer[i] ? "On" : "Off");
	}

	// Free buffer
	free(buffer);
}

static unsigned char hhkb_get_keyboard_mode(hid_device *handle)
{
	unsigned char buffer[USB_BUFFER_SIZE];

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_KEYBOARD_MODE);
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYBOARD_MODE ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	return buffer[6];
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

static hhkb_info *hhkb_get_info(hid_device *handle)
{
	unsigned char buffer[USB_BUFFER_SIZE];
	hhkb_info *result;

	// Write to HID device and save response to buffer
	hhkb_write(handle, GET_KEYBOARD_INFO);
	hhkb_read(handle, buffer);

	if (verbose_log) {
		// Debug log
		printf("debug: GET_KEYBOARD_INFO ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	result = malloc(sizeof(hhkb_info));
	memcpy(result, buffer + 6, sizeof(hhkb_info));

	return result;
}

static void hhkb_print_info(hid_device *handle)
{
	hhkb_info *info;

	// Get keyboard info
	info = hhkb_get_info(handle);

	// Print info
	printf("TypeNumber: %s\n", info->type_number);
	printf("Revision: %s\n", info->revision);
	printf("Serial: %s\n", info->serial);
	printf("AppFirmVersion: %X%d.%d%d\n", info->appfirmversion[0], info->appfirmversion[1],
		info->appfirmversion[2], info->appfirmversion[3]);
	printf("BootFirmVersion: %X%d.%d%d\n", info->bootfirmversion[0], info->bootfirmversion[1],
		info->bootfirmversion[2], info->bootfirmversion[3]);
	printf("RunningFirmware: %d\n", info->runningfirmware);

	// Free info buffer
	free(info);
}

static int hhkb_is_jis(hid_device *handle)
{
	hhkb_info *info;
	int ret;

	// Get keyboard information
	info = hhkb_get_info(handle);

	// All japanese models are PD-KBx20xx
	ret = !!strstr(info->type_number, "20");

	// Free info buffer
	free(info);
	return ret;
}

static int hhkb_is_hybrid(hid_device *handle)
{
	hhkb_info *info;
	int ret;

	// Get keyboard information
	info = hhkb_get_info(handle);

	// Hybrid models are PD-KB800xx, PD-KB820xx depending on exact model
	ret = !!strstr(info->type_number, "KB8");

	// Free info buffer
	free(info);
	return ret;
}

static unsigned char *hhkb_get_layout(hid_device *handle, unsigned char with_fn)
{
	unsigned char buffer[USB_BUFFER_SIZE];
	unsigned char *layout;
	int i;

	// Initialize buffer for communications
	memset(buffer, 0, sizeof(buffer));

	// Allocate layout array
	layout = (unsigned char *)calloc(128, 1);

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

	// First read
	hhkb_read(handle, buffer);
	for (i = 0; i < 58; i++)
		layout[i] = buffer[6 + i];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYMAP(1) ");
		for (i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Second read
	hhkb_read(handle, buffer);
	for (i = 0; i < 58; i++)
		layout[i + 58] = buffer[6 + i];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYMAP(2) ");
		for (i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Third read
	hhkb_read(handle, buffer);
	for (i = 0; i < 12; i++)
		layout[i + 116] = buffer[6 + i];

	// Debug log
	if (verbose_log) {
		printf("debug: GET_KEYMAP(3) ");
		for (i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Return complete array
	return layout;
}

static unsigned char hhkb_get_scancode(hid_device *handle, unsigned char remap_key)
{
	// Fetch the first layer scancode assigned to remap_key
	unsigned char *layout = hhkb_get_layout(handle, 0);
	return layout[remap_key];
}

static void hhkb_reset_to_factory_default(hid_device *handle)
{
	unsigned char buffer[USB_BUFFER_SIZE];

	// Write to HID device and save response to buffer
	hhkb_write(handle, RESET_FACTORY_DEFAULTS);
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: RESET_FACTORY_DEFAULTS ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}

	// Verify if device responded with the correct sequence of bytes
	if (buffer[0] == 85 && buffer[1] == 85 && buffer[2] == 3 && buffer[3] == 0) {
		printf("Success\n");
	} else {
		printf("error: did not get expected response for RESET_FACTORY_DEFAULTS\nerror: ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}
}

static void hhkb_reset_dipsw(hid_device *handle)
{
	// Initialize buffer for communications
	unsigned char buffer[USB_BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

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

	// Read response
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: RESET_DIPSW ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}
}

static void hhkb_write_keymap(hid_device *handle, unsigned char *layout, char fn)
{
	int i;

	// Initialize buffer for communications
	unsigned char buffer[USB_BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

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

	// Read response
	hhkb_read(handle, buffer);
	if (verbose_log) {
		printf("debug: WRITE_KEYMAP(1) ");
		for (i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	// Second pass
	memset(buffer, 0, sizeof(buffer));
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

	// Read response
	hhkb_read(handle, buffer);
	if (verbose_log) {
		printf("debug: WRITE_KEYMAP(2) ");
		for (i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}

	// Third pass
	memset(buffer, 0, sizeof(buffer));
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

	// Read response
	hhkb_read(handle, buffer);
	if (verbose_log) {
		printf("debug: WRITE_KEYMAP(3) ");
		for (i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}
}

static void hhkb_confirm_keymap(hid_device *handle)
{
	unsigned char buffer[USB_BUFFER_SIZE];

	// Confirm keymap
	hhkb_write(handle, CONFIRM_KEYMAP);
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: CONFIRM_KEYMAP ");
		for (int i = 0; i < 6; i++) {
			printf("0x%02X ", buffer[i]);
		}
		printf("\n");
	}
}

static void hhkb_remap_key(hid_device *handle, unsigned char remap_key, unsigned char remap_code, char fn)
{
	unsigned char *layout;

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

static void hhkb_print_layout_jis(hid_device *handle, int fn_layer)
{
	unsigned char *layout;
	int i;

	// Get layout array
	layout = hhkb_get_layout(handle, fn_layer);

	// Print first row
	printf("----------------------------------------------------------------------------\n|");
	for (i = 69; i > 54; i--) {
		printf(" %02d |", i);
	}
	printf("\n|");
	for (i = 69; i > 54; i--) {
		printf(" %02x |", layout[i]);
	}
	printf("\n----------------------------------------------------------------------------\n|");

	// Print second row
	for (i = 54; i > 40; i--) {
		if (i == 54) {
			printf("  %02d  |", i);
		} else if (i == 41) {
			printf("  %02d   |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n|");

	for (i = 54; i > 40; i--) {
		if (i == 54) {
			printf("  %02x  |", layout[i]);
		} else if (i == 41) {
			printf("  %02x   |", layout[i]);
		} else {

			printf(" %02x |", layout[i]);
		}
	}

	printf("\n---------------------------------------------------------------------      |\n|");

	// Print third row
	for (i = 40; i > 27; i--) {
		if (i == 40) {
			printf("  %02d   |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("      |");

	printf("\n|");

	for (i = 40; i > 27; i--) {
		if (i == 40) {
			printf("  %02x   |", layout[i]);
		} else {

			printf(" %02x |", layout[i]);
		}
	}
	printf("      |");

	printf("\n----------------------------------------------------------------------------\n|");

	// Print fourth row
	for (i = 27; i > 13; i--) {
		if (i == 27) {
			printf("   %02d    |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n|");

	for (i = 27; i > 13; i--) {
		if (i == 27) {
			printf("   %02x    |", layout[i]);
		} else {
			printf(" %02x |", layout[i]);
		}
	}

	printf("\n----------------------------------------------------------------------------\n|");

	// Print bottom row
	for (i = 13; i > 0; i--) {
		if (i == 13) {
			printf(" %02d | |", i);
		} else if (i == 8) {
			printf("    %02d    |", i);
		} else if (i == 3) {
			printf(" | %02d |", i);
		} else {
			printf(" %02d |", i);
		}
	}
	printf("\n|");

	for (i = 13; i > 0; i--) {
		if (i == 13) {
			printf(" %02x | |", layout[i]);
		} else if (i == 8) {
			printf("    %02x    |", layout[i]);
		} else if (i == 3) {
			printf(" | %02x |", layout[i]);
		} else {
			printf(" %02x |", layout[i]);
		}
	}
	printf("\n------ ---------------------------------------------------- ----------------");

	// Free layout array
	free(layout);

	printf("\n\n");
}

static void hhkb_dump_firmware(hid_device *handle)
{
	hhkb_info *info;
	unsigned char buffer[USB_BUFFER_SIZE];
	unsigned char *data;
	char filename[64];
	int size;
	int read;

	// Get serial number for filename
	info = hhkb_get_info(handle);

	// Format filename for firmware
	sprintf(filename, "%s.BIN", info->serial);

	// Initialize buffer for communications
	memset(buffer, 0, sizeof(buffer));

	// Allocate firmware data array
	data = (unsigned char *)calloc(300000, 1);

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Command ID
	buffer[3] = DUMP_FIRMWARE;

	// Write to HID device and clear buffer
	hhkb_write_buf(handle, buffer);
	memset(buffer, 0, sizeof(buffer));

	// Total read bytes
	size = 0;

	// Read loop
	do {
		// Save response to buffer
		hhkb_read(handle, buffer);

		// Debug log
		if (verbose_log) {
			printf("debug: DUMP_FIRMWARE ");
			for (int i = 0; i < 8; i++) {
				printf("0x%02X ", buffer[i]);
			}
			printf("\n");
		}

		// Get number of bytes to read
		read = buffer[5] - 2;

		// Copy firmware data to buffer
		memcpy(data + size, buffer + 8, read);
		size += read;
	} while (read >= 56);

	// Save firmware data to file
	FILE *f = fopen(filename, "wb");
	if (!f || !fwrite(data, size, 1, f)) {
		printf("error: failed to write data to disk\n");
		hhkb_quit(handle);
	}
	fclose(f);

	// Clean up buffers
	free(info);
	free(data);

	printf("Success (wrote %i bytes to %s)\n", size, filename);
}

static void hhkb_firmup_mode_change(hid_device *handle)
{
	unsigned char buffer[USB_BUFFER_SIZE];

	// Write to HID device
	hhkb_write(handle, FIRMUP_MODE_CHANGE);

	// Read response from device
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: FIRMUP_MODE_CHANGE ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}
}

static void hhkb_firmup_end(hid_device *handle)
{
	// Initialize buffer for communications
	unsigned char buffer[USB_BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	// Write to HID device
	hhkb_write(handle, FIRMUP_END);

	// Read response from device
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: FIRMUP_END ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}
}

static void hhkb_firmup_start(hid_device *handle, hhkb_firmware *data)
{
	// Initialize buffer for communications
	unsigned char buffer[USB_BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	// Added by USBDriver::Send
	buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	buffer[1] = 170;
	buffer[2] = 170;

	// Command ID
	buffer[3] = FIRMUP_START;

	// Unknown
	buffer[4] = 0;
	buffer[5] = 8;

	// Firmware size
	memcpy(&buffer[6], &data->file_size, 4);

	// CRC
	memcpy(&buffer[10], &data->crc16, 2);

	// Write to HID device
	hhkb_write_buf(handle, buffer);

	// Read response from device
	hhkb_read(handle, buffer);

	// Debug log
	if (verbose_log) {
		printf("debug: FIRMUP_START ");
		for (int i = 0; i < 6; i++)
			printf("0x%02X ", buffer[i]);

		printf("\n");
	}
}

static void hhkb_firmup_send(hid_device *handle, hhkb_firmware *fw)
{
	unsigned char write_buffer[USB_BUFFER_SIZE];
	unsigned char read_buffer[USB_BUFFER_SIZE];
	unsigned char *data;
	unsigned short packet_num;
	int firmware_size;
	int packet_count;
	int total_sent;
	int write_len;

	// Initialize arrays
	memset(write_buffer, 0, sizeof(write_buffer));
	memset(read_buffer, 0, sizeof(read_buffer));

	// Skip the first two bytes, as they are only verified by
	// the keymap tool, and not part of the actual firmware
	firmware_size = fw->file_size - 2;
	data = fw->raw_data + 2;

	// Calculate how many total packets are needed
	packet_count = (firmware_size + 57 - 1) / 57;
	total_sent = 0;

	// Added by USBDriver::Send
	write_buffer[0] = 0;

	// 170 is used in buffer[1] and buffer[2] for all requests
	write_buffer[1] = 170;
	write_buffer[2] = 170;

	// Command ID
	write_buffer[3] = FIRMUP_SEND;

	for (packet_num = 0; packet_num < packet_count; packet_num++) {
		// Number of bytes that fit in a single packet
		write_len = 57;

		// Only send the last bit if we're at the end
		if (firmware_size - total_sent < 57)
			write_len = firmware_size - total_sent;

		// How many bytes are in the packet
		write_buffer[5] = write_len + 2;

		// Expected packet number
		memcpy(&write_buffer[6], &packet_num, 2);

		// Copy binary data
		memcpy(&write_buffer[8], data + total_sent, write_len);

		// Write to HID device and clear data in buffer
		hhkb_write_buf(handle, write_buffer);
		memset(write_buffer + 8, 0, 57);

		// Read from HID device
		hhkb_read(handle, read_buffer);

		// Debug log
		if (verbose_log) {
			printf("debug: FIRMUP_SEND(%i) ", packet_num);
			for (int i = 0; i < 6; i++)
				printf("0x%02X ", read_buffer[i]);
			printf("\n");
		}

		// Keep track of data sent so far
		total_sent += write_len;

		// Check for write errors
		if (*(unsigned short *)&read_buffer[6] != packet_num) {
			printf("error: unable to send firmware during update\n");
			hhkb_quit(handle);
		}
	}
}

static hhkb_firmware *hhkb_load_firmware(hid_device *handle, const char *path)
{
	hhkb_firmware *fw;

	// Load firmware data from file
	FILE *f = fopen(path, "r");
	if (!f) {
		printf("error: unable to open firmware file\n");
		hhkb_quit(handle);
	}

	// Allocate struct for firmware data
	fw = calloc(sizeof(hhkb_firmware), 1);

	// Get file size
	fseek(f, 0L, SEEK_END);
	fw->file_size = ftell(f);
	rewind(f);

	// Allocate buffer for firmware file
	fw->raw_data = (unsigned char *)calloc(fw->file_size, 1);

	// Read firmware into buffer
	if (!fw->file_size || !fread(fw->raw_data, fw->file_size, 1, f)) {
		printf("error: unable to read firmware file\n");
		hhkb_quit(handle);
	}

	// Copy CRC from firmware file
	memcpy(&fw->crc16, fw->raw_data, 2);

	fclose(f);
	return fw;
}

static void hhkb_firmup(hid_device *handle, const char *path)
{
	hhkb_firmware *data;

	// Load firmware from file
	data = hhkb_load_firmware(handle, path);

	// Notify the device that the Keymap Tool is running
	hhkb_notify_application_state(handle, 0);

	// Enter firmware update mode
	hhkb_firmup_mode_change(handle);

	// Reopen handle to device, as entering firmware update mode
	// closes the current handle and reconnects the HID device
	hid_close(handle);
	sleep(4000);
	handle = hhkb_init();

	printf("Installing firmware...\n");

	// Start firmware update
	hhkb_firmup_start(handle, data);

	// Send firmware data
	hhkb_firmup_send(handle, data);

	// End firmware update
	hhkb_firmup_end(handle);
	printf("Success\n");

	// Notify the device that the Keymap Tool is closed
	hhkb_notify_application_state(handle, 1);

	// Clean up buffers
	free(data->raw_data);
	free(data);

	hid_close(handle);
	exit(EXIT_SUCCESS);
}