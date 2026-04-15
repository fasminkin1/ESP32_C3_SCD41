# BLE Communication Interface Specification

This document describes the interface for communicating with the ESP32-C3 SCD41 Air Monitor via Bluetooth Low Energy (BLE).

## 1. General Info
- **Device Name:** `SCD41_Monitor`
- **BT Appearance:** `833` (Multi-sensor)

## 2. Advertising (Passive Monitoring)
The device broadcasts periodic advertising packets. Real-time CO2 data is included in the **Service Data** field to allow monitoring without establishing a connection.

- **Service Data UUID:** `0x181A` (Environmental Sensing)
- **Format:** 2-byte Big Endian integer (CO2 in ppm).
- **Example:** `03 20` (Hex) -> `800` (Decimal) ppm.

## 3. GATT Service & Characteristics
All custom communication happens through the Primary Service.

- **SCD41 Service UUID:** `12345678-abcd-4444-8888-534344343100`

### 3.1 Telemetry Characteristic
Provides full real-time data and device configuration state.

- **UUID:** `12345678-abcd-4444-8888-534344343101`
- **Properties:** `READ`, `NOTIFY`
- **Format:** Minified JSON string.

**JSON Schema:**
```json
{
  "val": {
    "c": 800,    // CO2 (ppm)
    "t": 25.5,   // Temperature (°C)
    "h": 40.0,   // Humidity (%)
    "d": 11.2,   // Dew Point (°C)
    "v": 0.95    // VPD (kPa)
  },
  "cfg": {
    "i": 5000,   // Measurement Interval (ms)
    "a": 1,      // Measurements Active (1 = On, 0 = Off)
    "tm": 0      // Timer Remaining (minutes)
  },
  "s": 0         // Status Code (0=OK, 1=WARN, 2=CRIT)
}
```

### 3.2 Command Characteristic
Used to send configuration commands to the device.

- **UUID:** `12345678-abcd-4444-8888-534344343102`
- **Properties:** `WRITE`, `WRITE_WITHOUT_RESP`
- **Format:** UTF-8 String (Shell command).

**Available Commands:**
| Command | Description | Example |
|---------|-------------|---------|
| `app active <on/off>` | Start/Stop measurements | `app active off` |
| `app interval <ms>` | Set fetch interval | `app interval 10000` |
| `app timer <min>` | Set auto-stop timer | `app timer 60` |
| `app lang <en/ru>` | Change device language | `app lang en` |
| `app log <on/off>` | Toggle serial logging | `app log on` |

## 4. Status Codes (Field `s`)
- **0 (STATUS_OK):** Normal operation.
- **1 (STATUS_WARN):** Warning level (CO2 > 1000 ppm or Humidity outside 25-75% range).
- **2 (STATUS_CRIT):** Critical level (CO2 > 1500 ppm).
