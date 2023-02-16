#include "functions.h"
#include <argparse.h>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#define Sleep(x) usleep(x * 1000)
#endif

// Debug logging flag
int verbose_log = 0;

// Usage prompt for argparse
static const char *const usage[] = {
	"hhg [options] [[--] args]",
	NULL,
	NULL,
};

// Bits for arguments
enum {
	ACTION_INFO = (1 << 0),
	ACTION_DIP = (1 << 1),
	ACTION_MODE = (1 << 2),
	ACTION_KEYMAP = (1 << 3),
	ACTION_FACTORY_RESET = (1 << 4),
	ACTION_REMAP = (1 << 5),
	ACTION_DUMP_FW = (1 << 6)
};

int main(int argc, const char **argv)
{
	// Argument variables
	int action;
	int fn;
	int key;
	int code;
	char fw_file[255];

	// Clear argument variables
	action = fn = key = code = fw_file[0] = 0;

	// Argument parser options
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_BOOLEAN('v', "verbose", &verbose_log, "show debug messages"),
		OPT_GROUP("Basic options"),
		OPT_BIT('i', "info", &action, "print keyboard information", NULL, ACTION_INFO),
		OPT_BIT('d', "dip", &action, "print dipswitch state", NULL, ACTION_DIP),
		OPT_BIT('m', "mode", &action, "print keyboard mode", NULL, ACTION_MODE),
		OPT_BIT('k', "keymap", &action, "print current keymap", NULL, ACTION_KEYMAP),
		OPT_BIT('f', "factory-reset", &action, "reset to factory defaults", NULL, ACTION_FACTORY_RESET),
		OPT_GROUP("Keymapping options"),
		OPT_INTEGER('r', "remap-key", &key, "key number to remap", NULL, OPT_NONEG),
		OPT_INTEGER('s', "scancode", &code, "hid scancode to map", NULL, OPT_NONEG),
		OPT_BOOLEAN(0, "fn", &fn, "operate on function layer"),
		OPT_GROUP("Firmware options"),
		OPT_STRING(0, "flash-firmware", &fw_file, "flash firmware from file"),
		OPT_BIT(0, "dump-firmware", &action, "save current firmware to file", NULL, ACTION_DUMP_FW, 0),

		OPT_END(),
	};

	// Init argument parser
	struct argparse argparse;
	argparse_init(&argparse, options, usage, 0);
	argparse_describe(&argparse, 0, "");

	// Parse arguments
	argc = argparse_parse(&argparse, argc, argv);

	// We can't do firmware stuff yet
	if (strlen(fw_file) && action == 0) {
		printf("error: this command isn't implemented yet\n");
		return EXIT_FAILURE;
	}

	// Set remap flag if proper args are set
	if (key != 0 && key <= 69 && code != 0 && code <= 0xff) {
		action |= ACTION_REMAP;
	}

	// Show help message and quit if no args are set
	if (action == 0) {
		argparse_usage(&argparse);
		return EXIT_FAILURE;
	}

	// Connect to device
	hid_device *handle = hhkb_init();

	// Set remap flag if proper args are set
	if (key > 0 && key <= (hhkb_is_jis(handle) ? 69 : 60) && code > 0 && code <= 0xff) {
		action |= ACTION_REMAP;
	}

	// Debug log
	if (verbose_log)
		hhkb_print_product_info(handle);

	// Handle selected action
	if (action & ACTION_INFO) {
		hhkb_print_info(handle);
	}
	else if (action & ACTION_DIP) {
		hhkb_print_dip_switch_state(handle);
	}
	else if (action & ACTION_MODE) {
		hhkb_print_keyboard_mode(handle);
	}
	else if (action & ACTION_KEYMAP) {
		if (hhkb_is_jis(handle))
			hhkb_print_layout_jis(handle, fn);
		else
			hhkb_print_layout_ansi(handle, fn);
	}
	else if (action & ACTION_FACTORY_RESET) {
		// Confirm operation
		printf("Are you sure you want to restore factory defaults?\nPlease type 'reset' to continue: ");
		char str[10];
		fgets(str, 10, stdin);

		// Check input text
		if (!strcmp(str, "reset\n")) {
			Sleep(1000);
			hhkb_reset_to_factory_default(handle);
		} else {
			printf("Aborting..\n");
		}
	}
	else if (action & ACTION_REMAP) {
		// Hybrid models reserve FN+Q for pairing
		// FN+Z and FN+X are technically reserved as well, but can be remapped fine excluding media keys
		if (hhkb_is_hybrid(handle) && fn && (hhkb_is_jis(handle) ? key == 53 : key == 44)) {
			printf("error: FN+Q is reserved for bluetooth pairing on hybrid models\n");
			hhkb_quit(handle);
		}

		// Confirm operation
		printf("Are you sure you want to assign 0x%02X to %i?\nPlease type 'confirm' to continue: ", code, key);
		char str[10];
		fgets(str, 10, stdin);

		// Check input text
		if (!strcmp(str, "confirm\n")) {
			Sleep(1000);
			hhkb_remap_key(handle, key, code, fn);
		} else {
			printf("Aborting..\n");
		}
	}
	else if (action & ACTION_DUMP_FW) {
		// Confirm operation
		printf("This operation will take a couple of minutes, during which the keyboard will not be functional.\nPlease type 'confirm' to continue: ");
		char str[10];
		fgets(str, 10, stdin);

		// Check input text
		if (!strcmp(str, "confirm\n")) {
			hhkb_dump_firmware(handle);
		} else {
			printf("Aborting..\n");
		}
	}

	// Close handle and shutdown
	hid_close(handle);
	hid_exit();

	return EXIT_SUCCESS;
}