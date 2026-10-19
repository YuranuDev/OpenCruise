/**
 * @file AndroidAutoController.cpp
 * @brief AndroidAutoController の実装。
 *
 * Android Auto への接続処理は aasdk リポジトリの tools/simple サンプル実装
 * （usb_init.cpp）をそのまま呼び出して実行する。本クラスは次の 2 つを担当する。
 *  - libusb による USB バスの監視（タイマー、UI スレッド）
 *  - サンプル実装（UsbInit::initDevice）の実行（別スレッド）
 * 映像の受信・デコード・表示は tools/simple に処理が存在しないため実装しない。
 */

#include "AndroidAutoController.h"

#include <QTimer>

#include <memory>

#ifdef HEADUNIT_HAS_AASDK
#include <libusb.h>

#include <atomic>
#include <cstdint>
#include <thread>

// aasdk tools/simple のサンプル実装（変更せずにそのまま再利用する）。
#include "usb_init.hpp"
#endif

namespace {

/// USB バスを走査する間隔（ミリ秒）。
constexpr int kMonitorIntervalMs = 1000;

#ifdef HEADUNIT_HAS_AASDK
/// Android デバイス（Google）の USB ベンダ ID。
constexpr std::uint16_t kGoogleVendorId = 0x18D1;
/// AOAP アクセサリモードの PID 範囲（サンプル実装で使われている範囲）。
constexpr std::uint16_t kAoapProductIdMin = 0x2D00;
constexpr std::uint16_t kAoapProductIdMax = 0x2D05;

/**
 * @brief USB 走査の結果を保持する構造体。
 */
struct UsbScanResult {
    bool googleDeviceFound = false;        ///< VID 0x18D1 のデバイスが見つかったか
    bool accessoryModeFound = false;       ///< AOAP アクセサリモードのデバイスが見つかったか
    std::uint16_t accessoryProductId = 0;  ///< 見つかった AOAP デバイスの PID
    int errorCode = 0;                     ///< libusb のエラーコード（0 は正常）
};

/**
 * @brief USB バスを走査し、Android Auto の対象デバイスの有無を調べる。
 * @param context 監視用の libusb コンテキスト。
 * @return 走査結果。
 */
UsbScanResult scanUsbDevices(libusb_context *context)
{
    UsbScanResult result;

    libusb_device **devices = nullptr;
    const ssize_t count = libusb_get_device_list(context, &devices);

    // デバイス一覧の取得に失敗した場合はエラーコードを返す。
    if (count < 0) {
        result.errorCode = static_cast<int>(count);
        return result;
    }

    // 一覧を走査し、Google 製デバイスとアクセサリモードのデバイスを探す。
    for (ssize_t i = 0; i < count; ++i) {
        libusb_device_descriptor descriptor{};
        if (libusb_get_device_descriptor(devices[i], &descriptor) != 0) {
            continue;
        }
        if (descriptor.idVendor != kGoogleVendorId) {
            continue;
        }

        result.googleDeviceFound = true;

        if (descriptor.idProduct >= kAoapProductIdMin
            && descriptor.idProduct <= kAoapProductIdMax) {
            result.accessoryModeFound = true;
            result.accessoryProductId = descriptor.idProduct;
        }
    }

    libusb_free_device_list(devices, 1);
    return result;
}

/**
 * @brief バックグラウンドスレッドと共有する接続処理の状態。
 *
 * コントローラが先に破棄されてもスレッドが安全に参照できるよう共有ポインタで保持する。
 */
struct HandshakeStatus {
    std::atomic<bool> started{false};   ///< 接続処理を開始したか
    std::atomic<bool> running{false};   ///< 接続処理が実行中か
    std::atomic<bool> finished{false};  ///< 接続処理が終了したか
};
#endif  // HEADUNIT_HAS_AASDK

}  // namespace

/**
 * @brief AndroidAutoController の実装詳細。USB 関連の型をヘッダから隠蔽する。
 */
struct AndroidAutoController::Private {
#ifdef HEADUNIT_HAS_AASDK
    libusb_context *monitorContext = nullptr;   ///< 監視用の libusb コンテキスト
    std::shared_ptr<UsbInit> usbInit;           ///< tools/simple のサンプル実装
    std::shared_ptr<HandshakeStatus> handshake; ///< スレッド間で共有する接続処理の状態
    std::thread handshakeThread;                ///< 接続処理を実行するスレッド
#endif
    State state = State::Idle;  ///< 現在の接続状態
    QString detail;             ///< 状態の補足説明（表示用）
    QString handshakeText;      ///< 接続処理の実行状況（表示用）
};

/**
 * @brief コンストラクタ。監視タイマーと libusb を初期化する。
 * @param parent Qt の親オブジェクト。
 */
