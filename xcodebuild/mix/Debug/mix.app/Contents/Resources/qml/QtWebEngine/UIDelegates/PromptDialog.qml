/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// FIXME: prompt missing in Qt Quick Dialogs atm. Make our own for now.
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import QtQuick 2.0

ApplicationWindow {
    signal input(string text);
    signal accepted;
    signal rejected;
    property alias text: message.text;
    property alias prompt: field.text;

    width: 350
    height: 100
    flags: Qt.Dialog

    function open() {
        show();
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 4;
        Text {
            id: message;
            Layout.fillWidth: true;
        }
        TextField {
            id:field;
            Layout.fillWidth: true;
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8;
            Button {
                text: "OK"
                onClicked: {
                    input(field.text)
                    accepted();
                    close();
                    destroy();
                }
            }
            Button {
                text: "Cancel"
                onClicked: {
                    rejected();
                    close();
                    destroy();
                }
            }
        }
    }

}
