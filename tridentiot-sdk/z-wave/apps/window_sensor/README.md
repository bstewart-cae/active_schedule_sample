# Window sensor

The Z-Wave Window Sensor application reports when a window (or a door) is getting opened/closed.

| <!-- -->                  | <!-- -->                                  |
|---------------------------|-------------------------------------------|
| Role Type                 | Reporting Sleeping End device (RSS)       |
| Supporting Device Type    | Data reporting                            |
| Device Type               | Notification sensor                       |
| Generic Type              | Sensor Notification                       |
| Specific Type             | Notification Sensor                       |
| Requested security keys   | S2_UNAUTHENTICATED, and S2_AUTHENTICATED  |

The Window Sensor transmits the following CC Notification types/events:

-   Home access control
    -   Window/door open
    -   Window/door close

The Window Sensor supports the "push mode" only of Notification CC.

## Supported Command Classes

The Window Sensor implements mandatory and optional command classes. The table below lists the supported command classes, their version, and their required security class, if any.

| Command Class             | Version | Required Security Class        |
| ------------------------- |:-------:| ------------------------------ |
| Association               |    2    | Highest granted Security Class | 
| Association Group Info    |    3    | Highest granted Security Class |
| Battery                   |    1    | Highest granted Security Class |
| Device Reset Locally      |    1    | Highest granted Security Class |
| Firmware Update Meta Data |    5    | Highest granted Security Class |
| Indicator                 |    3    | Highest granted Security Class |
| Manufacturer Specific     |    2    | Highest Granted Security Class |
| Multi-Channel Association |    3    | Highest granted Security Class |
| Notification              |    8    | Highest granted Security Class |
| Powerlevel                |    1    | Highest granted Security Class |
| Security 2                |    1    | None                           |
| Supervision               |    1    | None                           |
| Transport Service         |    2    | None                           |
| Version                   |    3    | Highest granted Security Class |
| Wake Up                   |    2    | Highest granted Security Class |
| Z-Wave Plus Info          |    2    | None                           |

## Basic Command Class mapping

Basic Command Class is not mapped to any of the supported command classes.

## Association Group configuration

Application Association Group configuration

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
            <li>Battery: triggered upon low battery.</li>
            <li>
                Notification Report: triggered when the window (or door) is opened (simulated by pressing and holding BTN2) and when the window (or door) is closed (simulated by releasing BTN2).
            </li>
            <li>Indicator Report: Triggered when LED1 changes state.</li>
        </ul>
    </td>
</table>

X: For Z-Wave node count is equal to 5 and for Z-Wave Long Range it is 1.