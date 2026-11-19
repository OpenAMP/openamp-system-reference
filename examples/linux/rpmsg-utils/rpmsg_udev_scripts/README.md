# RPMsg udev Scripts

Udev rules and helper scripts to automatically manage RPMsg endpoint devices.
When an RPMsg channel is created by the remote processor, these scripts handle
device permissions and endpoint export automatically.

## Files

| File | Description |
|------|-------------|
| `99-rpmsg.rules` | udev rules — triggers scripts on RPMsg char device add/remove events |
| `rpmsg_create_channel.sh` | Creates RPMsg endpoint for a new channel using `rpmsg_export_ept` |

## How It Works

When the remote processor starts and establishes an RPMsg channel, the kernel
creates devices under `/sys/bus/rpmsg/devices/`. The udev rules trigger the
scripts in the following order:

```
Remote processor boots
    ↓
RPMsg channel created → virtio0.rpmsg-raw.-1.1026
    ↓ udev ACTION==add
rpmsg_create_channel.sh  → creates /dev/rpmsg0 via rpmsg_export_ept
    ↓ udev ACTION==add (rpmsg0 endpoint)
/dev/rpmsg0 permissions set to group rpmsg and mode 0660
```

## Installation
```bash
# Install udev rule and helper script from examples/linux/rpmsg-utils
sudo make install-rpmsg-udev

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## User Group Setup

The udev rule assigns `/dev/rpmsg*` devices to the `rpmsg` group with
`MODE="0660"` — only members of the `rpmsg` group can read and write the
devices.

```bash
# Create rpmsg group, or optionally can be created by default during rootfs build
sudo groupadd rpmsg

# Add user to the group
sudo usermod -aG rpmsg <username>

# Apply group membership without logout
newgrp rpmsg
```

> **Note:** Users must be members of the `rpmsg` group to access `/dev/rpmsg*`
> devices. Changes to group membership require logout/login or `newgrp rpmsg`
> to take effect.

## udev Rule Logic

```
SUBSYSTEM=="rpmsg", ACTION=="add"
    │
    ├── KERNEL=="virtio*.rpmsg_ctrl.0.0"  → SKIP (control device)
    ├── KERNEL=="virtio*.rpmsg_ns.53.53"  → SKIP (namespace device)
    ├── KERNEL=="rpmsg_ctrl[0-9]*"        → SKIP (ctrl device)
    ├── KERNEL=="rpmsg[0-9]*"             → set GROUP="rpmsg", MODE="0660"
    └── KERNEL=="virtio*.rpmsg-raw"       → rpmsg_create_channel.sh
```

## Requirements

- `rpmsg_export_ept` utility available in `/usr/bin`
- udev running on the target system

## Debugging

```bash
# Monitor udev events
udevadm monitor --udev --subsystem-match=rpmsg

# Test rule match without executing
udevadm test /sys/bus/rpmsg/devices/<device>

# Check udev logs
journalctl -u systemd-udevd -f
```
