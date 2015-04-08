#include "semg_demo.h"
//using namespace System;

sEMG_DEMO::sEMG_DEMO(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);//安装界面UI
    m_pTimer = new QTimer;
	m_pConnectTimer = NULL;
	triangle = new GLWidget;
    QLayout* pLayout1 = new QVBoxLayout();
    ui.tab_2->setLayout(pLayout1);
  //  pLayout1->addWidget(triangle);
	//Paint 图表页面
	m_pScrollArea = new QScrollArea();
	m_pChartWidget = new ChartWidget();
    QLayout* pLayout = new QVBoxLayout();
	ui.tab->setLayout(pLayout);
	pLayout->addWidget(m_pScrollArea);
	m_pScrollArea->setWidget(m_pChartWidget);

   // m_pChartWidget->setFixedSize(800, 400);
	m_pDataChannel = new DataChannel;
    m_pDataSave = new DataSave;
	//界面按钮初始化
    ui.GetData->setEnabled(false);
	ui.Pause->setEnabled(false);
	ui.Save->setEnabled(false);
	ui.SaveStop->setEnabled(false);
	m_bStore = false;
	 

    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(DrawTimerEvent()));
	connect(ui.actionOpen_device,SIGNAL(triggered()),this,SLOT(ActionOpenDevice()));
	connect(ui.GetData,SIGNAL(clicked()),this, SLOT(GetDataEnable()));
	connect(ui.Pause, SIGNAL(clicked()), this, SLOT(GetDataDisEnable()));
	connect(ui.Save, SIGNAL(clicked()), this, SLOT(StartSaveData()));
	connect(ui.SaveStop, SIGNAL(clicked()), this, SLOT(StopSaveData()));

}

sEMG_DEMO::~sEMG_DEMO()
{
	delete m_pTimer;
	delete m_pConnectTimer;//delete NULL 是安全的
}

void sEMG_DEMO::ActionOpenDevice()
{
	char dll_info[400];
	ui.actionOpen_device->setEnabled(false);
	BOOL ret = sEMG_open(TRUE);
	get_dll_info(dll_info);
	QListWidgetItem* pItem_flag = new QListWidgetItem;
		pItem_flag->setText( QString("Device:") );
		ui.Device_listWidget->addItem(pItem_flag);
	QListWidgetItem* pItem_num = new QListWidgetItem;
		pItem_num->setText( QString("Channel Num:"));
	    ui.Device_listWidget->addItem(pItem_num);
    QListWidgetItem* pItem_rate = new QListWidgetItem;
        pItem_rate->setText( QString("Sample Rate:"));
        ui.Device_listWidget->addItem(pItem_rate);
    QListWidgetItem* pItem_info = new QListWidgetItem;
	    pItem_info->setText( QString("%1").arg(dll_info));
        ui.Device_listWidget->addItem(pItem_info);
    
	if(!ret){//Failed
       QMessageBox::information(this, tr("ERROR"), QString("Device Undetected!!"));
	   pItem_flag->setText( QString("Device: Waiting for Reconnecting..."));
	   pItem_num->setText(  QString("Channel Num: 0"));
	   pItem_rate->setText( QString("Sample Rate: 0 k"));
	   m_pConnectTimer = new QTimer; 
	   m_pConnectTimer -> start(3000);//等3秒让连接成功
	   connect(m_pConnectTimer, SIGNAL(timeout()), this, SLOT(ConnectTimerEvent()));
	 }
	else{//Success
      channel_num = get_channel_num();
      AD_rate = get_AD_rate();
	  get_dll_info(dll_info);
	  pItem_flag->setText( QString("Device Open: Connect Succeed!"));
	  pItem_num->setText(  QString("Channel Num: %1").arg(channel_num));
	  pItem_rate->setText( QString("Sample Rate: %1 k").arg(AD_rate));
	  ui.GetData->setEnabled(true);
      ui.Save->setEnabled(true);
	  LoadChannelListWidget();
      LoadDataSave();
	}
	//
   //set_data_notify(NotifyTest);
}

