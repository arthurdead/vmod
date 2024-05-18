local map_to_root = null;

local cnst = ::getconsttable();
local root = ::getroottable();

map_to_root = function(tbl, name)
{
	local valid = true;
	local has_value = false;

	foreach(i,j in tbl) {
		if(i == "__vrefs" || i == "__vname" || i == "__internal_ptr__") {
			continue;
		}

		local currname = name+"_"+i;

		root[currname] <- j;

		local typ = typeof(j);
		if(typ == "string" || typ == "integer" || typ == "float") {
			has_value = true;
			cnst[currname] <- j;
		} else {
			valid = false;

			if(typ == "table") {
				map_to_root(j, currname);
			}
		}
	}

	root[name] <- tbl;

	if(has_value && valid) {
		cnst[name] <- tbl;
	}
}

map_to_root(::vmod, "vmod");
