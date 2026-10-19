# Qt + aasdk 車載システム簡易プロトタイプ

x86_64 PC 向けに、Qt Quick（QML）+ C++ + CMake で車載システムの簡易プロトタイプを作成したものです。
Android Auto の接続処理には、aasdk リポジトリの `tools/simple` サンプル実装を**変更せずに再利用**しています。

> **重要**: これは完成品ではなく、最小構成の試作品です。
> **ビルド・実行は本作業環境では未実施（未検証）** です。手順と実装内容のみをまとめています。

---

## 1. ファイル構成

| ファイル | 役割 |
| --- | --- |
| `CMakeLists.txt` | ビルド設定（Qt 6 と aasdk の探索・リンク） |
| `src/main.cpp` | アプリケーション起動、QML 読み込み、QML 型の登録 |
| `src/AndroidAutoController.h` / `.cpp` | aasdk（`tools/simple`）を利用した接続処理と接続状態の管理 |
| `QML/Main.qml` | メイン画面（時計 / 接続状態 / Android Auto 表示領域 / ダミー CAN / 設定ボタン） |
| `QML/Settings.qml` | 設定画面（仮実装、表示のみ） |
| `resources.qrc` | QML ファイルをリソースへ埋め込む |
| `aasdk/tools/simple/usb_init.cpp` ほか | **既存サンプル（変更していない）**。`usb_init.cpp` をビルド対象に加えて再利用する |

---

## 2. `aasdk/tools/simple` の調査結果（再利用の根拠）

- `main.cpp`: 起動後に `UsbInit usbInit; usbInit.initDevice();` を呼ぶだけ。終了時は「(Stub) Exiting.」をログ出力して終了する。
- `usb_init.cpp` / `.hpp`:
  - `libusb_init` でコンテキストを作成し、`libusb_get_device_list` で USB デバイスを列挙する。
  - VID `0x18D1`（Google）のデバイスを対象に、PID `0x2D00`〜`0x2D05`（AOAP アクセサリモード）かどうかを判定する。
  - 既にアクセサリモードの場合: `aasdk::usb::AccessoryModeQueryFactory` / `AccessoryModeQueryChainFactory` で `IAccessoryModeQueryChain` を生成し、`queryChain->start(handle, promise)` の後 `ioService_.run()` で待機する。
  - 通常モードの場合: libusb の制御転送で AOAP 文字列（manufacturer / model / description / version / URI / serial）を送信し、`ACCESSORY_START` を送ってデバイスの再列挙を待つ（最大 2 回の試行）。
- `README.md`: 「media channel、audio、video sink、TLS のロジックはこの最小版には存在しない」と明記されている。
- `CMakeLists.txt`: `aasdk` / `pthread` / `ssl` / `crypto` / `usb-1.0` をリンク（＝ aasdk とその依存がシステムにインストール済みである前提）。
- `build.sh`: `cmake -DCMAKE_BUILD_TYPE=...` + `make` で単体ビルド。

**結論**: 再利用できるのは「USB AOAP のハンドシェイク（アクセサリモードへの切り替えと検出）」まで。
映像（MediaSink / Video）や通信（Transport / Messenger）の実装はサンプルに存在しないため、本プロトタイプでは扱いません。

---

## 3. 前提パッケージ（Ubuntu / Debian の例）

```sh
# Qt 6（Quick / QML 実行時モジュールを含む）
sudo apt install qt6-base-dev qt6-declarative-dev qml6-module-qtquick

# ビルドツールと aasdk の依存
sudo apt install build-essential cmake pkg-config git
sudo apt install libusb-1.0-0-dev libssl-dev protobuf-compiler libprotobuf-dev libboost-all-dev
```

`aasdk` 本体は事前にビルド／インストールしておきます（リポジトリ直下の手順を使用）。

```sh
cd aasdk
./build.sh release           # libaasdk をビルド
cd build-release && sudo cpack -G DEB && sudo dpkg -i libaasdk*.deb
```

---

## 4. ビルドと実行

```sh
# このリポジトリ（headunit_prototype）のルートで実行
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/headunit
```

- aasdk をソースツリー内（`./aasdk`）や任意の場所に置いている場合:

```sh
cmake -S . -B build -DHEADUNIT_AASDK_DIR=/path/to/aasdk
```

- aasdk を使わず UI のみをビルドする場合（接続状態は「バックエンド未使用」と表示されます）:

```sh
cmake -S . -B build -DHEADUNIT_WITH_AASDK=OFF
```

USB デバイスへアクセスするには、udev ルールやユーザー権限の設定が必要になる場合があります（`aasdk/QUICK_REFERENCE.md` 参照）。

---

## 5. 実装状況

### 実装済み（コード作成済み）

- Qt Quick のメイン画面（`QML/Main.qml`）と設定画面（`QML/Settings.qml`）、`Loader` による簡易な画面遷移
- 現在時刻（時計・日付）の表示
- Android Auto の接続状態表示（`AndroidAutoController` のプロパティ）
- Android Auto の表示領域（プレースホルダ）
- CAN 情報のダミー表示（車速 / 回転数 / 水温を QML の `Timer` で擬似生成）
- `tools/simple` の `usb_init.cpp` を別スレッドで実行する接続処理（`AndroidAutoController::startHandshake()`）
- USB バスの監視（1 秒間隔の libusb 列挙）による接続状態の更新
- aasdk が無い環境・デバイスが無い環境でもアプリが起動し、状態表示のみで動作する構成

### 未実装（意図的に対象外）

- Android Auto の映像受信・デコード・画面表示（`tools/simple` に映像処理が存在しないため。`videoSupported` は常に `false`）
- Transport / Messenger / ControlServiceChannel などの通信初期化（サンプルに存在しないため推測実装しない）
- 実際の CAN 通信（ダミーデータのみ）
- 設定項目の保存と動作への反映（表示のみの仮実装）
- 接続処理スレッドの安全な停止（終了時は `detach()` してプロセス終了に任せる）
- マルチタッチや入力転送など、映像投影に付随する機能

### 未検証（この作業環境ではビルド・実行を実施していない）

- CMake 構成の成否、コンパイル、リンク、アプリの起動
- QML の描画および `AndroidAutoController` のプロパティ表示
- 実機（Android 端末）を接続した際の AOAP ハンドシェイクの成否

---

## 6. 制限事項・注意点

- `AndroidAutoController::AccessoryMode` は「AOAP アクセサリモードのデバイスを検出した」状態を表します。プロトコルネゴシエーション以降（映像投影など）は未実装です。
- サンプル実装 `UsbInit::initDevice()` は `boost::asio::io_service::run()` で待機する場合があり、その場合はバックグラウンドスレッドが終了しません。終了時は当該スレッドを切り離しています。
- 監視用とハンドシェイク用でそれぞれ libusb コンテキストを持ちます（走査は読み取りのみ）。
- コメントはすべての関数・メソッドの直前に記述し、C++ は Doxygen 形式としています。
