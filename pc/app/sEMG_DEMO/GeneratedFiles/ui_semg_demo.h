/********************************************************************************
** Form generated from reading UI file 'semg_demo.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SEMG_DEMO_H
#define UI_SEMG_DEMO_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_sEMG_DEMOClass
{
public:
    QAction *actionOpen_device;
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout;
    QListWidget *Device_listWidget;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *GetData;
    QPushButton *Pause;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *Save;
    QPushButton *SaveStop;
    QHBoxLayout *horizontalLayout_4;
    QLabel *lable1;
    QLabel *Label_Save_Time;
    QListWidget *Value_listWidget;
    QListWidget *Channel_listWidget;
    QVBoxLayout *verticalLayout_2;
    QTabWidget *tabWidget;
    QWidget *tab;
    QWidget *tab_3;
    QWidget *tab_2;
    QMenuBar *menuBar;
    QMenu *menuMenu;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *sEMG_DEMOClass)
    {
        if (sEMG_DEMOClass->objectName().isEmpty())
            sEMG_DEMOClass->setObjectName(QString::fromUtf8("sEMG_DEMOClass"));
        sEMG_DEMOClass->resize(747, 634);
        actionOpen_device = new QAction(sEMG_DEMOClass);
        actionOpen_device->setObjectName(QString::fromUtf8("actionOpen_device"));
        centralWidget = new QWidget(sEMG_DEMOClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        Device_listWidget = new QListWidget(centralWidget);
        Device_listWidget->setObjectName(QString::fromUtf8("Device_listWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Device_listWidget->sizePolicy().hasHeightForWidth());
        Device_listWidget->setSizePolicy(sizePolicy);
        Device_listWidget->setMinimumSize(QSize(200, 0));
        Device_listWidget->setMaximumSize(QSize(200, 200));

        verticalLayout->addWidget(Device_listWidget);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        GetData = new QPushButton(centralWidget);
        GetData->setObjectName(QString::fromUtf8("GetData"));
        GetData->setMaximumSize(QSize(200, 16777215));

        horizontalLayout_2->addWidget(GetData);

        Pause = new QPushButton(centralWidget);
        Pause->setObjectName(QString::fromUtf8("Pause"));
        Pause->setMaximumSize(QSize(200, 16777215));

        horizontalLayout_2->addWidget(Pause);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        Save = new QPushButton(centralWidget);
        Save->setObjectName(QString::fromUtf8("Save"));
        Save->setMaximumSize(QSize(200, 16777215));

        horizontalLayout_3->addWidget(Save);

        SaveStop = new QPushButton(centralWidget);
        SaveStop->setObjectName(QString::fromUtf8("SaveStop"));
        SaveStop->setMaximumSize(QSize(200, 16777215));

        horizontalLayout_3->addWidget(SaveStop);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        lable1 = new QLabel(centralWidget);
        lable1->setObjectName(QString::fromUtf8("lable1"));
        lable1->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_4->addWidget(lable1);

        Label_Save_Time = new QLabel(centralWidget);
        Label_Save_Time->setObjectName(QString::fromUtf8("Label_Save_Time"));
        Label_Save_Time->setMaximumSize(QSize(200, 16777215));

        horizontalLayout_4->addWidget(Label_Save_Time);


        verticalLayout->addLayout(horizontalLayout_4);

        Value_listWidget = new QListWidget(centralWidget);
        Value_listWidget->setObjectName(QString::fromUtf8("Value_listWidget"));
        Value_listWidget->setMaximumSize(QSize(200, 50));

        verticalLayout->addWidget(Value_listWidget);

        Channel_listWidget = new QListWidget(centralWidget);
        Channel_listWidget->setObjectName(QString::fromUtf8("Channel_listWidget"));
        Channel_listWidget->setMaximumSize(QSize(200, 16777215));

        verticalLayout->addWidget(Channel_listWidget);


        horizontalLayout->addLayout(verticalLayout);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        tabWidget->addTab(tab, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        tabWidget->addTab(tab_3, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        tabWidget->addTab(tab_2, QString());

        verticalLayout_2->addWidget(tabWidget);


        horizontalLayout->addLayout(verticalLayout_2);

        sEMG_DEMOClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(sEMG_DEMOClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 747, 23));
        menuMenu = new QMenu(menuBar);
        menuMenu->setObjectName(QString::fromUtf8("menuMenu"));
        sEMG_DEMOClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(sEMG_DEMOClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        sEMG_DEMOClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(sEMG_DEMOClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        sEMG_DEMOClass->setStatusBar(statusBar);

        menuBar->addAction(menuMenu->menuAction());
        menuMenu->addAction(actionOpen_device);

        retranslateUi(sEMG_DEMOClass);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(sEMG_DEMOClass);
    } // setupUi

    void retranslateUi(QMainWindow *sEMG_DEMOClass)
    {
        sEMG_DEMOClass->setWindowTitle(QApplication::translate("sEMG_DEMOClass", "sEMG_DEMO", 0, QApplication::UnicodeUTF8));
        actionOpen_device->setText(QApplication::translate("sEMG_DEMOClass", "open device", 0, QApplication::UnicodeUTF8));
        GetData->setText(QApplication::translate("sEMG_DEMOClass", "GetData", 0, QApplication::UnicodeUTF8));
        Pause->setText(QApplication::translate("sEMG_DEMOClass", "Pause", 0, QApplication::UnicodeUTF8));
        Save->setText(QApplication::translate("sEMG_DEMOClass", "Save", 0, QApplication::UnicodeUTF8));
        SaveStop->setText(QApplication::translate("sEMG_DEMOClass", "Save_stop", 0, QApplication::UnicodeUTF8));
        lable1->setText(QApplication::translate("sEMG_DEMOClass", "\345\255\230\345\202\250\346\214\201\347\273\255\346\227\266\351\227\264\357\274\232", 0, QApplication::UnicodeUTF8));
        Label_Save_Time->setText(QApplication::translate("sEMG_DEMOClass", "0.0 s", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        tabWidget->setToolTip(QString());
#endif // QT_NO_TOOLTIP
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("sEMG_DEMOClass", "paint", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QApplication::translate("sEMG_DEMOClass", "Page", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("sEMG_DEMOClass", "3D", 0, QApplication::UnicodeUTF8));
        menuMenu->setTitle(QApplication::translate("sEMG_DEMOClass", "menu", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class sEMG_DEMOClass: public Ui_sEMG_DEMOClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SEMG_DEMO_H
