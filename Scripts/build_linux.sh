#!/usr/bin/env bash
# Mars Hello World - Linux build (works inside the ghcr.io/epicgames/unreal-engine dev image).
# Usage: Scripts/build_linux.sh [UE_ROOT]   (default: $UE_ROOT or /home/ue4/UnrealEngine)
set -euo pipefail
UE_ROOT="${1:-${UE_ROOT:-/home/ue4/UnrealEngine}}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT="$HERE/MarsHelloWorld.uproject"
OUT="${OUT_DIR:-$HERE/Build/Linux}"

[ -x "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" ] || { echo "Unreal Engine not found at $UE_ROOT" >&2; exit 1; }

echo "=== 1/2 Generating voices and materials (mars_setup.py) ==="
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT" -run=pythonscript -script="mars_setup.py" \
  -unattended -nop4 -nosplash -nullrhi || echo "WARNING: asset setup failed; the game still runs with fallbacks."

echo "=== 2/2 BuildCookRun ==="
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PROJECT" -noP4 -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -iostore -package -archive -archivedirectory="$OUT" -utf8output

echo "Done. Output in $OUT"
