/**
 * @file main.cpp
 * @brief 車載システム簡易プロトタイプのアプリケーション起動処理。
 *
 * Qt Quick のメイン画面（QML/Main.qml）を読み込み、Android Auto の接続状態を扱う
 * AndroidAutoController を QML から利用できるように登録する。aasdk を利用した接続
 * 処理は AndroidAutoController 側（別スレッド）で行うため、起動処理は UI の準備のみを
 * 担当する。
 */

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStringLiteral>
#include <QUrl>
#include <QtQml>

#include <cstdio>

#include "AndroidAutoController.h"

namespace {

/// アプリに同梱する IBM Plex Sans JP のフォントファミリ名。
constexpr char kFontFamily[] = "IBM Plex Sans JP";

/**
 * @brief 同梱する IBM Plex Sans JP のフォントファイル（Qt リソースパス）。
 *
 * 利用できるウェイトは Thin / ExtraLight / Light / Regular / Medium / SemiBold / Bold。
 * 強調表示には Bold を使用する（QML 側は font.bold: true）。
 */
const char *const kFontResources[] = {
    ":/fonts/IBMPlexSansJP-Thin.ttf",
    ":/fonts/IBMPlexSansJP-ExtraLight.ttf",
    ":/fonts/IBMPlexSansJP-Light.ttf",
    ":/fonts/IBMPlexSansJP-Regular.ttf",
    ":/fonts/IBMPlexSansJP-Medium.ttf",
    ":/fonts/IBMPlexSansJP-SemiBold.ttf",
    ":/fonts/IBMPlexSansJP-Bold.ttf",
};

/**
 * @brief 同梱した IBM Plex Sans JP のフォントファイルをアプリケーションへ登録する。
 *
 * 登録に失敗した場合は警告を標準エラーへ出力し、処理は継続する。
 */
void loadBundledFonts()
{
    for (const char *resource : kFontResources) {
        const int id = QFontDatabase::addApplicationFont(QString::fromLatin1(resource));
        if (id < 0) {
            std::fprintf(stderr, "[headunit] フォントの読み込みに失敗しました: %s\n", resource);
        }
    }
}

}  // namespace

/**
 * @brief アプリケーションを起動し、QML のメイン画面を読み込む。
 * @param argc コマンドライン引数の数。
 * @param argv コマンドライン引数。
 * @return アプリケーションの終了コード（QML の読み込みに失敗した場合は 1）。
 */
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("HeadUnitPrototype"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("HeadUnitPrototype"));

    // 同梱フォント（IBM Plex Sans JP）を登録し、アプリ全体の既定フォントに設定する。
    // QML 側は font.family を指定しなくてもこの既定フォントを継承する。
    loadBundledFonts();
    app.setFont(QFont(QString::fromLatin1(kFontFamily)));

    // QML から AndroidAutoController を生成できるように登録する。
    qmlRegisterType<AndroidAutoController>("HeadUnit", 1, 0, "AndroidAutoController");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/QML/Main.qml")));

    // QML の読み込みに失敗した場合はエラーを出して終了する。
    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "[headunit] QML の読み込みに失敗しました: qrc:/QML/Main.qml\n");
        return 1;
    }

    return app.exec();
}
