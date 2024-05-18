::Constants.ESpatialMask <- {
	MASK_ALL = 0xFFFFFFFF,
	MASK_SOLID = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_MONSTER|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_PLAYERSOLID = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_PLAYERCLIP|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_MONSTER|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_NPCSOLID = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_MONSTERCLIP|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_MONSTER|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_WATER = (
		::Constants.FContents.CONTENTS_WATER|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_SLIME
	),
	MASK_OPAQUE = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_OPAQUE
	),
	MASK_BLOCKLOS = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_BLOCKLOS
	),
	MASK_SHOT = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_MONSTER|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_DEBRIS|
		::Constants.FContents.CONTENTS_HITBOX
	),
	MASK_SHOT_HULL = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_MONSTER|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_DEBRIS|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_SHOT_PORTAL = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_MONSTER
	),
	MASK_SOLID_BRUSHONLY = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_PLAYERSOLID_BRUSHONLY = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_PLAYERCLIP|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_NPCSOLID_BRUSHONLY = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_MOVEABLE|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_MONSTERCLIP|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_NPCWORLDSTATIC = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_MONSTERCLIP|
		::Constants.FContents.CONTENTS_GRATE
	),
	MASK_SPLITAREAPORTAL = (
		::Constants.FContents.CONTENTS_WATER|
		::Constants.FContents.CONTENTS_SLIME
	),
	MASK_CURRENT = (
		::Constants.FContents.CONTENTS_CURRENT_0|
		::Constants.FContents.CONTENTS_CURRENT_90|
		::Constants.FContents.CONTENTS_CURRENT_180|
		::Constants.FContents.CONTENTS_CURRENT_270|
		::Constants.FContents.CONTENTS_CURRENT_UP|
		::Constants.FContents.CONTENTS_CURRENT_DOWN
	),
	MASK_DEADSOLID = (
		::Constants.FContents.CONTENTS_SOLID|
		::Constants.FContents.CONTENTS_PLAYERCLIP|
		::Constants.FContents.CONTENTS_WINDOW|
		::Constants.FContents.CONTENTS_GRATE
	)
};

::Constants.ESpatialMask.MASK_OPAQUE_AND_NPCS <- (
	::Constants.ESpatialMask.MASK_OPAQUE|
	::Constants.FContents.CONTENTS_MONSTER
);

::Constants.ESpatialMask.MASK_BLOCKLOS_AND_NPCS <- (
	::Constants.ESpatialMask.MASK_BLOCKLOS|
	::Constants.FContents.CONTENTS_MONSTER
);

::Constants.ESpatialMask.MASK_VISIBLE <- (
	::Constants.ESpatialMask.MASK_OPAQUE|
	::Constants.FContents.CONTENTS_IGNORE_NODRAW_OPAQUE
);

::Constants.ESpatialMask.MASK_VISIBLE_AND_NPCS <- (
	::Constants.ESpatialMask.MASK_OPAQUE_AND_NPCS|
	::Constants.FContents.CONTENTS_IGNORE_NODRAW_OPAQUE
);

::Constants.EBrushSolid <- {
	BRUSHSOLID_TOGGLE = 0,
	BRUSHSOLID_NEVER = 1,
	BRUSHSOLID_ALWAYS = 2
};

::Constants.Server.MIN_COORD_FLOAT <- -16384.0;
::Constants.Server.MAX_COORD_FLOAT <- 16384.0;

::Constants.Math.vec3_zero <- Vector(0.0, 0.0, 0.0);
::Constants.Math.qang_zero <- QAngle(0.0, 0.0, 0.0);

::Constants.EHudType <- {
	HUDTYPE_UNDEFINED = 0,
	HUDTYPE_CTF = 1,
	HUDTYPE_CP = 2,
	HUDTYPE_ESCORT = 3,
	HUDTYPE_ARENA = 4,
	HUDTYPE_TRAINING = 5
};

local csnt = ::getconsttable();
foreach(i,j in ::Constants) {
	local valid = true;
	foreach(k,l in j) {
		local typ = typeof(l);
		if(typ == "null") {
			j[k] = 0;
			typ = "integer";
		}
		if(typ == "string" || typ == "integer" || typ == "float") {
			csnt[k] <- l;
		} else {
			valid = false;
		}
	}
	if(valid) {
		csnt[i] <- j;
	}
}
