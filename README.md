Atelier; Window Manager  
Rorolina; Application Launcher

学校の課題研究にて制作した、Linux/X11向けウィンドウマネージャとアプリケーションランチャです。


## 動作するっぽい環境
* Arch Linux (2015-02-13 Linux-3.18.5)
* Fedora 21

## 必要なパッケージ
* GCC 4.9.2
* CMake 3.1.2
* libX11 1.6.2
* Jansson 2.7
* GTK+2 2.24.25

ここに掲載したバージョンは開発環境に使用していたArch Linuxにセットアップされている  
ソフトウェア及びライブラリのバージョンです。Fedoraでのバージョンは確認していません。  
きっと真新しいAPIは使用していないので、ある程度古くてもコンパイルは可能かと思われます。

## インストール
```
$ cmake . && make
# make install
```

## ディレクトリ構成
```
atelier-1.0
`- atelier  <- ウィンドウマネージャのプログラムとリソース
  `- atelierrc.default <- 設定ファイルの雛形 $HOME/.atelierrcにコピーして使用
`- rorolina <- アプリケーションランチャのプログラム
```

## ライセンス
ライセンスは、MIT Licenseが適用されます。LICENSEファイルを参照してください。

## 連絡先
Shibafu <shibafu528@gmail.com>  
Twitter @shibafu528
