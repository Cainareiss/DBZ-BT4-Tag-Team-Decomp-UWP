# BT4decomp PS5 port

Early native port for jailbroken PlayStation 5 consoles. The target is an
x86-64 homebrew build of BT4decomp, not PS2 emulation. No game data is stored
in this repository; a compatible BT4 B14 Rev2 ISO will be required.

## Current milestone: PS5 Slim memory and exFAT-aware bootstrap

The game retains 32-bit pointers and expects its heap, stack, emulated PS2
registers and host objects below 4 GiB. The first payload safely probes the
complete proposed address map without replacing an existing mapping. It also
reserves a low secondary-image range at `0x20000000`.

This decides the next implementation path:

- **Probe passes:** compile the existing x86-64 game core as a fixed secondary
  image and load it from the position-independent bootstrap.
- **Probe fails:** adapt the 4 GiB pointer-window transformation used by the
  Apple Silicon port.

The bootstrap now treats the PS5 game-memory allowance as a planning ceiling of
12 GiB. It keeps the recompiled PS2 state in the required low 32-bit window and
reserves up to 8 GiB of the remaining budget for future textures, audio,
renderer buffers and streamed game data. It does **not** allocate 12 GiB at
startup: the probe maps only 64 MiB above 4 GiB, touches its boundary pages and
immediately releases it. This is safer on jailbreak environments whose payload
process limits can differ from retail game processes.

Game content is discovered in this order:

1. `--content-root=/custom/path` or `BT4_CONTENT_ROOT`;
2. `/mnt/usb0..7/bt4` and `/mnt/usb0..7/homebrew/bt4`;
3. `/mnt/ext0..1/bt4` and `/mnt/ext0..1/homebrew/bt4`;
4. `/data/bt4`.

This supports an exFAT USB/NVMe enclosure without hard-coding a single port.
The future ISO installer and asset streamer use the selected content root;
logs, settings and saves remain in `/data/bt4` so unplugging the drive cannot
damage persistent state. On PS5, the filesystem type and free space are logged.

The probe writes `/data/bt4/logs/ps5-port.log`, displays a PS5 notification and
releases every test mapping before exiting.

## Build and run

Use `ps5-payload-dev/sdk` v0.42 or newer; v0.42 added firmware 13.60 support.
The binary SDK also requires a supported Clang/LLVM installation.

```sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
make -C port/ps5

export PS5_HOST=192.168.1.50
export PS5_PORT=9021
make -C port/ps5 test
```

Host-only layout validation does not require the SDK:

```sh
make -C port/ps5 check
make -C port/ps5 host-check
```

The `PS5 port bootstrap` GitHub Actions workflow performs the same checks,
installs the official payload SDK and publishes the resulting ELF as an
artifact. It can be started manually after this branch is pushed.

## Planned platform boundary

The Xbox UWP port is the reference for low-address layout, ISO installation
and platform separation. Windows APIs are not carried over directly.

| Component | PS5 implementation |
| --- | --- |
| Executable bootstrap | `ps5-payload-dev/sdk`, target `x86_64-sie-ps5` |
| Game core | Existing ptr32/LLVM pipeline, fixed secondary image if possible |
| Game files | Auto-detected `/mnt/usb*` or `/mnt/ext*` BT4 directory, with `/data/bt4` fallback |
| Saves/logs | Always `/data/bt4` to survive external-drive removal |
| Renderer | RADV through the PS5 Vulkan homebrew stack |
| Input | DualSense pad service |
| Audio | PS5 AudioOut platform backend |
| Online | Existing deterministic simulation and protocol over PS5 sockets/TLS |

## Milestones

1. **M0 — now:** buildable payload, low/high-memory probes and external-storage discovery.
2. **M1:** compile the decompiled core for `x86_64-sie-ps5`, load its stripped
   data map and reach the headless game entry point.
3. **M2:** ISO installer, filesystem, saves, DualSense and audio.
4. **M3:** Vulkan presentation and the first rendered offline fight.
5. **M4:** Tag Team local, then online/rollback; voice chat comes last.

This directory is experimental. The game is not playable on PS5 yet.