void sEMG_DEMO::ConnectTimerEvent()
{
	if (get_dev_stat() == dev_CONNECT){
	  channel_num = get_channel_num();
      AD_rate = get_AD_rate();
	  ui.Device_listWidget->item(0)->setText( QString("Device Open: Connect Succeed!"));
	  ui.Device_listWidget->item(1)->setText(  QString("Channel Num: %1").arg(channel_num));
	  ui.Device_listWidget->item(2)->setText( QString("Sample Rate: %1 k").arg(AD_rate));
	  ui.GetData->setEnabled(true);
      ui.Save->setEnabled(true);
	  LoadChannelListWidget();
      LoadDataSave();
	  QMessageBox::information(this, tr("SUCCEED!"), QString("Device Detected!"));
      m_pConnectTimer -> stop();
	}
}

void sEMG_DEMO::DrawTimerEvent()
{
    //Sys_TimeStamp++;
	//Current_TimeStamp = get_timestamp();
	UpdateDatas();
	UpdateDataView(); 
}

void  sEMG_DEMO::GetDataEnable()
{
	m_pTimer->start(100);
	//Sys_TimeStamp = get_timestamp();
	ui.GetData->setEnabled(false);
	ui.Pause->setEnabled(true);
}

void sEMG_DEMO::GetDataDisEnable()
{
	m_pTimer->stop();
	ui.GetData->setEnabled(true);
	ui.Pause->setEnabled(false);
}
//生成Channel列表
void sEMG_DEMO::LoadChannelListWidget()
{
    int ChannelId = 0;	
	for (ChannelId = 0; ChannelId < channel_num; ChannelId++)
	{
		QListWidgetItem* pItem = new QListWidgetItem;
		pItem->setText( QString("Channel: %1 ").arg(ChannelId) );	
		//关联每一项
		pItem->setData(Qt::UserRole, QVariant(ChannelId));
		ui.Channel_listWidget->addItem(pItem);
		//if (sensorNodeIter->second.phyId == nSensorId)		
			//ui.SensorList->setCurrentItem(pItem);	
	}
	if(ui.Channel_listWidget->item(0) != NULL)
		ui.Channel_listWidget->setCurrentItem(ui.Channel_listWidget->item(0));
}

void sEMG_DEMO::LoadDataSave()
{
    Data data;
	int ChannelId = 0;	
	for (ChannelId = 0; ChannelId < channel_num; ChannelId++)
		m_pDataSave->Channels.insert(std::pair<int, Data>(ChannelId, data));//生成链表，插入暂时数据data
	
}

void sEMG_DEMO::UpdateDatas()
{
	//int nIndex;

	std::map<int, Data>& channels = m_pDataSave->Channels;
	std::map<int, Data>::iterator channels_Iter;
//	for(channels_Iter = channels.begin(); channels_Iter != channels.end(); channels_Iter++)
//	{	
    QList<QListWidgetItem*> selectedItems = ui.Channel_listWidget->selectedItems();
	if (selectedItems.count() > 0)
	{
		int ChannelId = selectedItems[0]->data(Qt::UserRole).toInt();
		channels_Iter = channels.find(ChannelId);
		std::vector<sEMGdata> retDataList = m_pDataChannel->GetData(channels_Iter->first);//中间有个拷贝,不合适
		for(int i=0;i<retDataList.size();i++)
		{
			channels_Iter->second.sData.push_back(retDataList[i]);//first represent id, second represent data
			
			if (m_bStore){//如果要存数据
		        QTextStream out(SaveDataFile);
				QString strOut = QString("ChannelID: %1,%2 Data: ").arg(channels_Iter->first).arg(i);
				for (int j = 0; j < 100;j++)
				{
					strOut.append(QString("%1,").arg(retDataList[i].point[j]));
					//strOut.append(QString::number(retDataList[i].point[j]));
				}
                strOut.append(QString("\n"));
		        out << strOut;				
	         }
		}
	}
//	}			
}


