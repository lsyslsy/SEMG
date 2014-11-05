#pragma once

#include <QWidget>
//#include <map>
#include <math.h>
#include "Data.h"
#include "DataSave.h"

class DataChannel;

class ChartWidget : public QWidget
{
	Q_OBJECT
public:


	ChartWidget(void);
	~ChartWidget(void);

	void paintEvent(QPaintEvent *event);

	void SetCurrentSensor(int ChannelId);
	void SetDataSave(DataSave* pDataSave);
	//void SetCurrentIndex(int nIndex);

	void SetUnit(float nUnit);
	float GetUnit();
    int count;
	//Data* m_pData;
//signals:
//	void currentIndexChanged(int value);
//protected:
	//void mouseMoveEvent(QMouseEvent * event);
	//void mousePressEvent(QMouseEvent * event);

private:
	int m_CurrentChannelId;
	//int m_CurrentIndex;
	float m_nUnit;
	DataSave* m_pDataSave;

	//void DrawCurve(QPainter& painter, const SensorNode& sensorNode, SensorNode::ChannelType channelType, int nAxisPos, double nScale, QColor color);
	//void UpdateCurrentIndexCursor(int xPos);
};
