<!--
SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
SPDX-FileCopyrightText: Z-Wave-Alliance <https://z-wavealliance.org/>

SPDX-License-Identifier: BSD-3-Clause
-->

# Door Lock

The Z-Wave certified Door Lock application shows a lock implementation. It supports user codes to open a door and thereby eliminate the need for traditional keys. It is possible to both lock and unlock the door remotely through the Z-Wave protocol.

The Door Lock application is based on:

| <!-- -->                | <!-- -->                                                                |
| ----------------------- | ----------------------------------------------------------------------- |
| Role Type               | Listening Sleeping End device (LSS / FLiRS)                             |
| Supporting Device Type  | Actuator                                                                |
| Device Type             | Lock                                                                    |
| Generic Type            | Entry Control                                                           |
| Specific Type           | Door Lock                                                               |
| Requested security keys | S0 and S2_ACCESS<br>The Door Lock features depend on the security level |

The application will only work when securely added to a network by a controller supporting security.
The controller MUST support security S2_Access_Control or S0 in order to be able to control the door lock.

**Not** implemented Door lock functionality:

- Timed Operation mode
- Door Lock condition
- Inside Door Handle State
- Functionality handling Lock timeout
- Target mode
- Auto-Relock, Hold And Release, Block to Block, and Twist Assist

## Supported Command Classes

The Door Lock application implements mandatory and some optional command classes. The table below lists the supported Command Classes, their version, and their required Security class, if any.

| Command Class             | Version | Required Security Class |
| ------------------------- |:-------:| ----------------------- |
| Association               | 2       | S0 or Access Control    |
| Association Group Info    | 3       | S0 or Access Control    |
| Basic                     | 2       | S0 or Access Control    |
| Battery                   | 1       | S0 or Access Control    |
| Device Reset Locally      | 1       | S0 or Access Control    |
| Door Lock                 | 4       | S0 or Access Control    |
| Firmware Update Meta Data | 5       | S0 or Access Control    |
| Indicator                 | 3       | S0 or Access Control    |
| Manufacturer Specific     | 2       | S0 or Access Control    |
| Multi-Channel Association | 3       | S0 or Access Control    |
| Powerlevel                | 1       | S0 or Access Control    |
| Security 0                | 1       | None                    |
| Security 2                | 1       | None                    |
| Supervision               | 1       | None                    |
| Transport Service         | 2       | None                    |
| User Code                 | 1       | S0 or Access Control    |
| User Credential           | 1       | S0 or Access Control    |
| Notification              | 8       | S0 or Access Control    |
| Version                   | 3       | S0 or Access Control    |
| Z-Wave Plus Info          | 2       | None                    |

## Basic Command Class mapping

The Basic Command Class is mapped according to the table below.


| Basic Command                       | Mapped Command                                     |
| ----------------------------------- | ---------------------------------------------------|
| Basic Set (Value)                   | Door Lock Operation Set (Door Lock Mode)           |
| Basic Report (Current Value = 0x00) | Door Lock Operation Report (Door Lock Mode = 0x00) |
| Basic Report (Current Value = 0xFF) | Door Lock Operation Set (Door Lock Mode)           |

## Association Groups

The table below shows the available association groups, including supported command classes for Z-Wave and Z-Wave Long Range respectively.

<table>
<tr>
    <th>ID</th>
    <th>Name</th>
    <th>Node Count</th>
    <th>Description</th>
</tr><tr>
    <td>1</td>
    <td>Lifeline</td>
    <td>X</td>
    <td>
        <p>Supports the following command classes:</p>
        <ul>
            <li>Device Reset Locally: triggered upon reset.</li>
            <li>Battery: Triggered upon low battery.</li>
            <li>Door Lock: Triggered upon a change in door lock configuration.</li>
            <li>Door Lock: Triggered upon a change in door lock operation.</li>
            <li>Indicator Report: Triggered when LED1 changes state.</li>
            <li>User Code: Triggered when a user code record is modified.</li>
            <li>User Credential: Triggered when a Duress User enters a Credential.</li>
            <li>Notification: Triggered when a User changes lock state via a user credential.</li>
        </ul>
    </td>
</tr>
</table>

X: For Z-Wave node count is equal to 5 and for Z-Wave Long Range it is 1.

## Usage on Linux platform - zwave_soc_door_lock_keypad
Application uses single character commands read from standard input.

| Character  | Description                                                    |
|----------- | ---------------------------------------------------------------|
| 1          | Send battery report                                            |
| 2          | Activate/deactivate outside door handle with User Code         |
| 3          | Activate/deactivate outside door handle with User Credential   |
| 4          | Start Credential Learn process for UUID=1 Type=PIN Slot=2      |
| 5          | Enter hard-coded data (1667) for the Credential Learn process  |
