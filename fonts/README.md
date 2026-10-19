# 同梱フォント（IBM Plex Sans JP）

このディレクトリには、アプリに同梱する IBM Plex Sans JP のフォントファイルを配置する。
`resources.qrc` に `fonts/...` として登録され、`src/main.cpp` の `loadBundledFonts()` が
Qt リソース（`:/fonts/...`）から読み込んでアプリ全体の既定フォントに設定する。

## 必要なファイル

以下の **7 ファイルをこのディレクトリ直下に** 配置すること（リポジトリには含まれない）。

| ファイル名 | ウェイト |
| --- | --- |
| `IBMPlexSansJP-Thin.ttf` | Thin |
| `IBMPlexSansJP-ExtraLight.ttf` | ExtraLight |
| `IBMPlexSansJP-Light.ttf` | Light |
| `IBMPlexSansJP-Regular.ttf` | Regular |
| `IBMPlexSansJP-Medium.ttf` | Medium |
| `IBMPlexSansJP-SemiBold.ttf` | SemiBold |
| `IBMPlexSansJP-Bold.ttf` | Bold |

## 注意

- 強調表示には Bold を使う（QML 側は `font.bold: true`）。Bold ファイルを同梱しても
  読み込んでおくことで、合成太字ではなく本来の Bold が使われる。
- 拡張子が `.ttf` の配布物を使う場合は、ファイル名を上記に揃えるか `resources.qrc` の
  記述を実際のファイル名・拡張子へ合わせること。
- フォントファイルが無い状態でビルドすると、Qt のリソースコンパイラ（rcc）が
  「Cannot find file」で失敗する。先にファイルを配置してからビルドすること。
