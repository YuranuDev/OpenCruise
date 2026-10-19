// =============================================================================
// メイン画面（車載向けの簡易 UI）
//   ・現在時刻の表示
//   ・Android Auto の接続状態表示と表示領域（映像表示は未実装）
//   ・CAN 情報のダミー表示（実 CAN 通信は未実装）
//   ・設定画面への遷移
// =============================================================================
import QtQuick
import QtQuick.Window
import HeadUnit 1.0

Window {
    id: root

    width: 1024
    height: 600
    visible: true
    color: "#0f1317"
    title: qsTr("Head Unit Prototype")

    // 接続状態に対応する表示色を返す。
    function statusColor() {
        switch (androidAuto.stateName) {
        case "accessoryMode":
            return "#3fb950"
        case "deviceFound":
        case "searching":
            return "#d29922"
        case "error":
            return "#f85149"
        default:
            return "#8b949e"
        }
    }

    // Android Auto の接続状態を管理する制御オブジェクト（C++ 側）。
    AndroidAutoController {
        id: androidAuto

        // 画面表示後に接続状態の監視を開始する。
        Component.onCompleted: start()
    }

    // 現在時刻（1 秒ごとに更新）。
    property string clockText: Qt.formatTime(new Date(), "HH:mm:ss")
    property string dateText: Qt.formatDate(new Date(), "yyyy/MM/dd")

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            root.clockText = Qt.formatTime(new Date(), "HH:mm:ss")
            root.dateText = Qt.formatDate(new Date(), "yyyy/MM/dd")
        }
    }

    // CAN 情報（ダミーデータ。実 CAN 通信は未実装）。
    property int vehicleSpeed: 0
    property int engineRpm: 0
    property int coolantTemp: 0
    property var canItems: [
        { "label": qsTr("車速"), "value": root.vehicleSpeed + " km/h" },
        { "label": qsTr("エンジン回転数"), "value": root.engineRpm + " rpm" },
        { "label": qsTr("水温"), "value": root.coolantTemp + " °C" }
    ]

    Timer {
        id: canTimer
        interval: 500
        running: true
        repeat: true

        // 擬似的にゆっくり変化させるためのカウンタ。
        property real tick: 0

        onTriggered: {
            tick = tick + 1
            root.vehicleSpeed = Math.round(50 + 20 * Math.sin(tick / 9))
            root.engineRpm = Math.round(2000 + 800 * Math.sin(tick / 6))
            root.coolantTemp = Math.round(87 + 4 * Math.sin(tick / 13))
        }
    }

    // --- ヘッダ（接続状態 / タイトル / 現在時刻） ---------------------------
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 72
        color: "#181d23"

        // 接続状態の表示。
        Rectangle {
            id: statusChip
            anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
            width: statusLabel.implicitWidth + 40
            height: 40
            radius: 20
            color: root.statusColor()

            Text {
                id: statusLabel
                anchors.centerIn: parent
                text: androidAuto.stateText
                color: "#0f1317"
                font.pixelSize: 18
                font.bold: true
            }
        }

        // 画面タイトル。
        Text {
            anchors.centerIn: parent
            text: qsTr("Head Unit Prototype")
            color: "#e6edf3"
            font.pixelSize: 24
            font.bold: true
        }

        // 現在時刻と日付。
        Column {
            anchors { right: parent.right; rightMargin: 24; top: parent.top; topMargin: 8 }
            width: 240
            spacing: 2

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignRight
                text: root.clockText
                color: "#e6edf3"
                font.pixelSize: 26
                font.bold: true
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignRight
                text: root.dateText
                color: "#8b949e"
                font.pixelSize: 14
            }
        }
    }

    // --- Android Auto 表示領域（映像表示は未実装） -------------------------
    Rectangle {
        id: androidAutoArea
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: footer.top
            margins: 16
        }
        color: "#05070a"
        radius: 12
        border.width: 1
        border.color: "#232a33"

        Column {
            anchors.centerIn: parent
            width: parent.width - 64
            spacing: 14

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Android Auto 表示領域")
                color: "#e6edf3"
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("接続状態: ") + androidAuto.stateText
                color: "#8b949e"
                font.pixelSize: 18
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: androidAuto.detailText
                color: "#8b949e"
                font.pixelSize: 15
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("映像の受信・デコード・表示は未実装です（tools/simple に映像処理が存在しないため）。")
                color: "#d29922"
                font.pixelSize: 15
                wrapMode: Text.WordWrap
            }
        }
    }

    // --- フッタ（ダミー CAN 情報 / 設定画面への遷移） -----------------------
    Rectangle {
        id: footer
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 112
        color: "#181d23"

        // CAN 情報のダミー表示。
        Row {
            id: canRow
            anchors { left: parent.left; leftMargin: 24; top: parent.top; topMargin: 16 }
            spacing: 32

            Repeater {
                model: root.canItems

                // CAN 情報 1 項目分の表示。
                Column {
                    width: 150
                    spacing: 4

                    Text {
                        width: parent.width
                        text: modelData.label
                        color: "#8b949e"
                        font.pixelSize: 14
                    }

                    Text {
                        width: parent.width
                        text: modelData.value
                        color: "#e6edf3"
                        font.pixelSize: 24
                        font.bold: true
                    }
                }
            }
        }

        // 使用中のバックエンドと接続処理の状況。
        Text {
            anchors { left: parent.left; leftMargin: 24; bottom: parent.bottom; bottomMargin: 12 }
            text: qsTr("バックエンド: ") + androidAuto.backendName
                  + qsTr(" / 接続処理: ") + androidAuto.handshakeText
            color: "#8b949e"
            font.pixelSize: 13
        }

        // 設定画面へ遷移するボタン。
        Rectangle {
            id: settingsButton
            anchors { right: parent.right; rightMargin: 24; top: parent.top; topMargin: 26 }
            width: 180
            height: 60
            radius: 10
            color: settingsMouseArea.pressed ? "#2f6fb0" : "#274866"

            Text {
                anchors.centerIn: parent
                text: qsTr("設定")
                color: "#e6edf3"
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                id: settingsMouseArea
                anchors.fill: parent
                onClicked: settingsLoader.source = "Settings.qml"
            }
        }
    }

    // --- 設定画面（Loader による簡易な画面遷移） ---------------------------
    Loader {
        id: settingsLoader
        anchors.fill: parent

        // 設定画面の「戻る」が押されたら閉じる。
        onLoaded: item.closed.connect(function() { settingsLoader.source = "" })
    }
}