void sEMG_DEMO::UpdateDataView()
{	
  /*
    std::vector<sEMGdata> retDataList = m_pDataChannel->GetData();
	for(int i=0;i<retDataList.size();i++)
	{
		m_pChartWidget->m_pData->sData.push_back(retDataList[i]);
	}
	m_pChartWidget->count = m_pChartWidget->m_pData->sData.size() * 100;
	*/
	//sprintf(str, "result = :%d\n", m_pChartWidget->count);
	//OutputDebugStringA(str);
   std::map<int, Data>& channels = m_pDataSave->Channels;
   std::map<int, Data>::iterator channels_Iter;
   QList<QListWidgetItem*> selectedItems = ui.Channel_listWidget->selectedItems();
	if (selectedItems.count() > 0)
	{
		int ChannelId = selectedItems[0]->data(Qt::UserRole).toInt();//选中的通道
		channels_Iter = channels.find(ChannelId);
        m_pChartWidget->SetCurrentSensor(ChannelId);
	    m_pChartWidget->count = channels_Iter->second.sData.size() * 100;
		UpdateValueListWidget(ChannelId);
		//UpdateDataListWidget(sensorNodeIter->second, nIndex);
        //int nWidthRequired =  channels_Iter->second.sData.size() * m_pChartWidget->GetUnit();
        int nWidthRequired = (int)(m_pChartWidget->count*m_pChartWidget->GetUnit());
        if (m_pChartWidget->width() < nWidthRequired)
			m_pChartWidget->setFixedWidth(nWidthRequired + m_pScrollArea->width() );
	 
        m_pScrollArea->ensureVisible(nWidthRequired , m_pChartWidget->height() / 2);
	    m_pChartWidget->update();//重绘制
	}

}


void sEMG_DEMO::UpdateValueListWidget(int ChannelId)
{
    //char str[100];
	if(ui.Value_listWidget->count() == 0)
	 {
		QListWidgetItem* pItem = new QListWidgetItem;
		pItem->setText(QString("Current channel: %1").arg(ChannelId));
		//pItem->setData( Qt::UserRole, QVariant(SensorNode::CT_AX));
		ui.Value_listWidget->addItem( pItem );

		QListWidgetItem* pItem_1 = new QListWidgetItem;
		pItem_1->setText(QString("Packet loss rate: %1%").arg(get_losenum()*100.0/get_timestamp(),6,'f',3));
		ui.Value_listWidget->addItem( pItem_1 );
	 }
	 else
	 {
	    ui.Value_listWidget->item(0)->setText(QString("Current channel: %1").arg(ChannelId));
		ui.Value_listWidget->item(1)->setText(QString("Packet loss rate: %1%").arg(get_losenum()*100.0/get_timestamp(),6,'f',3));
	 }
    //sprintf(str, "losenum = :%d, timestamp = %d \n", get_losenum(),get_timestamp() );
	//OutputDebugStringA(str);
}



void sEMG_DEMO::AddMessage(const QString& msg)
{

}

void sEMG_DEMO::showEvent(QShowEvent* event)
{
	resetChartWidgetSize();
	m_pChartWidget->SetDataSave(m_pDataSave);
}

void sEMG_DEMO::closeEvent(QCloseEvent* event)
{
	//自动保存数据
}

void sEMG_DEMO::resetChartWidgetSize()
{
	//m_pChartWidget->setFixedSize(m_pScrollArea->width()*1.2, m_pScrollArea->height() - 120);
	m_pChartWidget->setFixedSize(m_pScrollArea->width() - 20, m_pScrollArea->height() - 20);
}

//最大化最小化等的重定义大小事件
void sEMG_DEMO::resizeEvent(QResizeEvent * event)
{
	resetChartWidgetSize();
}


void sEMG_DEMO::StartSaveData()
{
	QString fileName = QFileDialog::getSaveFileName(this,"Save File","Result",tr("TXT Files (*.txt)"));
	if(fileName == "")
	{
		QMessageBox::information(this, tr("Error"), tr("No File Path!"));
		return;
	}
	SaveDataFile = new QFile(fileName);
	if (!SaveDataFile->open(QIODevice::ReadWrite | QIODevice::Text))
         return;
	ui.Save->setEnabled(false);
	ui.SaveStop->setEnabled(true);

	m_pSaveDataTimer = new QTimer; 
	m_nTakenTime = 0;
	m_pSaveDataTimer -> start(500);
	connect(m_pSaveDataTimer, SIGNAL(timeout()), this, SLOT(SaveDataTimerEvent()));
	m_bStore = true;
}

void sEMG_DEMO::SaveDataTimerEvent()
{
	m_nTakenTime += 1;
	QString TakenTime = QString("%1s").arg((m_nTakenTime/2.0),6,'f',1);
	ui.Label_Save_Time->setText(TakenTime);
}

void sEMG_DEMO::StopSaveData()
{
	ui.Save->setEnabled(true);
	ui.SaveStop->setEnabled(false);
	m_pSaveDataTimer -> stop();
	SaveDataFile->close();
	delete m_pSaveDataTimer;
	delete SaveDataFile;
	m_bStore = false;
}