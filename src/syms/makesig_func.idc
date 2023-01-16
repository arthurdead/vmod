#include <idc.idc>

static get_dtype_size(dtype)
{
	if(dtype == dt_byte) {
		return 1;
	} else if(dtype == dt_word) {
		return 2;
	} else if(dtype == dt_dword || dtype == dt_float) {
		return 4;
	} else if(dtype == dt_double) {
		return 8;
	} else {
		msg("unknown type %i", dtype);
		return -1;
	}
}

static add_wildcards(sig, num)
{
	auto i = 0;
	for(i = 0; i < num; ++i) {
		sig = sig + "? ";
	}
}

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

	auto func_start = get_func_attr(cursor_addr, FUNCATTR_START);
	if(func_start == -1) {
		msg("failed to get func start address\n");
		set_ida_state(old_state);
		return;
	}

	auto func_end = get_func_attr(cursor_addr, FUNCATTR_END);
	if(func_end == -1) {
		msg("failed to get func end address\n");
		set_ida_state(old_state);
		return;
	}

	auto sig = "";

	auto sig_valid = 0;

	auto addr_it = func_start;
	while(addr_it != BADADDR) {
		auto code = decode_insn(addr_it);
		if(code == 0) {
			msg("failed to decode instruction\n");
			set_ida_state(old_state);
			return;
		}

		if(code.n == 1 && (code.Op0.type == o_near || code.Op0.type == o_far)) {
			auto byte = get_wide_byte(addr_it);
			if(byte == 0x0F) {
				sig = sig + sprintf("0F %02X ", get_wide_byte(addr_it+1));
			} else {
				sig = sig + sprintf("%02X ", byte);
			}
			auto dsize = get_dtype_size(code.Op0.dtype);
			if(dsize == -1) {
				msg("invalid dtype size\n");
				set_ida_state(old_state);
				return;
			}
			add_wildcards(&sig, dsize);
			break;
		} else {
			auto addr_size = get_item_size(addr_it);
			auto i = 0;
			for(i = 0; i < addr_size; ++i) {
				auto loc = (addr_it + i);
				auto fixup_type = get_fixup_target_type(loc);
				if((fixup_type & FIXUP_MASK) == FIXUP_OFF32) {
					add_wildcards(&sig, 4);
					i = i + 3;
				} else {
					sig = sig + sprintf("%02X ", get_wide_byte(loc));
				}
			}
		}

		sig_valid = is_valid_sig(sig);
		if(sig_valid == 1) {
			break;
		}

		//addr_it = next_head(addr_it, func_end);
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