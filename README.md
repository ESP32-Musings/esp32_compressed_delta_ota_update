 ![Status](https://img.shields.io/badge/status-experimental-red)
# ESP32 Compressed Delta OTA Updates

## About the Project

The project aims at enabling firmware update of ESP32 Over-the-Air with compressed delta binaries. Testing was done with ESP32-DevKitC v4 board.
## Getting Started

### Hardware Required

To run the OTA demo, you need an ESP32 dev board (e.g. ESP32-WROVER Kit) or ESP32 core board (e.g. ESP32-DevKitC).
You can also try running on ESP32-S2, ESP32-C3 or ESP32-S3 dev boards and let me know how it worked out.

### Prerequisites

* **ESP-IDF v4.3 and above**

  You can visit the [ESP-IDF Programmming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#installation-step-by-step) for the installation steps.

* **detools v0.49.0 and above**

  Binary delta encoding in Python 3.6+. You can follow the instructions [here](https://pypi.org/project/detools/) for installation.

## Usage

1. Build the example `examples/http_delta_ota` - `idf.py build`. You can use the binary built here as the `base_binary`.

   For example usage, a `while(1)` loop printing 'Hello World!' was added to this example to create the `updated_binary`.

2. To generate the patch, we need 2 application binaries, namely `base_binary` and `updated_binary`.

   `detools create_patch -c heatshrink base_binary.bin updated_binary.bin patch.bin`

3. Open the project configuration menu (`idf.py menuconfig`) go to `Example Connection Configuration` ->
    1. WiFi SSID: WiFi network to which your PC is also connected to.
    2. WiFi Password: WiFi password

4. In order to test the OTA demo -> `examples/http_delta_ota` :
    1. Flash the firmware `idf.py -p PORT -b BAUD flash`.
    2. Run `idf.py -p PORT monitor` and note down the IP assigned to your ESP module. The default port is 80.

5. After getting the IP address, send the patch binary through a HTTP Post request over cURL.

   `curl -v -X POST --data-binary @- < patch.bin 192.168.201.9:80/ota`

## Demo Results

  - Base binary: `assets/v1.bin`
  - Updated binary: `assets/v2.bin`
  - Patch binary: `assets/patch_1_2.bin`
  - After successfully patching and rebooting, you can see the `Hello World` logs.

https://user-images.githubusercontent.com/42297532/152674614-2f4d3b9d-08d2-45bc-a3b6-29f33a191fd7.mp4

## Experiments

| Chip  | Scenario                                     | Base binary | Updated binary | Compressed binary patch (Heatshrink) | Patch-to-File % |
|-------|----------------------------------------------|-------------|----------------|--------------------------------------|-----------------|
| ESP32 | test_basic_enable_small_feature              | 168208      | 155136         | 11036                                | 7.11%           |
| ESP32 | test_nvs_app_modification                    | 190656      | 199824         | 16245                                | 8.13%           |
| ESP32 | test_http_upgrade_with_ssl                   | 672736      | 761184         | 138839                               | 18.24%          |
| ESP32 | test_provisioning_upgrade_idf_patch_version  | 966656      | 924448         | 234096                               | 25.32%          |

- As Heatshrink uses static allocation with small look-ahead buffers, it has almost no impact on heap memory.
### Test Scenarios: ESP-IDF 4.4-dev (Master branch)

1. **test_basic_enable_small_feature:** Enabling a small feature in an update

    Base binary: Compile `get-started/hello-world` example

    Updated binary: Same, but disable CONFIG_VFS_SUPPORT_IO option in sdkconfig

2. **test_nvs_app_modification**: Changing user application flow without changing the set of libraries used

    Base binary: Compile `storage/nvs_rw_value` example

    Updated binary: Compile `storage/nvs_rw_blob` example

3. **test_http_upgrade_with_ssl**: Changing user application flow with new set of libraries added

    Base binary: Compile `protocols/http_server/simple` example

    Updated binary: Compile `protocols/https_server/simple` example

4. **test_provisioning_upgrade_idf_patch_version**: Upgrading IDF to the next patch version

    Base binary: Using IDF 4.3, compile `provisioning/wifi_prov_mgr` example

    Update binary: Same, but with IDF 4.3.1; sdkconfig is generated from scratch.

## To-do:

- [x] Experiments with more test scenarios, especially with bigger binaries

- [x] Add complete workflow example
  - Currently patch is applied to the `ota_0` partition rather than the `factory`

- [ ] Optimize Heatshrink compression parameters
  - Currently running on low memory usage mode (8, 7) and static allocation
  - Memory usage and app binary size analysis

- [ ] Binary patch with LZMA compression in detools
  - LZMA is memory-heavy but will provide a greater compression ratio for the patch

## Acknowledgements & Resources

- detool: Binary delta encoding in Python 3 and C 🡒 [Source](https://github.com/eerimoq/detools) | [Docs](https://detools.readthedocs.io/en/latest/)
- heatshrink: An Embedded Data Compression Library 🡒 [Source](https://github.com/atomicobject/heatshrink) | [Blog](https://spin.atomicobject.com/2013/03/14/heatshrink-embedded-data-compression/)
- Delta updates for embedded systems 🡒 [Source](https://gitlab.endian.se/thesis-projects/delta-updates-for-embedded-systems) | [Docs](https://odr.chalmers.se/bitstream/20.500.12380/302598/1/21-17%20Lindh.pdf)
- bspatch for ESP32 🡒 [Source](https://github.com/Blockstream/esp32_bsdiff)

## License

Distributed under the MIT License. See `LICENSE` for more information.

**Note**: The `delta` component is licensed under the Apache-2.0 License. Please see `components/delta/LICENSE` for more information.
