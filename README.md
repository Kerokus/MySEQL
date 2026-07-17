# MySEQL — MySEQ Open for EverQuest Legends

A fork of **MySEQ Open** (the classic-EverQuest map/radar tool) adapted for **EverQuest Legends**,
a modern **64-bit** fork of classic EQ. The server reads the live `eqgame.exe` process memory,
walks the in-game entity list, and streams each spawn's name/position/level/class/race/etc. to the
GUI client, which plots them on the zone map.

> Read-only observation of your own client's RAM. No client anti-cheat is present, but **radar-style
> tools are commonly against server rules** even when they only read your own memory — know
> EverQuest Legends' stance before using this on a live account.

## How it works

```
client (C# WinForms radar)  <--TCP 5555-->  server (C++, ReadProcessMemory)  -->  eqgame.exe
```

The server is fully **offset-config-driven** (`EQL-myseqserver.ini`). EQL kept classic EQ's
fat/inline spawn struct — just shifted, and 64-bit — so adapting it was mostly an ini rewrite plus
one small traversal change (the entity list needs a two-hop manager deref) and a class decoder.

### Spawn-list traversal (EQL)

```
manager    = [eqgame.exe + 0xF88728]     ; SpawnHeaderAddr (ImageBase-relative, 0x140000000 base)
firstSpawn = [manager + 0x10]            ; SpawnListHop  (EQL-only extra deref)
next       = [spawn   + 0x10]            ; NextOffset    (walk forward until next == 0)
```

`SpawnListHop` is an EQL-only ini key: when non-zero the server does the extra `+0x10` manager hop
and walks forward-only. Absent/0 = original single-deref live-EQ behavior (backward compatible).

### Confirmed SpawnInfo offsets (entity struct, live-verified)

| Field | Offset | Notes |
|------|--------|-------|
| Next / Prev | `0x10` / `0x08` | EmbeddedList node |
| X / Y / Z | `0x74 / 0x78 / 0x7c` | plain IEEE floats (EQ `/loc` order Y,X,Z) |
| Heading | `0x94` | 0–512 |
| Name | `0xb8` | decorated tag, e.g. `#Trooper_Axlia00` |
| Lastname | `0xf8` | clean display name |
| Type | `0x139` | byte: NPC=1 / PC=0 / corpse=2–4 |
| SpawnID | `0x178` | dword |
| Level | `0x5c1` | byte |
| **Race** | `0x10F4` | dword; **standard EQ race id** (22=Beetle, 37=Snake, 44=Guard, 54=Orc, 128=Iksar…) |
| **Class** | `0x1100` | dword **BITMASK** (bit N = class N; multiclass = OR of bits) — see below |
| Model/anim component | `0x1298` | pointer |

Other static globals: local player `[eqgame.exe+0xF04430]` (also used for `CharInfo`),
zone-info short name `0x140F0EA50` (`ZoneAddr`).

### Class is a bitmask (Hero's Journey multiclass)

EQL stores class as a **bitmask** (`1<<classid`), OR-ing bits for multiclass characters
(e.g. Anomander = `0xC80` = MNK|SHM|NEC). The client expects a single class **id**, so the server
converts the bitmask to the primary (lowest set) bit when `ClassIsBitmask=1`
(`Spawn::lowestSetBitIndex`). Multi-bit spawns are players; single-bit are NPCs
(orc priest → Cleric, dervish cutthroat → Rogue, crusader → Shadow Knight, generic → Warrior).

## Build

Both projects target **VS2022**. (See the client note below re: VS2026.)

**Server** (C++, `Release|x64`):
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ^
  server\MySEQ.server.vcxproj -p:Configuration=Release -p:Platform=x64
```
Output: `server\x64\Release\server.exe`

**Client** (C# .NET Framework 4.8 WinForms, `Release|x64`):
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ^
  client\MySEQ.client.csproj -t:restore,build -p:Configuration=Release -p:Platform=x64
```
Output: `client\bin\x64\Release\MySEQ.exe`

> **VS2026 gotcha:** MSBuild v18 fails with `MSB3821` on four `.resx` (legacy BinaryFormatter image
> data). Use VS2022's MSBuild (or the VS2022 IDE). If a freshly-unzipped tree complains about
> "mark of the web", run `Get-ChildItem -Recurse -File | Unblock-File`.

## Run

Launch EverQuest Legends and log in first, then:

**1. Server** — run **from the Release folder** (the `-f` handler resolves a bare filename against the
*current directory*, and it also looks there for `config.ini`):
```
cd server\x64\Release
.\server.exe -f EQL-myseqserver.ini
```
(Or pass a full ini path from anywhere: `server.exe -f "D:\...\EQL-myseqserver.ini"`.)
The server window shows the zone (`nro`), your character name, and live spawn counts once connected.

**2. Client** — run `client\bin\x64\Release\MySEQ.exe` and connect to `127.0.0.1:5555`.

> **cfg must sit next to the client exe.** The client reads class/race names from `cfg\Classes.txt`
> and `cfg\Races.txt` at `<exe dir>\cfg`. The build does **not** auto-copy them, so after building
> copy `client\cfg\*` into `client\bin\x64\Release\cfg\` — otherwise every class/race shows
> "id: Unknown".

## Maps

**Maps are not included** in this repo or in releases — EQ map files are third-party community work
and are not redistributed here. To draw the zone under the spawns, create a `maps` folder next to the
client exe (`client\maps\`) and add EQ map `.txt` files yourself; the client also creates this folder
on first run. Multiple map sets can be kept as sub-folders and swapped from the **Map → Use Map Pack**
menu. (The `maps` folder is git-ignored.)

## Status

**Working:** spawn radar (names + positions), zone auto-map, level, type (PC/NPC/corpse), heading,
speed, spawn id, class, race, char name.

**Not yet mapped for EQL:** ground items, world/time (`WorldAddr`), target (`TargetAddr`),
inventory (`ItemsAddr`), and per-spawn primary/offhand/owner (parked, read 0).

## License

Fork of **MySEQ Open** (© the ShowEQ/MySEQ developers), released under the **GNU GPL** — see
`License.rtf` / `server\gpl-3.0.txt`. This adaptation is provided under the same terms.
