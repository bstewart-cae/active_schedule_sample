# Linux x86
This folder contains Linux x86 implementation of platform dependent components.

## Variants
Linux x86 PAL has two implementations that can be selected during build with `PLATFORM_VARIANT` CMake option:
* Real-time - `x86_REALTIME`
* Emulated tick - `x86_EMULATED`

### Real-time
In this mode application runs in real-time. Tick events are generated with system timer using `ITIMER_REAL`. For the application to work properly, it is required that no other process preempts it for a longer period of time (e.g. 100ms), as it would cause application to be unresponsive. To limit that possibility it is recommended to use `PREEMPT_RT` kernel with `CONFIG_HZ_1000` option and run sample applications on CPU cores separate from non-real-time processes.

### Emulated tick
In this mode tick is synchronized with external tick server. Application communicates with server using Unix domain socket provided with `--tick-server <path>` command line option. On startup, application registers with tick server to receive tick events. These events are delivered to application via `SIGCONT` signal. Application signals end of tick to the server when it enters IDLE task or immediately when it's sleeping. Tick server sends next tick event when all currently registered applications signals end of current tick.

#### Tick Server Commands
| Command        | Value | Request data   | Response data           |
|--------------- |------ |--------------- |------------------------ |
| Register       | 0x01  | PID (4 bytes)  | Current tick (4 bytes)  |
| Tick End       | 0x02  | PID (4 bytes)  |                         |
| Get Tick       | 0x03  |                | Current tick (4 bytes)  |
| Set Timer      | 0x04  |                | Timeout in ms (4 bytes) |
| Cancel Timer   | 0x05  |                |                         |

Data is send with little-endian order. Response is optional, sent only when command returns some data (e.g. current tick). `Set Timer` response is sent when timer times out. Each client can set only one timer.

Example implementation of tick server can be found in [z-wave-test-system repository](https://github.com/Z-Wave-Alliance/z-wave-test-system/blob/main/z_wave_ts/src/z_wave_ts/tick_server.py).

## AppsHw
This folder contains implementation for abstractions specific for each sample application.

## PAL

### Radio
Physical layer is replaced by Z-Wave Network Emulation library. Some properties like speed or channel are simulated and sent as a metadata with actual radio data. Please see [ZNE](#z-wave-network-emulation) for more information.

Simulated radio region can be changed with `--region <region>` argument of an application.

### NVM / Retention Registers
Host filesystem is used for storage. Separate directory is used for each nvm area (`zpal_nvm_area_t`) and retention registers:

| Area                              | Folder            |
|---------------------------------- |------------------ |
| ZPAL_NVM_AREA_APPLICATION         | storage/nvm_app   |
| ZPAL_NVM_AREA_STACK               | storage/nvm_stack |
| ZPAL_NVM_AREA_MANUFACTURER_TOKENS | storage/mfg_token |
| Retention registers               | storage/retention |

Data is stored as a raw binary files with names in following format `<id>.bin`.

Base path for storage can be changed with `--storage <path>` argument of an application.

### UART
UART has two implementations:
* TCP server

  This is default mode. Application listens on port `4901`. It can be changed with `--port <port>` argument. Node can accept connection from controller application e.g. `PC Controller`.

* PTY

  This mode uses `pseudo-terminal` and can be enabled with `--pty` argument. After pseudo-terminal is allocated, application prints its path to standard output, e.g.:
  ```bash
  PTY: /dev/pts/1
  ```

TCP server or pseudo-terminal is created in parent process. Data is forwarded between parent and child processes using pipes. This allows to keep uninterrupted connection while application is restarting.

### Debug output
Debug output use standard output.

### Not supported interfaces
Implementation of the following interfaces is minimal:
* Bootloader - Firmware update is not supported


## Z-Wave Network Emulation
Z-Wave Network Emulation is a library that replaces communication over physical radio with communication over local network.

### Modes
ZNE has three modes:
* Multicast

  Communicates via multicast UDP with following parameters:

  | IP Address | Port |
  |----------- |----- |
  | 224.0.0.0  | 4321 |

  All sent messages are forwarded to all nodes (including node that sent this message). Message header contains PID of the process to allow for a node to filter out messages sent by itself.

* Routed (Real-time)

  Uses client-server architecture. Communicates via UDP. `ZNE` server listens on port specified by `--zne-port <PORT>` argument. Application uses port value of `<PORT> + <ID>` where `<ID>` is set by `--id <ID>`. 

* Routed (Emulated Tick)

  Uses client-server architecture. Communicates via Unix Socket. `ZNE` server listens on `zne.socket` located in TMP directory specified by`--tmp-path <PATH>` argument.

### ZNE Header
ZNE Header consists of following fields:
* ZNE ID - ID of a node in ZNE network. For Multicast mode it is PID of node process. For Routed mode it has a value of `<PORT> + <ID>`.
* Speed - Z-Wave Speed
* Channel - Z-Wave Channel
* Region - Z-Wave Region
* RSSI - frame RSSI

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             ZNE ID                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Speed     |    Channel    |     Region    |      RSSI     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Command FIFO
STDIN/OUT is used for communication by default. FIFO mode can be enabled with `--fifo` argument. In this mode three FIFOs are used:
| File name  | Purpose        |
|----------- |--------------- |
| `in.fifo`  | Command input  |
| `out.fifo` | Command output |
| `ev.fifo`  | Event output   |

FIFOs have to be created at the location specified by `--storage <path>` argument, before application is started. Messages use binary format with variable length payload.

##### Headers
```
Command request / Event         Command response

 0 1 2 3 4 5 6 7                 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+               +-+-+-+-+-+-+-+-+
|Command / Event|               |    Command    |
+-+-+-+-+-+-+-+-+               +-+-+-+-+-+-+-+-+
|     Length    |               |     Status    |
+-+-+-+-+-+-+-+-+               +-+-+-+-+-+-+-+-+
                                |     Length    |
                                +-+-+-+-+-+-+-+-+
```

#### Commands
| Command        | Value | Input data     | Output data        |
|--------------- |------ |--------------- |------------------- |
| Quit           | 0     |                |                    |
| Restart        | 1     |                |                    |
| Wake Up        | 2     |                |                    |
| Get DSK        | 3     |                | 16 bytes of DSK    |
| Get Node ID    | 4     |                | 2 bytes of Node ID |
| Get PTY        | 5     |                | PTY path           |
| AppsHw Command | 6     | `char` command |                    |

#### Events
| Event   | Value |
|-------- |------ |
| Startup | 0     |

#### Statuses
| Status             | Value |
|------------------- |------ |
| OK                 | 0     |
| Failed             | 1     |
| Unknown command    | 2     |
| Invalid parameters | 3     |

<!--
SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>

SPDX-License-Identifier: BSD-3-Clause
-->

