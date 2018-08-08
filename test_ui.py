# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'source/user_interface/qt/commandfiledialog.ui'
#
# Created: Tue May 15 15:25:39 2018
#      by: pyside2-uic  running on PySide2 5.11.0a1
#
# WARNING! All changes made in this file will be lost!

from PySide2 import QtCore, QtGui, QtWidgets

class Ui_CommandFileDialog(object):
    def setupUi(self, CommandFileDialog):
        CommandFileDialog.setObjectName("CommandFileDialog")
        CommandFileDialog.resize(502, 517)
        self.verticalLayout = QtWidgets.QVBoxLayout(CommandFileDialog)
        self.verticalLayout.setObjectName("verticalLayout")
        self.contents_plainTextEdit = QtWidgets.QPlainTextEdit(CommandFileDialog)
        self.contents_plainTextEdit.setObjectName("contents_plainTextEdit")
        self.verticalLayout.addWidget(self.contents_plainTextEdit)
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.all_pushButton = QtWidgets.QPushButton(CommandFileDialog)
        self.all_pushButton.setObjectName("all_pushButton")
        self.horizontalLayout.addWidget(self.all_pushButton)
        self.selected_pushButton = QtWidgets.QPushButton(CommandFileDialog)
        self.selected_pushButton.setObjectName("selected_pushButton")
        self.horizontalLayout.addWidget(self.selected_pushButton)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.cancel_pushButton = QtWidgets.QPushButton(CommandFileDialog)
        self.cancel_pushButton.setObjectName("cancel_pushButton")
        self.horizontalLayout.addWidget(self.cancel_pushButton)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi(CommandFileDialog)
        QtCore.QMetaObject.connectSlotsByName(CommandFileDialog)

    def retranslateUi(self, CommandFileDialog):
        CommandFileDialog.setWindowTitle(QtWidgets.QApplication.translate("CommandFileDialog", "Command File Viewer", None, -1))
        self.all_pushButton.setText(QtWidgets.QApplication.translate("CommandFileDialog", "All", None, -1))
        self.selected_pushButton.setText(QtWidgets.QApplication.translate("CommandFileDialog", "Selected", None, -1))
        self.cancel_pushButton.setText(QtWidgets.QApplication.translate("CommandFileDialog", "Cancel", None, -1))

