# Builds a self-contained MySEQL release zip from the current Release build outputs.
# Run AFTER building both projects (Release|x64).  Usage:  .\package-release.ps1 -Version 1.0.0
#
# Maps are intentionally NOT bundled (users supply their own; we don't redistribute third-party
# map packs). The client creates an empty maps\ folder on first run.
param(
    [string]$Version = "1.0.0"
)
$ErrorActionPreference = "Stop"
$root  = Split-Path -Parent $MyInvocation.MyCommand.Path
$stage = Join-Path $root "dist\MySEQL-v$Version"
$cr    = Join-Path $root "client\bin\x64\Release"
$sr    = Join-Path $root "server\x64\Release"

Remove-Item (Join-Path $root "dist") -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$stage\server","$stage\client\cfg" | Out-Null

# --- server: standalone exe (statically linked) + inis ---
Copy-Item "$sr\server.exe","$sr\myseqserver.ini","$sr\config.ini" "$stage\server"

# --- client: exe + dlls + config + cfg\ (skip pdb/xml/manifest/application/app.publish/maps) ---
Copy-Item "$cr\MySEQ.exe","$cr\MySEQ.exe.config" "$stage\client"
Copy-Item "$cr\*.dll" "$stage\client"
Copy-Item "$cr\cfg\*" "$stage\client\cfg" -Recurse

# --- quick-start note ---
@"
MySEQL - EverQuest Legends radar (server + client)

SETUP
  1. Launch EverQuest Legends and log in to your character.
  2. Run  server\server.exe   (if it can't read the game, right-click > Run as administrator).
  3. Run  client\MySEQ.exe , then click Connect (defaults to 127.0.0.1:5555).

MAPS (not included)
  This build ships without maps. To see the zone drawn under the spawns, put EQ map .txt files in a
  'maps' folder next to MySEQ.exe (client\maps\). You can organize multiple sets as sub-folders and
  switch between them from the  Map > Use Map Pack  menu. Map files are widely available online;
  they are not distributed here.

NOTES
  - Requires Windows 10/11 (.NET Framework 4.8 is built in). Run the client on the same PC as the game.
  - This build matches a specific EQL client patch. If a game patch breaks it, a new release is needed.
"@ | Set-Content "$stage\START-HERE.txt" -Encoding UTF8

# --- zip ---
$zip = Join-Path $root "dist\MySEQL-v$Version.zip"
Compress-Archive -Path "$stage\*" -DestinationPath $zip -Force
"{0}  ({1:N1} MB)" -f $zip, ((Get-Item $zip).Length / 1MB)
