<img src="assets/logo.svg" alt="dominiius" width="500px">

_Reach your Wii U using a `.local` domain_

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Releases](https://img.shields.io/github/v/release/fengb/dominiius)](https://github.com/fengb/dominiius/releases)

---

### Installation

1. Download the latest `dominiius.wps` from [Releases](https://github.com/fengb/dominiius/releases)
2. Copy it into `sd:/wiiu/environments/aroma/plugins`

### Usage

While the plugin is active, the Wii U becomes reachable by its Wii nickname:

```sh
ping wiiu.local
```

- The domain is active while the console is online
- Spaces and symbols in the nickname become dashes: `My Wii` → `my-wii.local`

### Technical details

1. **Nickname**
    - Read once at plugin load — a nickname change is assumed to require a full reboot.
    - Parsed out of `sys/proc/prefs/wii_acct.xml` from a manually mounted SLC NAND partition.

2. **Listener**
    - Expects some mDNS traffic every few minutes and will forcibly reconnect if the wire goes quiet.
    - Stops on `ON_APPLICATION_REQUESTS_EXIT` — waiting until `ON_APPLICATION_ENDS` risks the socket being double closed by the plugin and the system shutdown.

### Building

The repo uses Docker:

```sh
docker compose run --rm app make
```

## Credits

- [mjansson/mdns](https://github.com/mjansson/mdns)
- [Wii U Plugin System](https://github.com/wiiu-env/WiiUPluginSystem)
- [WUT](https://github.com/devkitPro/wut)
- [libmocha](https://github.com/wiiu-env/libmocha)