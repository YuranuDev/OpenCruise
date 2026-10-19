// =============================================================================
// 設定画面（仮実装）
//   表示のみのプレースホルダであり、設定値の保存や動作への反映は未実装。
//   メイン画面（Main.qml）から Loader で読み込まれ、closed() で閉じる。
// =============================================================================
import QtQuick

Item {
    id: settingsPage

    // 設定画面を閉じる要求を親（Main.qml）へ通知する。
    signal closed()

    // 設定項目（表示のみのダミー）。
    property var settingItems: [
        { "title": qsTr("画面の明るさ"), "value": qsTr("70 %（固定）") },
        { "title": qsTr("音量"), "value": qsTr("40 %（固定）") },
        { "title": qsTr("CAN 情報"), "value": qsTr("ダミー表示") },
        { "title": qsTr("Android Auto"), "value": qsTr("tools/simple の接続処理を使用") }
    ]

    // 背景（メイン画面を覆う）。
    Rectangle {
        id: background
        anchors.fill: parent
        color: "#0f1317"

        // 背面のメイン画面へマウス操作が届かないようにする。
        MouseArea {
            anchors.fill: parent
        }

        // ヘッダ。
        Rectangle {
            id: settingsHeader
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 72
            color: "#181d23"

            Text {
                anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                text: qsTr("設定")
                color: "#e6edf3"
                font.pixelSize: 24
                font.bold: true
            }
        }

        // 設定項目の一覧（表示のみ）。
        Column {
            id: itemsColumn
            anchors { top: settingsHeader.bottom; left: parent.left; right: parent.right; margins: 24 }
            spacing: 16

            Repeater {
                model: settingsPage.settingItems

                // 設定項目 1 件分の表示。
                Rectangle {
                    width: itemsColumn.width
                    height: 64
                    radius: 10
                    color: "#181d23"

                    Text {
                        anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                        text: modelData.title
                        color: "#e6edf3"
                        font.pixelSize: 18
                    }

                    Text {
                        anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
                        text: modelData.value
                        color: "#8b949e"
                        font.pixelSize: 18
                    }
                }
            }

            Text {
                width: itemsColumn.width
                text: qsTr("この設定画面は仮実装です。値の保存や動作への反映は未実装です。")
                color: "#d29922"
                font.pixelSize: 15
                wrapMode: Text.WordWrap
            }
        }

        // メイン画面へ戻るボタン。
        Rectangle {
            id: backButton
            anchors { left: parent.left; leftMargin: 24; bottom: parent.bottom; bottomMargin: 24 }
            width: 180
            height: 60
            radius: 10
            color: backMouseArea.pressed ? "#2f6fb0" : "#274866"

            Text {
                anchors.centerIn: parent
                text: qsTr("戻る")
                color: "#e6edf3"
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                id: backMouseArea
                anchors.fill: parent
                onClicked: settingsPage.closed()
            }
        }
    }
}
