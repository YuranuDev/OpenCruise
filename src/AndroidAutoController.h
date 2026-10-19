/**
 * @file AndroidAutoController.h
 * @brief Android Auto（aasdk）の接続処理と接続状態を Qt の UI へ公開する制御クラス。
 *
 * aasdk リポジトリの tools/simple に含まれるサンプル実装を利用して USB AOAP の
 * 接続処理を実行し、その進行状況と USB バスの監視結果を QML から参照できる
 * プロパティとして提供する。USB の走査はタイマー、接続処理は別スレッドで行うため
 * メイン（UI）スレッドをブロックしない。
 *
 * 未実装: Android Auto の映像受信・デコード・画面表示。tools/simple には映像処理が
 * 存在しないため、本クラスは映像を扱わない（videoSupported は常に false）。
 */

#pragma once

#include <QObject>
#include <QString>

#include <memory>

class QTimer;

/**
 * @brief Android Auto の接続処理と接続状態を管理するクラス。
 */
class AndroidAutoController : public QObject
{
    Q_OBJECT

    /// 接続状態（列挙値）。
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    /// 接続状態を表す機械可読な文字列（QML 側の条件分岐用）。
    Q_PROPERTY(QString stateName READ stateName NOTIFY stateChanged)
    /// 接続状態の表示用文字列。
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    /// 接続状態の補足説明（表示用）。
    Q_PROPERTY(QString detailText READ detailText NOTIFY stateChanged)
    /// AOAP 接続処理（tools/simple の処理）の進行状況。
    Q_PROPERTY(QString handshakeText READ handshakeText NOTIFY handshakeChanged)
    /// 利用しているバックエンドの説明。
    Q_PROPERTY(QString backendName READ backendName CONSTANT)
    /// 映像表示に対応しているかどうか（現状は常に false）。
    Q_PROPERTY(bool videoSupported READ videoSupported CONSTANT)

public:
    /**
     * @brief 接続状態。
     */
    enum State {
        BackendUnavailable,  ///< aasdk 未組み込み、または libusb の初期化に失敗
        Idle,                ///< start() 未呼び出し、または監視停止中
        Searching,           ///< Android デバイス（VID 0x18D1）を待機中
        DeviceFound,         ///< Google 製 USB デバイスを検出（アクセサリモード切り替え中）
        AccessoryMode,       ///< AOAP アクセサリモードのデバイスを検出
        Error                ///< USB デバイスの列挙に失敗
    };
    Q_ENUM(State)

    /**
     * @brief コンストラクタ。監視タイマーと libusb の初期化を行う。
     * @param parent Qt の親オブジェクト。
     */
    explicit AndroidAutoController(QObject *parent = nullptr);

    /**
     * @brief デストラクタ。監視タイマーと libusb コンテキストを解放する。
     */
    ~AndroidAutoController() override;

    /**
     * @brief 現在の接続状態を返す。
     * @return 接続状態の列挙値。
     */
    State state() const;

    /**
     * @brief 接続状態を表す機械可読な文字列を返す。
     * @return "idle" などの文字列。
     */
    QString stateName() const;

    /**
     * @brief 接続状態の表示用文字列を返す。
     * @return 画面表示用の状態文字列。
     */
    QString stateText() const;

    /**
     * @brief 接続状態の補足説明を返す。
     * @return 画面表示用の説明文字列。
     */
    QString detailText() const;

    /**
     * @brief AOAP 接続処理の進行状況を返す。
     * @return 進行状況の表示用文字列。
     */
    QString handshakeText() const;

    /**
     * @brief 利用しているバックエンドの説明を返す。
     * @return バックエンドの説明文字列。
     */
    QString backendName() const;

    /**
     * @brief 映像表示に対応しているかどうかを返す。
     * @return tools/simple に映像処理が無いため常に false。
     */
    bool videoSupported() const;

    /**
     * @brief 接続状態の監視を開始し、必要に応じて接続処理を開始する。
     */
    Q_INVOKABLE void start();

    /**
     * @brief 接続状態の監視を停止する。
     */
    Q_INVOKABLE void stop();

signals:
    /**
     * @brief 接続状態または補足説明が変化したときに発行される。
     */
    void stateChanged();

    /**
     * @brief 接続処理の進行状況が変化したときに発行される。
     */
    void handshakeChanged();

private:
    /**
     * @brief 監視タイマーのコールバック。USB バスを走査して状態を更新する。
     */
    void onMonitorTimeout();

    /**
     * @brief 接続処理（AOAP ハンドシェイク）を別スレッドで開始する。
     */
    void startHandshake();

    /**
     * @brief 接続処理の進行状況を UI 用プロパティへ反映する。
     */
    void publishHandshakeStatus();

    /**
     * @brief 状態と補足説明を更新し、変化があればシグナルを発行する。
     * @param state 新しい接続状態。
     * @param detail 補足説明（表示用）。
     */
    void setState(State state, const QString &detail);

    /// 実装詳細（USB 関連の状態を隠蔽するための前方宣言）。
    struct Private;

    std::unique_ptr<Private> d_;  ///< 実装詳細
    QTimer *monitorTimer_;        ///< USB バスの監視タイマー
};
