# cfs_frontend

CoeFont STUDIO 用の非公式な Windows 用 GUI フロントエンドです。

音声を個別に保存するボタンを押したときに「名前を付けて保存」のダイアログが表示され、
音声ファイルと同じ名前でテキストファイルを同時に保存できるようになります。

それ以外の変更点はありません。

動作には WebView2 が必要です。  
ソースコードは Visual Studio 2019 でコンパイルできます。

## 注意事項

cfs_frontend は無保証で提供されます。  
cfs_frontend を使用したこと及び使用しなかったことによるいかなる損害について、開発者は一切の責任を負いません。

これに同意できない場合、あなたは cfs_frontend を使用することができません。  
作成した音声ファイルなどは CoeFont STUDIO の利用規約を厳守の上でご利用ください。

## FAQ

### Q. 動作がおかしい

まずウィルス対策ソフトやファイアーウォールなどにブロックされていないか確認してください。

このツールはファイルをダウンロードするとき以外、ほぼ普通のブラウザとして動きます。  
もし何かおかしい動作が起きた場合、それがファイルのダウンロードに関連することならツールの問題、
そうでないなら CoeFont STUDIO 側の問題の可能性が高いです。

### Q. 「cfs_frontend.exe.WebView2」という変なフォルダーが作られる

cfs_frontend.exe を起動すると必ず作られるフォルダーで、ブラウザーが保持するデータが入っています。  
削除しても特に問題はありませんが、ログイン状態などが解除されるでしょう。  
変なフォルダーではありません。

## 更新履歴

### v0.4

- デフォルトの文字エンコーディングを UTF-8 から UTF-8(BOM) に変更した
- アプリケーションにアイコンを設定した

### v0.3

- テキストの文字エンコーディングを保存ダイアログで選べるようにした

### v0.2

- 細かい修正など

### v0.1

- 初版

## Credits

cfs_frontend is made possible by the following open source softwares.

### Microsoft.Web.WebView2

https://www.nuget.org/packages/Microsoft.Web.WebView2

Copyright (C) Microsoft Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
   * The name of Microsoft Corporation, or the names of its contributors 
may not be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### PicoJSON

https://github.com/kazuho/picojson

Copyright 2009-2010 Cybozu Labs, Inc.
Copyright 2011-2014 Kazuho Oku
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
