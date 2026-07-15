#ifndef RADARECHODATAADMS_H
#define RADARECHODATAADMS_H


/**
* @file  RadarEchoDataAdms.h
* @brief MyClass:  雷达数据录取:回波
* @author roiade
* @date   2024-02-22  16:0:55
* @version
* @copyright
*/
#include<Singleton.h>
#include<ConnectionPool.h>
#include<QTimer>
#include<CUTypeDefine.h>
#include<DronePosition.h>
#include<CompInertialNav.h>
#include<concurrentqueue.h>
#include<thread>
#include<CharAndNumProcessing.h>
#include<QObject>
#include<RGTimeSystem.h>
#include<CharAndNumProcessing.h>
#include<DefExchangeWithSP.h>
#include<QtMaterialCircularProgressCustom.h>
#include"CommonDefine.h"
#include"SignalCoreGlobalSignal.h"
#include"SignalCoreGlobalVal.h"
#include<ScndLvlDefn/DSignalProcPointProtocol.h>

namespace SignalCore
{


    QJsonObject toJsonObj( Attitude v );
    QJsonObject toJsonObj(DroneLocInfo v );
    QJsonObject toJsonObj( RadarMeasureCoor5D v);
    QJsonObject toJsonObj( SMeteorologicalElementsSendWithTime v);
    QJsonObject toJsonObj(IRadar_V4_1::RadarTrkSingleTrk trk);


    QByteArray JsonData( QJsonObject obj );

    class RadarEchoDataAdms: public QObject, public BuildStatus, public Singleton<RadarEchoDataAdms>
    {
        Q_OBJECT
        /// 1.修改目录之后需要重新设定数据库
    public:
        RadarEchoDataAdms();
        ~RadarEchoDataAdms();
    public:
        // BuildStatus interface
        virtual bool variableApplication() override;
        virtual bool internalSignalConnect() override;
        virtual bool exec() override;
        virtual void destroy() override;
        virtual void active() override;
    public:
        int echoDataSaveAffterPar(shared_ptr<RawData2SP> raw_info,
                                  QList<IRadarV51::SignalProcPoint>& points );
        int loadData4Reloay( QString path, int64_t id, char* memory, int lenMax, shared_ptr<RawData2SP>& ptr );
        QString SPPnt2Str(IRadar_V4_1::SpDataDot& pnt, bool isRadians = true);
        //提取数据
        void RFExtractRawData( double& rate, bool& is_running, QString path);
    public slots:
        void Test_forSignal(QString dataName, uint64_t dataNumber, uint64_t dataNumber_max);
    protected:
        /** @brief 保存路径初始化   2024-02-22 18:08:28 .*/
        void RFSaveFolderInitial();
    private:
        moodycamel::ConcurrentQueue< shared_ptr<RawData2SP> > dataAffterParingLst;
        bool needResetSavePath = false;
        QString mEchodataSaveRoot = "./dataSave/解析后数据录取";
        QString mEchodataSavePath = "";
        QString mEchodataSaveTable = "EchoDataInfo";//保存使用的表
        QString data_namepath = "";//选择的数据包
        qint64  mRawDataSaveTime = 0;
    };

}
#endif // RADARECHODATAADMS_H
