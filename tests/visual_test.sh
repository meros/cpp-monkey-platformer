#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
REFERENCE_DIR="$SCRIPT_DIR/reference_screenshots"
OUTPUT_DIR="$BUILD_DIR/visual_test_output"

mkdir -p "$OUTPUT_DIR"

GAME="$BUILD_DIR/monkey_game"

if [ ! -f "$GAME" ]; then
    echo "ERROR: Game binary not found at $GAME"
    echo "       Run: cmake -B build && cmake --build build"
    exit 1
fi

echo "=== Visual Regression Test ==="

# Take a screenshot after 10 frames (enough for physics to settle and render)
echo "Capturing screenshot (10 frames)..."
cd "$PROJECT_DIR"
xvfb-run -s "-screen 0 800x600x24" "$GAME" --screenshot "$OUTPUT_DIR/frame10.png" 10

if [ ! -f "$OUTPUT_DIR/frame10.png" ]; then
    echo "FAIL: Screenshot was not created"
    exit 1
fi

echo "Screenshot captured: $OUTPUT_DIR/frame10.png"

# Check that the screenshot is not blank (all one color)
COLORS=$(identify -format "%k" "$OUTPUT_DIR/frame10.png")
echo "Unique colors in screenshot: $COLORS"
if [ "$COLORS" -le 2 ]; then
    echo "FAIL: Screenshot appears blank (only $COLORS unique colors)"
    exit 1
fi

echo "PASS: Screenshot has real content ($COLORS unique colors)"

# If reference exists, compare
if [ -f "$REFERENCE_DIR/frame10.png" ]; then
    echo "Comparing against reference screenshot..."

    # Compare images - AE (absolute error) counts differing pixels
    # stderr gets the count, redirect it to stdout
    DIFF_PIXELS=$(compare -metric AE \
        "$REFERENCE_DIR/frame10.png" "$OUTPUT_DIR/frame10.png" \
        "$OUTPUT_DIR/diff.png" 2>&1) || true

    # Extract just the number (AE outputs just a number to stderr)
    DIFF_PIXELS=$(echo "$DIFF_PIXELS" | grep -oE '^[0-9]+' | head -1)
    DIFF_PIXELS=${DIFF_PIXELS:-999999}

    TOTAL_PIXELS=$(identify -format "%[fx:w*h]" "$OUTPUT_DIR/frame10.png")

    echo "Different pixels: $DIFF_PIXELS / $TOTAL_PIXELS"

    # Allow up to 5% pixel difference (physics sim may vary slightly)
    MAX_DIFF=$((TOTAL_PIXELS * 5 / 100))
    if [ "$DIFF_PIXELS" -le "$MAX_DIFF" ]; then
        echo "PASS: Visual regression within threshold ($DIFF_PIXELS <= $MAX_DIFF)"
    else
        echo "FAIL: Visual regression exceeds 5% threshold ($DIFF_PIXELS > $MAX_DIFF)"
        echo "  Reference: $REFERENCE_DIR/frame10.png"
        echo "  Actual:    $OUTPUT_DIR/frame10.png"
        echo "  Diff:      $OUTPUT_DIR/diff.png"
        exit 1
    fi
else
    echo "No reference screenshot found."
    echo "To create one, run:"
    echo "  mkdir -p $REFERENCE_DIR"
    echo "  cp $OUTPUT_DIR/frame10.png $REFERENCE_DIR/frame10.png"
    echo "SKIP: No reference to compare against (first run)"
fi

echo "=== Visual test complete ==="
