# Matrix Portal ESP32-S3 — bike counter display
# Scans BLE advertisements from sensor unit, renders running total
# as amber 7-segment digits on black background.
#
# BLE packet format (must match sensor firmware):
#   AD type 0xFF (manufacturer specific)
#   Company ID : 0xFFFF  (2 bytes, little-endian)
#   Count      : uint16  (2 bytes, little-endian)

import struct
import board
import displayio
import framebufferio
import rgbmatrix
import _bleio

# ── Display ────────────────────────────────────────────────────────────────────

displayio.release_displays()
matrix = rgbmatrix.RGBMatrix(
    width=64, height=32, bit_depth=2,
    rgb_pins=[board.MTX_R1, board.MTX_G1, board.MTX_B1, board.MTX_R2, board.MTX_G2, board.MTX_B2],
    addr_pins=[board.MTX_ADDRA, board.MTX_ADDRB, board.MTX_ADDRC, board.MTX_ADDRD],
    clock_pin=board.MTX_CLK, latch_pin=board.MTX_LAT, output_enable_pin=board.MTX_OE,
)
display = framebufferio.FramebufferDisplay(matrix, auto_refresh=True)

palette = displayio.Palette(2)
palette[0] = 0x000000
palette[1] = 0xFF4400  # orange

bitmap = displayio.Bitmap(64, 32, 2)
group  = displayio.Group()
group.append(displayio.TileGrid(bitmap, pixel_shader=palette))
display.root_group = group

# ── 7-segment renderer ─────────────────────────────────────────────────────────

DW = 12   # digit width
DH = 30   # digit height
ST = 3    # segment thickness
GX = 4    # gap between digits

#        a  b  c  d  e  f  g
SEG = [
    (1, 1, 1, 1, 1, 1, 0),  # 0
    (0, 1, 1, 0, 0, 0, 0),  # 1
    (1, 1, 0, 1, 1, 0, 1),  # 2
    (1, 1, 1, 1, 0, 0, 1),  # 3
    (0, 1, 1, 0, 0, 1, 1),  # 4
    (1, 0, 1, 1, 0, 1, 1),  # 5
    (1, 0, 1, 1, 1, 1, 1),  # 6
    (1, 1, 1, 0, 0, 0, 0),  # 7
    (1, 1, 1, 1, 1, 1, 1),  # 8
    (1, 1, 1, 1, 0, 1, 1),  # 9
]

def _rect(x, y, w, h):
    for py in range(y, min(y + h, 32)):
        for px in range(x, min(x + w, 64)):
            bitmap[px, py] = 1

def _digit(ox, oy, n):
    a, b, c, d, e, f, g = SEG[n]
    m = DH // 2
    if a: _rect(ox + 1,       oy,       DW - 2, ST) # top
    if b: _rect(ox + DW - ST, oy,       ST,     m)  # top-right
    if c: _rect(ox + DW - ST, oy + m,   ST,     m)  # bot-right
    if d: _rect(ox + 1,       oy + DH - ST, DW - 2, ST) # bottom
    if e: _rect(ox,           oy + m,   ST,     m)  # bot-left
    if f: _rect(ox,           oy,       ST,     m)  # top-left
    if g: _rect(ox + 1,       oy + m - 1, DW - 2, ST) # middle

def render(n):
    bitmap.fill(0)
    digits = [int(ch) for ch in str(max(0, n))]
    nd     = len(digits)
    ox     = (64 - (nd * DW + (nd - 1) * GX)) // 2
    oy     = (32 - DH) // 2
    for i, d in enumerate(digits):
        _digit(ox + i * (DW + GX), oy, d)

# ── BLE ────────────────────────────────────────────────────────────────────────

COMPANY_ID = 0xFFFF  # must match sensor firmware

def _read_count(raw):
    """Walk BLE AD structures, return count if our manufacturer data is present."""
    i = 0
    while i < len(raw) - 1:
        ln = raw[i]
        if ln == 0:
            break
        if raw[i + 1] == 0xFF and ln >= 5:
            if struct.unpack_from('<H', raw, i + 2)[0] == COMPANY_ID:
                return struct.unpack_from('<H', raw, i + 4)[0]
        i += ln + 1
    return None

# ── Main ───────────────────────────────────────────────────────────────────────

render(0)
current = 0

while True:
    try:
        for entry in _bleio.adapter.start_scan(
            minimum_rssi=-90,
            active=False,
            timeout=2.0,
        ):
            count = _read_count(bytes(entry.advertisement_bytes))
            if count is not None and count != current:
                current = count
                render(current)
    except _bleio.BluetoothError:
        _bleio.adapter.stop_scan()
