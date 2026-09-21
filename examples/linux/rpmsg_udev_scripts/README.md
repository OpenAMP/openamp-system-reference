# RPMsg udev Scripts

Udev rules and helper scripts to automatically manage RPMsg endpoint devices.
When an RPMsg channel is created by the remote processor, these scripts handle
device permissions, symlink creation, and endpoint export automatically.

## Files

| File | Description |
|------|-------------|
| `99-rpmsg.rules` | udev rules — triggers scripts on RPMsg char device add/remove events |
| `rpmsg_create_channel.sh` | Creates RPMsg endpoint for a new channel using `rpmsg_export_ept` |
| `rpmsg_add_ept_symlink.sh` | Creates user-friendly symlinks and sets device permissions on endpoint add |
| `rpmsg_remove_ept_symlink.sh` | Removes symlinks when endpoint is removed |

## How It Works

When the remote processor starts and establishes an RPMsg channel, the kernel
creates devices under `/sys/bus/rpmsg/devices/`. The udev rules trigger the
scripts in the following order:

```
Remote processor boots
    ↓
RPMsg channel created → virtio0.rpmsg-openamp-demo-channel.-1.1024
    ↓ udev ACTION==add
rpmsg_create_channel.sh  → creates /dev/rpmsg0 via rpmsg_export_ept
    ↓ udev ACTION==add (rpmsg0 endpoint)
rpmsg_add_ept_symlink.sh → creates symlink
    └── /dev/rpmsg_rpmsg-openamp-demo-channel.-1.1024.rpmsg0  → /dev/rpmsg0
```

On endpoint removal:
```
udev ACTION==remove (rpmsg0)
    ↓
rpmsg_remove_ept_symlink.sh → removes symlink


```
When removing the symlink, we need to find out which rpmsgX device is mapped
to which symlink. The symlink name ends with `.rpmsgX`, so the removal script
removes links matching `/dev/rpmsg_*.rpmsgX`.

## Installation
```bash
# Install scripts and udev rule on a target system
sudo make install

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

For staged installs used by Yocto and other build systems:

```bash
make DESTDIR=<image-root> install
```

The Makefile supports the standard install variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `DESTDIR` | empty | Staging root used by package/image builds |
| `prefix` | `/usr` | Install prefix |
| `bindir` | `$(prefix)/bin` | Directory for helper scripts |
| `udevdir` | `/etc/udev/rules.d` | Directory for udev rules |

Example:

```bash
make DESTDIR="${D}" prefix=/usr udevdir=/lib/udev/rules.d install
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

## Symlink Naming Convention

| Symlink | Example | Points to |
|---------|---------|-----------|
| `rpmsg_ept_name.src.dst.rpmsg_dev` | `/dev/rpmsg_rpmsg-openamp-demo-channel.-1.1024.rpmsg0` | `/dev/rpmsg0` |

## udev Rule Logic

```
SUBSYSTEM=="rpmsg", ACTION=="add"
    │
    ├── KERNEL=="virtio*.rpmsg_ctrl.0.0"  → SKIP (control device)
    ├── KERNEL=="virtio*.rpmsg_ns.53.53"  → SKIP (namespace device)
    ├── KERNEL=="rpmsg_ctrl[0-9]*"        → SKIP (ctrl device)
    ├── KERNEL=="rpmsg*[0-9]*"            → rpmsg_add_ept_symlink.sh
    └── default                           → rpmsg_create_channel.sh
```

## Requirements

- Linux kernel 6.18 or later (`rpmsg_add_ept_symlink.sh`)
- `rpmsg_export_ept` utility available in `/usr/bin`
- udev running on the target system

## Debugging

# Monitor udev events
udevadm monitor --udev --subsystem-match=rpmsg

# Test rule match without executing
udevadm test /sys/bus/rpmsg/devices/<device>

# Check udev logs
journalctl -u systemd-udevd -f
```
