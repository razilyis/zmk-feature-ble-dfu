# zmk-feature-ble-dfu

Adafruit nRF52 BootloaderをBluetooth OTA DFU modeで起動するためのZMK feature moduleです。

## 開発状態

現在は最小PoCです。`&ble_dfu` behaviorを押すと、Adafruit nRF52 BootloaderのOTA reset type `0xA8`を指定して再起動します。

次の機能はまだ実装していません。

- 押下中キーの全解放待ち
- 長押しや確認操作による誤操作防止
- Firmware packageの転送
- BLE GATTによるPCアプリとの更新準備通信
- split peripheralへの遠隔OTA移行
- active Bootloaderの種類／version検証

このmoduleはFirmwareを書き換えません。再起動後のDFU転送と書き換えはBootloaderおよびDFU clientが担当します。

## 確認状況

- AroundFortyDBのZMK v0.3.0構成で右central、左peripheral、settings resetをbuild済み
- Keymap Editorでwrapper `&ble_dfu_ota`がCustom macro「BLE_DFU_OTA」として表示されることを確認済み
- AroundFortyDB実機で`&ble_dfu_ota`押下後に通常接続が切断され、`AdaDFU`としてadvertiseすることを確認済み
- Nordic Legacy DFU service UUID `00001530-1212-efde-1523-785feabcd123`をWindowsから検出済み
- reset 1回で通常Firmwareへ復帰することを確認済み。DFU package転送は未確認

## 対応範囲

- 初期検証対象: ZMK v0.3.0
- 初期board: Seeed Studio XIAO nRF52840 (`seeeduino_xiao_ble`)
- Bootloader: Adafruit nRF52 Bootloader

OTA reset type `0xA8`を解釈しないBootloaderでは使用できません。実機でUSB UF2による復旧手段を確認してから試してください。

## 導入

`config/west.yml`へmoduleを追加します。

```yaml
remotes:
  - name: razilyis
    url-base: https://github.com/razilyis

projects:
  - name: zmk-feature-ble-dfu
    remote: razilyis
    revision: main
```

対象shieldの`.conf`で有効にします。

```conf
CONFIG_ZMK_BLE_DFU=y
```

keymapでbehaviorをincludeし、任意のキーへ割り当てます。

```dts
#include <behaviors/ble_dfu.dtsi>

&ble_dfu
```

## Keymap Editor

Keymap Editorは外部ZMK moduleの`.dtsi`や独自compatibleをbehavior pickerへ直接登録しません。pickerから割り当てる場合は、keymap内へ標準`zmk,behavior-macro` wrapperを追加します。

```dts
/ {
    macros {
        ble_dfu_ota: ble_dfu_ota {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&macro_tap &ble_dfu>;
            label = "BLE_DFU_OTA";
        };
    };
};
```

Keymap Editorでは`&ble_dfu_ota`をcustom macroとして選択します。Firmware buildではwrapperからmoduleの`&ble_dfu`を呼び出します。

## 注意

`&ble_dfu`は現在、押下直後に再起動します。誤操作を避けるため、通常入力layerへ直接配置せずSettings layer、長押し、または安全なcomboから呼び出してください。

## ライセンス

MIT Licenseです。
