## Verify HailoRT Installation

Make sure that hailo is identified correctly by running this command: `hailortcli fw-control identify`, The expected output should look similar to the one below:

```sh
$ hailortcli fw-control identify
Identifying board
Control Protocol Version: 2
Firmware Version: X.X.X (develop,app)
Logger Version: 0
Board Name: Hailo-8
Device Architecture: HAILO8_B0
Serial Number: 0000000000000009
Part Number: HM218B1C2FA
Product Name: HAILO-8 AI ACCELERATOR M.2 M KEY MODULE
```