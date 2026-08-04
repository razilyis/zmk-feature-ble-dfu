# zmk-feature-ble-dfu

Adafruit nRF52 BootloaderをBluetooth OTA DFU modeで起動するためのZMK feature moduleです。

> [!IMPORTANT]
> このmoduleは単体では使用できません。提供するのはZMKから対応Bootloaderへ再起動するbehaviorだけです。Firmware packageの生成・検証・BLE転送、PCアプリ、Bootloaderの導入、失敗時のUSB復旧機能は含みません。

## このmoduleだけではできないこと

`&ble_dfu`を実行すると通常のZMK Firmwareを終了し、Adafruit nRF52 BootloaderへOTA reset type `0xA8`を渡して再起動します。moduleの処理はそこで終了します。

Bluetooth経由のFirmware更新には、少なくとも次の構成が別途必要です。

- OTA reset type `0xA8`とNordic Legacy DFUをサポートするAdafruit nRF52 Bootloader
- このmoduleを取り込んだZMK configと、対象キーへ割り当てた`&ble_dfu`またはwrapper behavior
- board、shield、central／peripheralが一致するLegacy DFU package
- packageのside、hash、init packet、device条件を検証できるDFU client
- `AdaDFU`を検出し、Legacy DFU protocolでFirmwareを転送できるPCアプリ
- 更新失敗時に対象sideを書き戻せる、検証済みUSB UF2とdata対応USB cable

KeyClip for Windowsには独立した「BLE OTA DFU」画面を初期実装していますが、現段階では開発PCへ登録したpackage identity付きhelperを利用する実験機能です。helper、署名済みidentity package、証明書信頼、external location登録のinstaller統合は未完了で、正式配布機能ではありません。このrepositoryをZMK configへ追加するだけではBluetooth Firmware更新は完成しません。

左右分割キーボードでは、centralからperipheralを遠隔でOTA modeへ移行する機能もありません。現PoCでは左右それぞれへbehaviorを配置し、更新するsideを物理操作します。

## 開発状態

現在は最小PoCです。`&ble_dfu` behaviorを押すと、Adafruit nRF52 BootloaderのOTA reset type `0xA8`を指定して再起動します。

次の機能はまだ実装していません。

- 押下中キーの全解放待ち
- 長押しや確認操作による誤操作防止
- Firmware packageの転送
- BLE GATTによるPCアプリとの更新準備通信
- split peripheralへの遠隔OTA移行
- active Bootloaderの種類／version検証

このmoduleはFirmwareを書き換えません。再起動後のDFU転送と書き換えはBootloaderおよび別途用意したDFU clientが担当します。

## 確認状況

- AroundFortyDBのZMK v0.3.0構成で右central、左peripheral、settings resetをbuild済み
- Keymap Editorでwrapper `&ble_dfu_ota`がCustom macro「BLE_DFU_OTA」として表示されることを確認済み
- AroundFortyDB実機で`&ble_dfu_ota`押下後に通常接続が切断され、`AdaDFU`としてadvertiseすることを確認済み
- Nordic Legacy DFU service UUID `00001530-1212-efde-1523-785feabcd123`をWindowsから検出済み
- Windowsのpackage identity付きclientからLegacy DFU GATT接続、DFU version 8、notification購読を確認済み
- 最初の診断で`START_DFU` Success responseが約15.6秒後に到着し、10秒timeoutが短すぎることを確認済み
- AroundFortyDBの右centralで全転送を3回、左peripheralで全転送を1回完了済み
- USB未接続の試験では左右とも手動resetなしで通常入力へ復帰することを確認済み
- KeyClipのBLE OTA DFU画面を含む正式な配布・左右順次実機回帰・途中中止／USB復旧試験は未完了

## 対応範囲

- 初期検証対象: ZMK v0.3.0
- 初期board: Seeed Studio XIAO nRF52840 (`seeeduino_xiao_ble`)
- Bootloader: Adafruit nRF52 Bootloader

OTA reset type `0xA8`を解釈しないBootloaderでは使用できません。実機でUSB UF2による復旧手段を確認してから試してください。

Nordic Secure DFU専用Bootloaderや、Zephyr MCUmgr／MCUbootのSMP転送には対応していません。

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

## GitHub Actionsで通常FirmwareとDFU packageを両方作る

通常のZMK buildとDFU package生成は分けてください。標準の`build` jobが作る通常のFirmware artifact `firmware`はKeyClipの「USB AutoFlash」用としてそのまま残し、後続の`package-ble-dfu` jobがそれをdownloadして、KeyClipの「BLE OTA DFU」用artifact `firmware-ble-dfu`を追加生成します。

```text
build
  └─ firmware                 USB AutoFlash用（通常のZMK Firmware ZIP）
       └─ package-ble-dfu
            └─ firmware-ble-dfu  BLE OTA DFU用の追加artifact
```

これにより、GitHub Actionsの同じrunから次の2種類をdownloadできます。

- `firmware`: KeyClipの「USB AutoFlash」でUSB Bootloader modeへ書き込むUF2一式
- `firmware-ble-dfu`: KeyClipの「BLE OTA DFU」で使用する左右のLegacy DFU ZIP、USB復旧用UF2、照合用manifest

`firmware-ble-dfu`は`firmware`の代替ではありません。KeyClipではUSB書き込みに`firmware`、Bluetooth書き込みに`firmware-ble-dfu`を使い分けます。

### 必要な補助script

