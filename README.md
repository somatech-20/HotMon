# HotMon - Global Hotkey Monitor Tool (windows)

Ever wondered why your favorite shortcut isn’t working, or which app is hijacking your hotkeys? **HotMon** gives you instant answers. 

**HotMon** is a lightweight, command-line tool designed to help power users, developers, and IT professionals monitor and troubleshoot global hotkey usage on Windows. 

## Features

- ✅ Detect all modifier-based combos (Ctrl, Alt, Shift, Win)
- ✅ Live logging of hotkeys + foreground app
- ✅ Export logs to CSV
- ✅ Probe system-wide hotkey availability
- ✅ Command-line friendly (with aliases)
<!-- - ✅ Silent or timed capture modes -->

## Usage
`HotMon.exe [flags]`

### CLI Flags
| Flag		      |Alias	      | Description
|---------|-----|-------------|
| --probe       | -p          |   Probe available hotkeys and store
|  --search     | -f <kw>       |   Search hotkey combo
|  --live-log   | -l, -i        |   Start live or Interactive monitor
|  --help       | -h            |   Show this help message
---

## Hotkey Controls (While Running)
- Press **Ctrl + F10** → Save log as `hotkey_log.csv`
- Press **Ctrl + F11** → Probe hotkeys and auto-save to `hotkey_available.csv`
- Press **ESC** → Exit

## Output Files
- `hotkey_log.csv`: Records [timestamp, hotkey, application]
- `hotkey_available.csv`: Records [hotkey, availability status]

## Build Instructions
Use Visual Studio (with Desktop C++ workload) or:

```bash
g++ hotkey_monitor.cpp -o hotkey_monitor.exe -luser32 -lpsapi
```

## License
[HotMon](#hotmon---global-hotkey-monitor-tool-windows) is licensed under the GNU General Public License (GPL). See the [LICENSE](LICENSE) file for details.
