#include <idc.idc>

static is_valid_sig(sig)
{
	auto count = 0;

	auto search_it = 0;
	while(1) {
		search_it = find_binary(search_it, SEARCH_DOWN|SEARCH_NEXT, sig);
		if(search_it == BADADDR) {
			break;
		}

		count = count + 1;
	}

	if(count == 0) {
		return 0;
	} else if(count == 1) {
		return 1;
	} else {
		return 2;
	}
}

static main()
{
	auto_wait();

	auto old_state = set_ida_state(IDA_STATUS_WORK);

	auto cursor_addr = get_screen_ea();
	if(cursor_addr == BADADDR) {
		msg("failed to get cursor address\n");
		set_ida_state(old_state);
		return;
	}

	auto sig = "";

	auto sig_valid = 0;

	auto addr_it = cursor_addr;
	while(addr_it != BADADDR) {
		sig = sig + sprintf("%02X ", get_wide_byte(addr_it));

		sig_valid = is_valid_sig(sig);
		if(sig_valid == 1) {
			break;
		}

		addr_it = next_addr(addr_it);
	}

	if(sig_valid != 1) {
		msg("failed to create signature\n");
		set_ida_state(old_state);
		return;
	}

	auto siglen = strlen(sig);

	sig = substr(sig, 0, siglen-1);

	msg("%s\n", sig);

	auto c = 0;

	auto yamlsig = "[0x";
	auto i = 0;
	for(i = 0; i < siglen; ++i) {
		c = substr(sig, i, i+1);
		if(c == " ") {
			yamlsig = yamlsig + ", 0x";
			continue;
		} else if(c == "?") {
			yamlsig = yamlsig + "null";
		} else {
			yamlsig = yamlsig + c;
		}
	}
	yamlsig = yamlsig + "]";
	msg("%s\n", yamlsig);

	set_ida_state(old_state);
}