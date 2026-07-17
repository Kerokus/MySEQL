/*==============================================================================

	Copyright (C) 2006-2024  All developers at https://www.showeq.net/forums/forum.php

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  ==============================================================================*/

#include "stdafx.h"
#include "Spawn.h"

Spawn::Spawn(void)

{
	// reserve space for 700 spawns, which should cover most zones.

	// note: vectors will grow as needed even if we exceed the 700

	spawnList.reserve(700);

	largestOffset = 0;
}

void Spawn::setOffset(offset_types ot, UINT value, const string& ptrName)
{
	offsets[ot] = value;
	ptrNames[ot] = ptrName;
}

void Spawn::init(IniReaderInterface* ir_intf)
{
	if (ir_intf->readIntegerEntry("SpawnInfo Offsets", "EightBitRace") == 0)
		race8 = false;
	else
		race8 = true;

	// EQL: class field is a bitmask, not an id byte (see Spawn.h). Off (0) = classic id-byte behavior.
	classIsBitmask = (ir_intf->readIntegerEntry("SpawnInfo Offsets", "ClassIsBitmask") != 0);

	// EQL: Hide field is a signed invis-type int32, not a hide-enum byte (see Spawn.h). Off (0) = classic byte.
	hideIsInvisType = (ir_intf->readIntegerEntry("SpawnInfo Offsets", "HideIsInvisType") != 0);

	setOffset(OT_name, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "NameOffset"), "First Name");
	setOffset(OT_lastname, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "LastNameOffset"), "Last Name");
	setOffset(OT_x, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "XOffset"), "X");
	setOffset(OT_y, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "YOffset"), "Y");
	setOffset(OT_z, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "ZOffset"), "Z");
	setOffset(OT_speed, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "SpeedOffset"), "Speed");
	setOffset(OT_heading, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "HeadingOffset"), "Heading");
	setOffset(OT_prev, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "PrevOffset"), "Previous");
	setOffset(OT_next, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "NextOffset"), "Next");
	setOffset(OT_type, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "TypeOffset"), "Type");
	setOffset(OT_level, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "LevelOffset"), "Level");
	setOffset(OT_hidden, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "HideOffset"), "Hidden");
	setOffset(OT_class, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "ClassOffset"), "Class");
	setOffset(OT_id, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "SpawnIDOffset"), "Id");
	setOffset(OT_owner, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "OwnerIDOffset"), "OwnerID");
	setOffset(OT_race, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "RaceOffset"), "Race");
	setOffset(OT_primary, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "PrimaryOffset"), "Primary");
	setOffset(OT_offhand, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "OffhandOffset"), "Offhand");
	// EQL: multiclass bitmask (dword). Secondary/tertiary = its set bits minus the primary class.
	setOffset(OT_multiclass, (UINT)ir_intf->readIntegerEntry("SpawnInfo Offsets", "MultiClassOffset"), "MultiClass");
	// Determine how many bytes we should read for each spawn
	largestOffset = 0;
	for (int i = 0; i < OT_max; i++)
	{
		if (offsets[i] > largestOffset)
			largestOffset = offsets[i];
	}

	largestOffset += 30;
	// The raw buffer is what where we dump raw data from the EQ process into
	rawBuffer = new char[largestOffset];
	cout << "Spawn: Spawn Offsets read in." << endl;
}

/* the rest of the net buffer can be filled in directly, but this makes
   it easier to pack in the string information */

void Spawn::packNetBufferStrings(UINT flags, const string& firstName, const string& lastName)
{
	memset(tempNetBuffer.name, 0, 30);
	memset(tempNetBuffer.lastName, 0, 22);
	firstName._Copy_s(tempNetBuffer.name, 30, 30);
	lastName._Copy_s(tempNetBuffer.lastName, 22, 22);
	tempNetBuffer.flags = flags;
	tempNetBuffer.class2 = 0;   // default; set in packNetBufferRaw for real spawns
	tempNetBuffer.class3 = 0;
}

