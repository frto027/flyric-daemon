import QtQuick 2.8
import QtQuick.Window 2.2
/* Qt.labs.platform.Menu is duplicate with Qt.Quick.Controls.Menu */
import Qt.labs.platform 1.1 as QtLab
import QtQuick.Controls 2.0

import top.frto027.flyric 1.0

Window {
    visible: true
    width: 640
    height: 480
    /* fixed size */
    maximumHeight: height
    minimumHeight: height
    maximumWidth: width
    minimumWidth: width

    title: qsTr("Flyric Qt Daemon Configure")

    FlyricConfigManager{
        id:flyricConfigManager
    }
    FlyricWindowThread{
        id:flyricWindowThread
        onWindowExit: /*window.*/Qt.quit()
    }
    QtLab.SystemTrayIcon{
        id:systemTrayIcon
        visible: false
        iconSource:"qrc:/icon.png"
        menu:QtLab.Menu{
            QtLab.Menu{
                title: "Debug"
                QtLab.MenuItem{
                    text:"pause"
                    onTriggered: flyricWindowThread.pause_now()
                }
                QtLab.MenuItem{
                    text:"play"
                    onTriggered: flyricWindowThread.play_continue()
                }
                QtLab.MenuItem{
                    text:"enable sync"
                    onTriggered: flyricWindowThread.switch_sync(true)
                }
                QtLab.MenuItem{
                    text:"disable sync"
                    onTriggered: flyricWindowThread.switch_sync(false)
                }
            }

            QtLab.MenuItem{
                text: qsTr("Always on top")
                checked: flyricWindowThread.isTop
                onTriggered: {
                    flyricWindowThread.isTop = ! flyricWindowThread.isTop
                }
            }

            QtLab.MenuItem{
                text: qsTr("Resize")
                checked:flyricWindowThread.isResizeable
                onTriggered: {
                    flyricWindowThread.isResizeable = ! flyricWindowThread.isResizeable
                }
            }

            QtLab.MenuItem{
                text: qsTr("Background transparent(may need restart)")
                checked: flyricWindowThread.isBackgroundTransparent
                onTriggered: {
                    flyricWindowThread.isBackgroundTransparent = ! flyricWindowThread.isBackgroundTransparent
                }
            }

            QtLab.MenuItem{
                text: qsTr("Exit")
                onTriggered: {
                    close()
                    flyricWindowThread.exitWindow()
                }
            }
        }
    }
    /* default font */
    QtLab.FileDialog{
        id:defaultFontDialog
        currentFile: defaultFontTextField.text
        onAccepted: {
            const pre = "file:///"
            var f = file.toString()
            // if(f.startsWith(pre))
            if(f.substring(0,pre.length) === pre){
                f = f.substring(pre.length)
            }
            defaultFontTextField.text = f
        }
    }

    Text {
        x: 20
        y: 94
        text: qsTr("Default font file:")
        font.pixelSize: 12
    }


    TextField {
        id: defaultFontTextField
        x: 15
        y: 112
        width: 345
        height: 40
        text: flyricConfigManager.getDefaultFontPath()
    }

    Button {
        x: 377
        y: 112
        text: qsTr("Browse")
        onClicked: defaultFontDialog.open()
    }
    /* font folder */
    Text {
        x: 20
        y: 158
        text: qsTr("Font folders:")
        font.pixelSize: 12
    }

    Rectangle{
        x: 15
        y: 176
        width: 345
        height: 145
        clip: false
        border.color: "#f66e6e"

        ListView{
            id:fontFolderListView
            anchors.rightMargin: 6
            anchors.topMargin: 6
            anchors.leftMargin: 6
            anchors.bottomMargin: 6
            clip: true
            anchors.fill: parent

            model: ListModel{
                id:fontFoldersLM
                /*
                ListElement{
                    path:"aaa"
                }
                ListElement{
                    path:"bbb"
                }
                */
                //load default font folders
                Component.onCompleted: {
                    var x = flyricConfigManager.getDefaultFontFolders()
                    for(var i=0;i<x.length;i++){
                        fontFoldersLM.append({path:x[i]})
                    }
                }
            }


            delegate: SwipeDelegate {
                width: parent.width
                text: path
                swipe.right: Label{
                    text: qsTr("Remove")
                    verticalAlignment: Label.AlignVCenter
                    padding: 12
                    height: parent.height
                    anchors.right: parent.right
                    background: Rectangle {
                        color: parent.pressed ? Qt.darker("tomato", 1.1) : "tomato"
                    }
                    SwipeDelegate.onClicked: fontFoldersLM.remove(index)
                }
            }

            add: Transition {
                NumberAnimation { properties: "x"; from: -fontFolderListView.width; duration: 200 }
            }
            remove: Transition {
                ParallelAnimation {
                    NumberAnimation { properties: "x"; to: -fontFolderListView.width; duration: 200 }
                }
            }
        }

    }
    Popup {
        id: folderAldreadyExist
        anchors.centerIn: parent
        Text {
            text: qsTr("Folder already exist in the list")
        }
    }

    QtLab.FolderDialog{
        id:fontFolderDialog
        onAccepted: {
            var f = folder.toString()
            const pre = "file:///"
            if(f.substring(0,pre.length)===pre)
                f = f.substring(pre.length)
            var i;
            for(i=0;i<fontFoldersLM.count;i++){
                if(fontFoldersLM.get(i).path === f)
                    break
            }
            if(i === fontFoldersLM.count)
                fontFoldersLM.append({path:f})
            else
                folderAldreadyExist.open()
        }
    }

    Button {
        x: 377
        y: 181
        width: 100
        height: 35
        text: qsTr("Add")
        onClicked:fontFolderDialog.open()
    }

    /* start button */
    Button {
        id: button
        x: 15
        y: 422
        text: qsTr("Save and Start")
        onClicked: {
            flyricConfigManager.setDefaultFont(defaultFontTextField.text)

            var folder = []
            for(var i=0;i<fontFoldersLM.count;i++)
                folder.push(fontFoldersLM.get(i).path)
            flyricConfigManager.setFontFolder(folder)

            flyricConfigManager.setUdpPort(parseInt(portTextField.text))
            flyricConfigManager.setFrcFolder(frcFolderTextField.text)
            flyricConfigManager.setTimeOffset(offsetTextField.text)

            flyricConfigManager.save()

            /*window.*/hide()
            flyricWindowThread.createAndShow(flyricConfigManager)

            systemTrayIcon.show()

            flyricWindowThread.startUdpServer()

        }
    }

    Text {
        x: 20
        y: 341
        text: qsTr("UDP Port:")
        font.pixelSize: 12
    }

    TextField {
        id: portTextField
        x: 88
        y: 327
        width: 75
        height: 40
        inputMethodHints: Qt.ImhDigitsOnly
        validator: IntValidator {bottom: 0; top: 99999;}
        text: flyricConfigManager.getUdpPort() + ""
        onEditingFinished: {
            var x = parseInt(text)
            if(x > 65535){
                portTextField.text = "65535"
            }
        }
    }

    Text {
        x: 169
        y: 341
        text: qsTr("( 0 - 65535 )")
        font.pixelSize: 12
    }

    Text{
        x:20
        y:380
        text: qsTr("time offset:")
        font.pixelSize: 12
    }

    TextField{
        id:offsetTextField
        x:100
        y:370
        inputMethodHints: Qt.ImhDigitsOnly
        text: flyricConfigManager.getTimeOffset() + ""
    }

    TextEdit {
        id: textEdit
        x: 300
        y: 453
        width: 340
        height: 27
        text: qsTr("ConfPath:") + flyricConfigManager.confPath
        renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignRight
        cursorVisible: true
        readOnly: true
        font.pixelSize: 12
    }

    Text {
        x: 15
        y: 13
        text: qsTr("Lyric(.frc) folders:")
        font.pixelSize: 12
    }

    TextField {
        id: frcFolderTextField
        x: 15
        y: 41
        width: 345
        height: 40
        text: flyricConfigManager.getFrcFolder()
    }
    /* frc folder */
    QtLab.FolderDialog{
        id:frcFolderDialog
        currentFolder:"file:///" + frcFolderTextField.text
        onAccepted: {
            const pre = "file:///"
            var f = folder.toString()
            // if(f.startsWith(pre))
            if(f.substring(0,pre.length) === pre){
                f = f.substring(pre.length)
            }
            frcFolderTextField.text = f
        }
    }
    Button {
        x: 377
        y: 41
        text: qsTr("Browse")
        onClicked: frcFolderDialog.open()
    }




}

















































































/*##^## Designer {
    D{i:9;anchors_height:226;anchors_width:345;anchors_x:"-162";anchors_y:"-33"}
}
 ##^##*/
