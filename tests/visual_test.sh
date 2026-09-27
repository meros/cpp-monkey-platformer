#!/usr/bin/env bash
# Leafwind visual regression (docs/DESIGN.md 8.3): screenshot levels 1, 4, 7 and 10 after 120
# fixed steps (deterministic: seeded particles, t = frames/60) and compare against
# tests/reference_screenshots/ with ImageMagick RMSE, threshold 0.08.
#   tests/visual_test.sh [path/to/monkey_game]      compare
#   UPDATE_REFERENCES=1 tests/visual_test.sh ...     regenerate the references
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
GAME="${1:-$PROJECT_DIR/build/monkey_game}"
REFERENCE_DIR="$SCRIPT_DIR/reference_screenshots"
OUTPUT_DIR="$(dirname "$GAME")/visual_test_output"
THRESHOLD=0.08
mkdir -p "$OUTPUT_DIR" "$REFERENCE_DIR"

if [ ! -x "$GAME" ]; then
    echo "ERROR: game binary not found at $GAME (build first: cmake -B build && cmake --build build)"
    exit 1
fi

cd "$PROJECT_DIR"
status=0
for level in 1 4 7 10; do
    out="$OUTPUT_DIR/level$level.png"
    ref="$REFERENCE_DIR/level$level.png"
    xvfb-run -a -s "-screen 0 800x600x24" "$GAME" --screenshot "$level" 120 "$out" > /dev/null 2>&1
    if [ ! -f "$out" ]; then
        echo "FAIL level $level: screenshot not created"
        status=1
        continue
    fi
    colors=$(identify -format "%k" "$out")
    if [ "$colors" -le 16 ]; then
        echo "FAIL level $level: screenshot looks blank ($colors colours)"
        status=1
        continue
    fi
    if [ "${UPDATE_REFERENCES:-0}" = "1" ] || [ ! -f "$ref" ]; then
        cp "$out" "$ref"
        echo "level $level: reference written ($colors colours)"
        continue
    fi
    # compare prints "abs (normalized)" to stderr and exits 1 when images differ at all
    rmse=$(compare -metric RMSE "$ref" "$out" "$OUTPUT_DIR/diff$level.png" 2>&1 >/dev/null || true)
    norm=$(echo "$rmse" | sed -n 's/.*(\([0-9.e+-]*\)).*/\1/p')
    norm=${norm:-1}
    if awk "BEGIN{exit !($norm <= $THRESHOLD)}"; then
        echo "PASS level $level: RMSE $norm <= $THRESHOLD"
    else
        echo "FAIL level $level: RMSE $norm > $THRESHOLD (reference $ref, actual $out, diff $OUTPUT_DIR/diff$level.png)"
        status=1
    fi
done
exit $status
