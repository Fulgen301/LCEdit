#!/bin/bash
#Copyright (c) 2019, George Tokmaji
#Permission to use, copy, modify, and/or distribute this software for any
#purpose with or without fee is hereby granted, provided that the above
#copyright notice and this permission notice appear in all copies.
#
#THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

DESTDIR=appdir cmake --build . --target install

if [ "$TRAVIS_OS_NAME" = "linux" ]
then
	linuxdeployqt=linuxdeployqt-continuous-x86_64.AppImage
	if [[ ! -x $linuxdeployqt ]]; then
		wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/$linuxdeployqt" || exit 1
		chmod a+x $linuxdeployqt
	fi
	unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
	./$linuxdeployqt appdir/usr/share/applications/lcedit.desktop -appimage -no-strip -qmake=/opt/qt511/bin/qmake
	wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
	bash ./upload.sh LCEdit*.AppImage*
fi