void Spawn::packNetBufferRaw(UINT flags, QWORD _this)
{
	packNetBufferStrings(flags, extractRawString(OT_name), extractRawString(OT_lastname));
	tempNetBuffer.x = extractRawFloat(OT_x);
	tempNetBuffer.y = extractRawFloat(OT_y);
	tempNetBuffer.z = extractRawFloat(OT_z);
	tempNetBuffer.heading = extractRawFloat(OT_heading);
	tempNetBuffer.speed = extractRawFloat(OT_speed);
	// When the packet is for process id's use this
	// otherwise use the real id.
	if (flags == 0x06) {
		tempNetBuffer.id = (UINT)_this;
	}
	else {
		tempNetBuffer.id = extractRawDWord(OT_id);
	}

	tempNetBuffer.type = extractRawByte(OT_type);
	// Primary class: EQL uses a plain id byte @ ClassOffset (0x10FC). (ClassIsBitmask=1 keeps the old
	// bitmask->lowest-bit behavior for compatibility.)
	tempNetBuffer._class = classIsBitmask
		? lowestSetBitIndex(extractRawDWord(OT_class))
		: extractRawByte(OT_class);
	// Secondary/tertiary class: the multiclass bitmask's set bits, excluding the primary class id.
	{
		DWORD mcMask = offsets[OT_multiclass] ? extractRawDWord(OT_multiclass) : 0;
		BYTE extras[2] = { 0, 0 };
		int n = 0;
		for (int bit = 1; bit <= 31 && n < 2; ++bit)
			if (((mcMask >> bit) & 1u) && bit != tempNetBuffer._class)
				extras[n++] = (BYTE)bit;
		tempNetBuffer.class2 = extras[0];
		tempNetBuffer.class3 = extras[1];
	}

	if (race8 == true) {
		// Use an 8 bit race, and a 16 bit material id
		tempNetBuffer.race = extractRawByte(OT_race);
		tempNetBuffer.primary = extractRawWord(OT_primary);
		tempNetBuffer.offhand = extractRawWord(OT_offhand);
		tempNetBuffer.id = extractRawWord(OT_id);
		tempNetBuffer.owner = extractRawWord(OT_owner);
	}
	else {
		tempNetBuffer.race = extractRawDWord(OT_race);
		tempNetBuffer.primary = extractRawDWord(OT_primary);
		tempNetBuffer.offhand = extractRawDWord(OT_offhand);
		tempNetBuffer.owner = extractRawDWord(OT_owner);
	}
	tempNetBuffer.level = extractRawByte(OT_level);

	if (hideIsInvisType) {
		// EQL stores invis as a signed int32 at HideOffset (live-verified on the local player):
		//   0 = visible, 1 = invisible, -2 = invis-vs-undead, -1 = invis-vs-animals.
		// Translate to MySEQ's Hide enum (0 Vis,1 Invis,2 Hidden,3 IVU,4 IVA) so the client's
		// hide-status label resolves correctly with no client change.
		switch (extractRawInt(OT_hidden)) {
		case  0: tempNetBuffer.hidden = 0; break;  // visible
		case  1: tempNetBuffer.hidden = 1; break;  // invisible
		case -2: tempNetBuffer.hidden = 3; break;  // invis vs undead
		case -1: tempNetBuffer.hidden = 4; break;  // invis vs animals
		default: tempNetBuffer.hidden = 1; break;  // any other nonzero invis state -> Invisible
		}
	}
	else {
		tempNetBuffer.hidden = extractRawByte(OT_hidden);
	}
}

void Spawn::packNetBufferEmpty(UINT flags, QWORD _this)
{
	packNetBufferStrings(flags, "", "");
	tempNetBuffer.id = 99999;
}

void Spawn::packNetBufferFrom(const Item& item)
{
	packNetBufferStrings(item.tempItemBuffer.flags, item.tempItemBuffer.name, "");
	tempNetBuffer.x = item.tempItemBuffer.x;
	tempNetBuffer.y = item.tempItemBuffer.y;
	tempNetBuffer.z = item.tempItemBuffer.z;
	tempNetBuffer.id = item.tempItemBuffer.id;
}

void Spawn::packNetBufferWorld(const World& world)
{
	packNetBufferStrings(world.tempWorldBuffer.flags, "", "");
	tempNetBuffer.type = world.tempWorldBuffer.hour;
	tempNetBuffer._class = world.tempWorldBuffer.minute;
	tempNetBuffer.level = world.tempWorldBuffer.day;
	tempNetBuffer.hidden = world.tempWorldBuffer.month;
	tempNetBuffer.race = world.tempWorldBuffer.year;
}