AndroidAutoController::AndroidAutoController(QObject *parent)
    : QObject(parent)
    , d_(new Private())
    , monitorTimer_(new QTimer(this))
{
    monitorTimer_->setInterval(kMonitorIntervalMs);
    connect(monitorTimer_, &QTimer::timeout, this, &AndroidAutoController::onMonitorTimeout);

#ifdef HEADUNIT_HAS_AASDK
    libusb_context *context = nullptr;

    // libusb を初期化できた場合のみ USB バスの監視を行う。
    if (libusb_init(&context) != 0) {
        setState(BackendUnavailable,
                 QStringLiteral("libusb の初期化に失敗しました。USB デバイスを監視できません。"));
        return;
    }

    d_->monitorContext = context;
    d_->handshake = std::make_shared<HandshakeStatus>();
    setState(Idle, QStringLiteral("start() の呼び出しを待っています。"));
#else
    setState(BackendUnavailable,
             QStringLiteral("aasdk を組み込まずにビルドされています（-DHEADUNIT_WITH_AASDK=OFF）。"
                            "Android Auto の接続処理は無効です。"));
#endif
}

/**
 * @brief デストラクタ。監視タイマーと libusb コンテキストを解放する。
 */
AndroidAutoController::~AndroidAutoController()
{
    monitorTimer_->stop();

#ifdef HEADUNIT_HAS_AASDK
    // サンプル実装の接続処理は io_service 上で待ち続ける可能性があるため、
    // 終了を待たずに切り離す（試作品ではプロセス終了と同時に終了する前提）。
    if (d_->handshakeThread.joinable()) {
        d_->handshakeThread.detach();
    }

    if (d_->monitorContext != nullptr) {
        libusb_exit(d_->monitorContext);
        d_->monitorContext = nullptr;
    }
#endif
}

/**
 * @brief 現在の接続状態を返す。
 * @return 接続状態の列挙値。
 */
AndroidAutoController::State AndroidAutoController::state() const
{
    return d_->state;
}

/**
 * @brief 接続状態を表す機械可読な文字列を返す。
 * @return 状態に対応する文字列（QML 側の条件分岐に使用）。
 */
QString AndroidAutoController::stateName() const
{
    switch (d_->state) {
    case BackendUnavailable:
        return QStringLiteral("backendUnavailable");
    case Idle:
        return QStringLiteral("idle");
    case Searching:
        return QStringLiteral("searching");
    case DeviceFound:
        return QStringLiteral("deviceFound");
    case AccessoryMode:
        return QStringLiteral("accessoryMode");
    case Error:
        return QStringLiteral("error");
    }
    return QStringLiteral("unknown");
}

/**
 * @brief 接続状態の表示用文字列を返す。
 * @return 画面表示用の状態文字列。
 */
QString AndroidAutoController::stateText() const
{
    switch (d_->state) {
    case BackendUnavailable:
        return QStringLiteral("バックエンド未使用");
    case Idle:
        return QStringLiteral("待機中");
    case Searching:
        return QStringLiteral("デバイス待機中");
    case DeviceFound:
        return QStringLiteral("接続準備中");
    case AccessoryMode:
        return QStringLiteral("接続（アクセサリモード）");
    case Error:
        return QStringLiteral("USB エラー");
    }
    return QStringLiteral("不明");
}

/**
 * @brief 接続状態の補足説明を返す。
 * @return 画面表示用の説明文字列。
 */
QString AndroidAutoController::detailText() const
{
    return d_->detail;
}

/**
 * @brief AOAP 接続処理の進行状況を返す。
 * @return 進行状況の表示用文字列。
 */
QString AndroidAutoController::handshakeText() const
{
#ifdef HEADUNIT_HAS_AASDK
    // まだ 1 度も走査していない場合は「未実行」として扱う。
    if (d_->handshakeText.isEmpty()) {
        return QStringLiteral("未実行");
    }
    return d_->handshakeText;
#else
    return QStringLiteral("未実行（aasdk 未組み込み）");
#endif
}

/**
 * @brief 利用しているバックエンドの説明を返す。
 * @return バックエンドの説明文字列。
 */
QString AndroidAutoController::backendName() const
{
#ifdef HEADUNIT_HAS_AASDK
    return QStringLiteral("aasdk（tools/simple の AOAP 接続処理）");
#else
    return QStringLiteral("なし（UI のみのビルド）");
#endif
}

/**
 * @brief 映像表示に対応しているかどうかを返す。
 * @return tools/simple に映像処理が無いため常に false。
 */
bool AndroidAutoController::videoSupported() const
{
    return false;
}

/**
 * @brief 接続状態の監視を開始し、必要に応じて接続処理を開始する。
 *
 * USB の走査はタイマー、接続処理は別スレッドで実行するため UI をブロックしない。
 */
void AndroidAutoController::start()
{
#ifdef HEADUNIT_HAS_AASDK
    // libusb が初期化されていない場合は監視できない。
    if (d_->monitorContext == nullptr) {
        setState(BackendUnavailable,
                 QStringLiteral("libusb が初期化されていないため監視を開始できません。"));
        return;
    }

    if (!monitorTimer_->isActive()) {
        monitorTimer_->start();
    }

    // 起動直後に 1 度走査して、接続状態をすぐ画面へ反映する。
    onMonitorTimeout();
#else
    setState(BackendUnavailable,
             QStringLiteral("aasdk を組み込まずにビルドされているため接続処理を開始できません。"));
#endif
}

