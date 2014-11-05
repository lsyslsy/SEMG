#ifdef UNICODE
	#undef UNICODE
#endif
#include "ChartWidget.h"
#include <QPainter>
//#include <QMouseEvent>


ChartWidget::ChartWidget(void)
{
	m_CurrentChannelId = 0;
	m_nUnit = 0.5;
	//m_CurrentIndex = 0;
	count = 0;
	//ÉèÖÃ±³¾°µ÷É«°å
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	//m_pData = new Data;
}

ChartWidget::~ChartWidget(void)
{
}


void ChartWidget::SetCurrentSensor(int ChannelId)
{
	m_CurrentChannelId = ChannelId;
}

void ChartWidget::SetDataSave(DataSave* pDataSave)
{
     m_pDataSave = pDataSave;
}

/*
void ChartWidget::SetCurrentIndex(int nIndex)
{
	m_CurrentIndex = nIndex;
}*/

float ChartWidget::GetUnit()
{
	return m_nUnit;
}

void ChartWidget::SetUnit(float nUnit)
{
	m_nUnit = nUnit;
}

void ChartWidget::paintEvent(QPaintEvent *event)
{
	    
	    int windowHeight  = height();
		int windowWidth = width();
		int XAxis = windowHeight / 2;
		double AScale = windowHeight / 2.0;
		QPainter painter(this);
		painter.setBackground( QBrush( Qt::white ) );

		painter.setPen(QPen(Qt::black,  m_nUnit  , Qt::SolidLine, Qt::RoundCap));
		
		painter.drawLine(0, XAxis, windowWidth, XAxis);

		painter.drawLine(count*m_nUnit, 0, count*m_nUnit, windowHeight);

		std::map<int, Data>::iterator channels_Iter = m_pDataSave->Channels.find(m_CurrentChannelId);
			
    	if (count!=0){
		int nStartIndex = 0;
		int nDrawCount = (int) (1500 / m_nUnit) ;
		int nDataCount = count;
	
		if( nDrawCount < nDataCount)
	    {
		    nStartIndex = nDataCount - nDrawCount;
	    }
        int nDataPointCount = nDataCount - nStartIndex;

	    QPointF* dataPoints = new QPointF[ nDataPointCount ];
	    for(int i=nStartIndex; i<nDataCount; i++)
	    {         
			dataPoints[i - nStartIndex].setX(  i * m_nUnit  );
		    int j = i/100;
			dataPoints[i - nStartIndex].setY(XAxis - channels_Iter->second.sData[j].point[(i - nStartIndex)%100]/4.0*XAxis);
		    //dataPoints[i - nStartIndex].setY( XAxis - m_pData->sData[j].point[(i - nStartIndex)%100]/4.0*XAxis);          
	    }
        painter.setPen(QPen(Qt::red, m_nUnit  , Qt::DotLine, Qt::RoundCap));
	    //painter.drawPolyline(dataPoints, nDataPointCount);
		painter.drawPoints(dataPoints, nDataPointCount);
        delete[] dataPoints;

      /* QPoint* dataPoints = new QPoint[ 20 ];
        for(int i=nStartIndex; i<nStartIndex + 20; i++)
		{
			dataPoints[i - nStartIndex].setX(  i * m_nUnit);
		    dataPoints[i - nStartIndex].setY( m_pData->sData[0].point[i - nStartIndex]* AScale/30);
		}

        painter.setPen(QPen(Qt::red, 2, Qt::SolidLine, Qt::RoundCap));
	    painter.drawPolyline(dataPoints, 20);
		painter.drawPoints(dataPoints,20);
        delete[] dataPoints;*/
      
			/*QPen pen;
		pen.setWidth(3);
		pen.setBrush(Qt::red);
	    painter.setPen(pen);
        QPoint* dataPoint =new QPoint;
		dataPoint->setX(count*m_nUnit);
		dataPoint->setY(windowHeight/4);
		painter.drawPoints( dataPoint,1);
		delete[] dataPoint;*/
		}
}

//void ChartWidget::DrawCurve(QPainter& painter, const SensorNode& sensorNode, SensorNode::ChannelType channelType, int nAxisPos, double nScale, QColor color)
//{
//
//	bool bDrawFFT = false;
//
//	int nStartIndex = 0;
//	int nDrawCount = 150;
//	int nDataCount = sensorNode.DataCount();
//	if(nDrawCount > 0 && nDrawCount < nDataCount)
//	{
//		nStartIndex = nDataCount - nDrawCount;
//	}
//
//	int nDataPointCount = nDataCount - nStartIndex;
//	QPoint* dataPoints = new QPoint[ nDataPointCount ];
//
//	for(int i=nStartIndex; i<nDataCount; i++)
//	{
//		dataPoints[i - nStartIndex].setX( i * m_nUnit );
//		if(channelType >= SensorNode::CT_RX && channelType <= SensorNode::CT_RW)
//		{		
//			//dataPoints[i - nStartIndex].setY( nAxisPos - sensorNode.GetData(channelType, i, bUseInterpolate, nInterpolateSize, bUseFFF, nFFTWindow, nMinFFTFreq, nMaxFFTFreq) * nScale );
//			dataPoints[i - nStartIndex].setY( nAxisPos - sensorNode.GetData(channelType, i, 0, 0, 0, 0, 0, 0) * nScale );
//		}
//		else
//		{
//			//dataPoints[i - nStartIndex].setY( nAxisPos - sensorNode.GetData(channelType, i, bUseFilter, nBufferSize, bUseFFF, nFFTWindow, nMinFFTFreq, nMaxFFTFreq) * nScale);
//			dataPoints[i - nStartIndex].setY( nAxisPos - sensorNode.GetData(channelType, i, 0, 0, 0, 0, 0, 0) * nScale);
//		}  
//	}
//
//	painter.setPen(QPen(color, m_nUnit / 4, Qt::DotLine, Qt::RoundCap));
//	painter.drawPolyline(dataPoints, nDataPointCount);
//	//painter.setPen(QPen(Qt::black, m_nUnit / 2, Qt::SolidLine, Qt::RoundCap));
//	//painter.drawPoints(dataPoints, nDataPointCount);
//	
//	delete[] dataPoints;	
//	
//}




//void ChartWidget::mouseMoveEvent(QMouseEvent * event)
//{
//	if (event->buttons() & Qt::LeftButton)
//	{
//		UpdateCurrentIndexCursor(event->x());
//	}
//}
//
//void ChartWidget::mousePressEvent(QMouseEvent * event)
//{
//	if(event->button() == Qt::LeftButton)
//	{
//		UpdateCurrentIndexCursor(event->x());
//	}
//}

//void ChartWidget::UpdateCurrentIndexCursor(int xPos)
//{
//	emit currentIndexChanged(xPos / m_nUnit);
//}