ZMK config repositoryへ、次の2ファイルを追加します。

- `scripts/uf2_to_ihex.py`: UF2の書き込み先addressを保持したままIntel HEXへ変換し、XIAO nRF52840のfamily IDとapplication領域を検証する
- `scripts/create_ble_dfu_manifest.py`: 左右DFU ZIPのside、file名、SHA-256、sizeとbuild情報を`firmware-manifest.json`へ出力する

`adafruit-nrfutil`へUF2を直接渡すのではなく、一度address保持型Intel HEXへ変換します。単純にUF2からpayloadだけを連結すると書き込み先addressが失われるため、この用途には使用しないでください。

### Workflow例

`.github/workflows/build.yml`の通常buildを残し、その後へ次のjobを追加します。この例は`seeeduino_xiao_ble`を使用する左右分割AroundFortyDBで実機確認した構成です。

```yaml
name: Build ZMK firmware
on: [push, pull_request, workflow_dispatch]

jobs:
  build:
    uses: zmkfirmware/zmk/.github/workflows/build-user-config.yml@v0.3.0

  package-ble-dfu:
    name: Package BLE DFU firmware
    needs: build
    runs-on: ubuntu-latest
    permissions:
      contents: read
    steps:
      - name: Checkout configuration
        uses: actions/checkout@v4

      - name: Download normal ZMK firmware artifact
        uses: actions/download-artifact@v4
        with:
          name: firmware
          path: firmware

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.11"

      - name: Install Adafruit Legacy DFU packaging tool
        run: python -m pip install --disable-pip-version-check adafruit-nrfutil==0.5.3.post16

      - name: Locate left and right UF2 files
        id: uf2
        shell: bash
        run: |
          set -euo pipefail
          left="$(find firmware -maxdepth 1 -type f -name 'around_forty_db_left*-zmk.uf2' -print -quit)"
          right="$(find firmware -maxdepth 1 -type f -name 'around_forty_db_right*-zmk.uf2' -print -quit)"
          test -n "$left"
          test -n "$right"
          echo "left=$left" >> "$GITHUB_OUTPUT"
          echo "right=$right" >> "$GITHUB_OUTPUT"

      - name: Convert UF2 files to address-preserving Intel HEX
        run: |
          python scripts/uf2_to_ihex.py "${{ steps.uf2.outputs.left }}" intermediate/around_forty_db_left.hex
          python scripts/uf2_to_ihex.py "${{ steps.uf2.outputs.right }}" intermediate/around_forty_db_right.hex

      - name: Generate Adafruit Legacy DFU packages
        run: |
          mkdir -p package
          adafruit-nrfutil dfu genpkg --dev-type 0x0052 --sd-req 0xFFFE --application-version "${{ github.run_number }}" --application intermediate/around_forty_db_left.hex package/around_forty_db_left.dfu.zip
          adafruit-nrfutil dfu genpkg --dev-type 0x0052 --sd-req 0xFFFE --application-version "${{ github.run_number }}" --application intermediate/around_forty_db_right.hex package/around_forty_db_right.dfu.zip

      - name: Add recovery UF2 files and manifest
        shell: bash
        run: |
          cp firmware/*.uf2 package/
          python scripts/create_ble_dfu_manifest.py \
            --directory package \
            --repository "${{ github.repository }}" \
            --branch "${{ github.ref_name }}" \
            --run-id "${{ github.run_id }}" \
            --run-number "${{ github.run_number }}" \
            --commit "${{ github.sha }}"

      - name: Upload separate BLE DFU artifact
        uses: actions/upload-artifact@v4
        with:
          name: firmware-ble-dfu
          path: package/
          if-no-files-found: error
```

別のキーボードへ流用するときは、少なくとも次を変更してください。

- `find`の左右UF2 file pattern
- 中間HEXとDFU ZIPのfile名
- manifest script内の`right-central`／`left-peripheral`と対応file名
- board、Bootloader、SoftDeviceに合わせた`--dev-type`と`--sd-req`
- UF2変換script内のfamily ID、application開始address、flash終端address

`0x0052`、`0xFFFE`、family ID `0xADA52840`、application開始address `0x00027000`は、今回確認したXIAO nRF52840 + Adafruit nRF52 Bootloader向けの値です。他boardや別Bootloaderへそのまま流用しないでください。`adafruit-nrfutil`は再現性のため確認済みの`0.5.3.post16`へ固定しています。

生成される`firmware-ble-dfu`には、最低でも次を含めます。

```text
around_forty_db_right.dfu.zip
around_forty_db_left.dfu.zip
around_forty_db_right...-zmk.uf2
around_forty_db_left...-zmk.uf2
settings_reset...-zmk.uf2
firmware-manifest.json
```

DFU ZIPだけでなく同じrunの復旧用UF2も同梱することで、転送失敗時に対象sideをUSBで復旧できます。PCアプリは`firmware-manifest.json`のsideとSHA-256を照合し、左右を取り違えたpackageを転送しないようにしてください。

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

`AdaDFU`へ入った後にDFU clientから`START_DFU`を送ると、application領域の準備・消去が始まる可能性があります。転送を中断すると通常Firmwareへ戻れず、USB UF2での復旧が必要になる場合があります。

対象sideの照合、復旧UF2の照合、USB復旧手順を確認してから使用してください。左右分割では反対sideのFirmwareを選ばないよう特に注意し、BLE DFU中はUSBを接続しない運用を初期条件とします。

## ライセンス

MIT Licenseです。
