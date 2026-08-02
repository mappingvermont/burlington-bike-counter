# Display Unit — Matrix Portal S3

**Main reference:** https://learn.adafruit.com/adafruit-matrixportal-s3

The display unit receives BLE advertisements from the sensor unit and renders the running bike count as large amber digits on a black background.

## BOM (~$135)

| Part | Source | Link | Price |
|---|---|---|---|
| 64×32 RGB LED Matrix, 6mm pitch | Adafruit | [#2276](https://www.adafruit.com/product/2276) | $64.95 |
| Matrix Portal ESP32-S3 | Adafruit | [#5778](https://www.adafruit.com/product/5778) | $20.00 |
| Anker Power Bank 20K 87W | Amazon | [Amazon](https://www.amazon.com/Anker-Portable-Charger-Charging-Battery/dp/B0CXDXP8VR) | $50.00 |

Datasheet: [adafruit-matrixportal-s3.pdf](adafruit-matrixportal-s3.pdf)

## Power

- At ~300mA draw (~15Wh/day): lasts ~4 days per charge
- Inside enclosure: built-in USB-C cable from Anker directly to Matrix Portal
- Charging: USB-C panel mount connector on enclosure exterior → second USB-C port on Anker inside. Plug a standard USB-C charger in from outside without opening the box.
- Daytime only — no need to run overnight

---

## Connecting to the Board

- When plugged in, macOS will show an "Allow Accessory to Connect" dialog that disappears quickly. If you miss it, go to **System Settings → Privacy & Security → Accessories** and approve it there
- Once approved, the board mounts as **CIRCUITPY** in Finder and `/Volumes/CIRCUITPY/` in the shell
- A serial port also appears at `/dev/cu.usbmodemXXXX` (the suffix changes each session)

Find the current port with:
```
ls /dev/cu.usbmodem*
```

## Copying Files to the Board

Drop `code.py` directly onto the CIRCUITPY volume — the board auto-restarts and runs it immediately:
```
cp code.py /Volumes/CIRCUITPY/code.py
```

For quick one-off commands without copying a file, use `mpremote`:
```
mpremote connect /dev/cu.usbmodemXXXX exec "import board; print(dir(board))"
```

Install mpremote if needed: `pip3 install mpremote`

---

## Physical Connection

- The Matrix Portal plugs into the **left-side JIN connector** on the back of the panel (left = right when viewing from the front)
- The Matrix Portal **overhangs the edge** of the panel — it does not sit flush on the back
- The **white arrow on the panel back** should point up and right when correctly oriented
- **Remove the amber tape** from the two power standoffs before connecting — this prevents proper seating
- After removing tape, connect the power wires from the Matrix Portal's screw terminals to the panel's power input
- The connector is keyed — if you see garbage/partial output, the board is flipped the wrong way

---

## Pin Names

The Matrix Portal S3 uses `MTX_` prefixed pin names, not the generic names found in most HUB75 examples:

```python
matrix = rgbmatrix.RGBMatrix(
    width=64, height=32, bit_depth=4,
    rgb_pins=[board.MTX_R1, board.MTX_G1, board.MTX_B1,
              board.MTX_R2, board.MTX_G2, board.MTX_B2],
    addr_pins=[board.MTX_ADDRA, board.MTX_ADDRB, board.MTX_ADDRC, board.MTX_ADDRD],
    clock_pin=board.MTX_CLK, latch_pin=board.MTX_LAT, output_enable_pin=board.MTX_OE,
)
```

## Color

Amber: `0xFFAA00` — brighter than orange for outdoor visibility. Current as of
`code.py`; tuned twice from an initial `0xFF4400` orange.

The panel's green LEDs are 3-4× more luminous than red (per datasheet), so achieving orange/amber requires much less green than you'd expect from RGB color pickers.

---

## Flashing CircuitPython

If the board ships with pre-loaded demo code that disables the USB drive:

1. Hold BOOT, unplug USB, replug USB, release BOOT → enters ROM bootloader
2. `pip3 install esptool`
3. `esptool --port /dev/cu.usbmodemXXXX --chip esp32s3 --baud 460800 write_flash -z 0x0 firmware.bin`
4. Download the UF2 file from `downloads.circuitpython.org/bin/adafruit_matrixportal_s3/en_US/`
5. The board will reboot into UF2 mode (mounts as `MATRXS3BOOT`) — drag the `.uf2` onto it
6. Board reboots into clean CircuitPython, CIRCUITPY mounts
