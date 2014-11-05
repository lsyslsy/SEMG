#ifndef SEMG_DEMO_H
#define SEMG_DEMO_H

#include <QtGui/QMainWindow>
#include "ui_semg_demo.h"
#include <windows.h>
#include "../sEMG_DLL/socket.h"
#include "../sEMG_DLL/sEMG_DLL.h"

#include <QtOpenGL>
#include <QTimer>
//#include <QDialog>
#include <QString>
#include <QListWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollArea>
//#include <QHBoxLayout>
//#include <math.h>
#include <QFile>
#include <QTextStream>
#include "GLWidget.h"
#include "ChartWidget.h"
#include "DataChannel.h"
#include "DataSave.h"
#include "Data.h"
#include <vector>
#include <map>

#define DEMO_DEBUG

class ChartWidget;
class QScrollArea;
class DataChannel;
class DataSave;
class Data;
class GLWidget;
class QLabel;
class QFile;

class sEMG_DEMO : public QMainWindow
{
	Q_OBJECT

public:
	sEMG_DEMO(QWidget *parent = 0, Qt::WFlags flags = 0);
	~sEMG_DEMO();

private:
	Ui::sEMG_DEMOClass ui; 
	GLWidget* triangle;
	QScrollArea* m_pScrollArea;
	QScrollArea* m_pScrollArea1;
    ChartWidget* m_pChartWidget;
    DataChannel* m_pDataChannel;
	DataSave*    m_pDataSave;
    QTimer* m_pTimer;
    //unsigned int Current_TimeStamp;
    //unsigned int Sys_TimeStamp;
	unsigned char m_nTakenTime;
	bool m_bStore;
	//QString m_SaveDataPath;
	QTimer* m_pSaveDataTimer;
	QFile *SaveDataFile;

	QTimer* m_pConnectTimer;

	

	void UpdateDatas();
    void UpdateDataView();
	void UpdateValueListWidget(int ChannelId);
	void AddMessage(const QString& msg);
	void resetChartWidgetSize();
	void LoadChannelListWidget();
    void LoadDataSave();

    unsigned char channel_num;
	unsigned char AD_rate;

private slots:
    void  DrawTimerEvent();
	void  ActionOpenDevice();
	void  GetDataEnable();
    void  GetDataDisEnable();

	void  StartSaveData();
	void  SaveDataTimerEvent();
    void  StopSaveData();

	void  ConnectTimerEvent();
protected:
	void showEvent(QShowEvent* event);
	void closeEvent(QCloseEvent* event);
	void resizeEvent(QResizeEvent* event);
};

#endif // SEMG_DEMO_H