/**
 * @brief 接続状態の監視を停止する。
 *
 * バックグラウンドで実行中の接続処理は停止できないため、実行中であれば継続する。
 */
void AndroidAutoController::stop()
{
    monitorTimer_->stop();
    setState(Idle,
             QStringLiteral("監視を停止しました。バックグラウンドの接続処理は継続している場合があります。"));
}

/**
 * @brief 監視タイマーのコールバック。USB バスを走査して状態を更新する。
 */
void AndroidAutoController::onMonitorTimeout()
{
#ifdef HEADUNIT_HAS_AASDK
    publishHandshakeStatus();

    if (d_->monitorContext == nullptr) {
        return;
    }

    const UsbScanResult result = scanUsbDevices(d_->monitorContext);

    // デバイス一覧を取得できなかった場合はエラー状態にする。
    if (result.errorCode != 0) {
        setState(Error,
                 QStringLiteral("USB デバイス一覧の取得に失敗しました（libusb エラー %1）。")
                     .arg(result.errorCode));
        return;
    }

    // アクセサリモードのデバイスを検出した場合は、サンプル実装で AOAP ハンドシェイクを開始する。
    if (result.accessoryModeFound) {
        startHandshake();
        setState(AccessoryMode,
                 QStringLiteral("AOAP アクセサリモードのデバイスを検出しました（PID 0x%1）。"
                                "tools/simple の接続処理を実行しています。")
                     .arg(result.accessoryProductId, 4, 16, QLatin1Char('0')).toUpper());
        return;
    }

    // 通常モードの Google 製デバイスを検出した場合は、サンプル実装がアクセサリモードへの切り替えを試行する。
    if (result.googleDeviceFound) {
        startHandshake();
        setState(DeviceFound,
                 QStringLiteral("Google 製 USB デバイス（VID 0x18D1）を検出しました。"
                                "tools/simple の接続処理がアクセサリモードへの切り替えを試行します。"));
        return;
    }

    setState(Searching,
             QStringLiteral("Android デバイス（VID 0x18D1）の接続を待機しています。"));
#endif
}

/**
 * @brief 接続処理（AOAP ハンドシェイク）を別スレッドで開始する。
 *
 * tools/simple の UsbInit::initDevice() をそのまま呼び出す。すでに開始済みの場合は
 * 何もしない（サンプル実装は 1 回の呼び出しで再列挙待ちまで行うため）。
 */
void AndroidAutoController::startHandshake()
{
#ifdef HEADUNIT_HAS_AASDK
    if (d_->handshake == nullptr) {
        return;
    }

    // 二重起動を防ぐ（exchange は最初の 1 回だけ false を返す）。
    if (d_->handshake->started.exchange(true)) {
        return;
    }

    // サンプル実装のインスタンスを生成し、スレッドと共有する。
    if (!d_->usbInit) {
        d_->usbInit = std::make_shared<UsbInit>();
    }

    const std::shared_ptr<UsbInit> usbInit = d_->usbInit;
    const std::shared_ptr<HandshakeStatus> status = d_->handshake;

    d_->handshake->running.store(true);

    // 接続処理は USB 制御転送を含むため、UI スレッドではなく別スレッドで実行する。
    d_->handshakeThread = std::thread([usbInit, status]() {
        usbInit->initDevice();
        status->running.store(false);
        status->finished.store(true);
    });
#endif
}

/**
 * @brief 接続処理の進行状況を UI 用プロパティへ反映する。
 */
void AndroidAutoController::publishHandshakeStatus()
{
#ifdef HEADUNIT_HAS_AASDK
    if (d_->handshake == nullptr) {
        return;
    }

    QString text;

    // 接続処理の状態を表示用の文字列に変換する。
    if (!d_->handshake->started.load()) {
        text = QStringLiteral("未実行");
    } else if (d_->handshake->running.load()) {
        text = QStringLiteral("実行中（tools/simple の処理）");
    } else {
        text = QStringLiteral("終了（詳細は起動時の標準出力ログを参照）");
    }

    if (text != d_->handshakeText) {
        d_->handshakeText = text;
        emit handshakeChanged();
    }
#endif
}

/**
 * @brief 状態と補足説明を更新し、変化があればシグナルを発行する。
 * @param state 新しい接続状態。
 * @param detail 補足説明（表示用）。
 */
void AndroidAutoController::setState(State state, const QString &detail)
{
    bool changed = false;

    if (d_->state != state) {
        d_->state = state;
        changed = true;
    }

    if (d_->detail != detail) {
        d_->detail = detail;
        changed = true;
    }

    // state / stateName / stateText / detailText は同じ通知シグナルを共有している。
    if (changed) {
        emit stateChanged();
    }
}
