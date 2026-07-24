#ifndef SIGNALCOREDEFINE_H
#define SIGNALCOREDEFINE_H
#include<InstancePtrBirth.h>
#include<QObject>
#include<QVariant>
#include<CUTypeDefine.h>
#include<EquipmentIndex.h>
#include<Singleton.h>
#include<CharAndNumProcessing.h>
namespace SignalCore
{

    struct DefReplayInfo
    {
        QString db_path;
        int64_t id;
        int  version = 1;//0x00:旧版信号处理 ; 0x01:新版信号处理
    };
    /** @brief 数据保存   2025-01-27 13:08:53 .*/
    struct DefEchoSaveConfig
    {
        int64_t   mIsSaveRawData = 0x00 ; //是否保存原始数据(保存时为时戳)
        int       mDataSaveBy    = 0x01 ;
        //数据保存:0x00:无效, 0x01:通过阵面坐标系进行保存 0x02:通过站心坐标系进行保存 0x03:保存跟踪波束数据
        // 0x04:保存信号处理识别数据 0x05:无人机
        double    azi_deg_min    = 0x00 ;
        double    azi_deg_max    = 0x00 ;
        double    ele_deg_min    = 0x00 ;
        double    ele_deg_max    = 0x00 ;
        QString   rootFolder     = "./" ; //原始数据保存根目录
        bool isAngleInScope(double azi_rad, double ele_rad);
        int isDronInScope(PointRAE /*dronLoc*/, double /*azi_rad*/, double /*ele_rad*/ );

        QJsonObject toJsonObj();
        void fromJson(QJsonObject obj);
        MacFuncJsonDataOp

    };
    /************************************| 杂波图 |************************************/
    struct WaveIndex
    {
        WaveIndex(int arrayIdx, int pulseNbr, int sampleNbr, float bc_azi_rad, float bc_ele_rad);
        int   arrayIndex = 0;//阵面序号
        float bc_azi_rad = 0;//阵面方位
        float bc_ele_rad = 0;//阵面俯仰
        int   pulseAmount = 0;
        int   sampleNbr = 0;
        QString toStr(qint32 reservedWidth = 0 );


        friend bool operator == (WaveIndex v1, WaveIndex v2)
        {
            if( v1.arrayIndex != v2.arrayIndex )
            {
                return false;
            }
            if( std::abs(v1.bc_azi_rad - v2.bc_azi_rad) > 1e-6 )
            {
                return false;
            }
            if( std::abs(v1.bc_ele_rad - v2.bc_ele_rad) > 1e-6 )
            {
                return false;
            }
            if( v1.pulseAmount  != v2.pulseAmount)
            {
                return false;
            }
            if( v1.pulseAmount  != v2.pulseAmount)
            {
                return false;
            }
            return  true;
        }
        friend bool operator < ( WaveIndex v1, WaveIndex v2)
        {
            if( v1 == v2 )
            {
                return false;
            }
            if( v1.arrayIndex != v2.arrayIndex )
            {
                return v1.arrayIndex < v2.arrayIndex;
            }
            if( v1.pulseAmount != v2.pulseAmount )
            {
                return v1.pulseAmount < v2.pulseAmount;
            }
            if( v1.sampleNbr != v2.sampleNbr )
            {
                return v1.sampleNbr < v2.sampleNbr;
            }
            //阵面序号相同
            if( std::abs(v1.bc_azi_rad - v2.bc_azi_rad) > 1e-6 )
            {
                return v1.bc_azi_rad < v2.bc_azi_rad;
            }
            //波束方位相同
            return v1.bc_ele_rad < v2.bc_ele_rad;
        }
    };
    struct DefClutterMemory  : public InstancePtrBirth<DefClutterMemory>
    {
        DefClutterMemory( int pulseNbr, int sampleNbr  );
        ~DefClutterMemory();
        //在使用前，需要判断使用合理(冲数量和采样点数量一致)
        bool valid( int pulseNbr, int sampleNbr );
        float rateWeight = 0.04;//迭代权重
        int   pulseNbr = 0;
        int   sampleNbr = 0;
        //
        int indexValid( int pulseIdx, int sampleIdx);
        int update( int pulseIdx, int sampleIdx, int amp );
        float getValue( int pulseIdx, int sampleIdx  );
        float* getArray();
        int32_t size();
        void save( WaveIndex idx )
        {
            QString path = QString("./log/clutter_save/%0_%1_%2.dat").arg( idx.arrayIndex ).arg( int(  rad2deg( idx.bc_azi_rad)) )
                           .arg( int( rad2deg( idx.bc_ele_rad ) ) );
            SaveData( reinterpret_cast<char*>(&this->pulseNbr), sizeof(int), path);
            SaveDataAppend( reinterpret_cast<char*>(&this->sampleNbr), sizeof(int), path);
            SaveDataAppend( reinterpret_cast<char*>(*this->array), sizeof(float)*this->size(), path);
        }
    private:
        float** array = nullptr;
    };
    /************************************| 杂波图管理器 |************************************/
    struct DefClutterMemoryMng : public Singleton<DefClutterMemoryMng>
    {
        enum class ClutterType
        {
            TStatic = 0x11,
            TDynamic = 0x12
        };

        DefClutterMemoryMng();
        ~DefClutterMemoryMng();
        shared_ptr< DefClutterMemory> getClutter(
            int arrayIdx, float bc_azi_rad,
            float bc_ele_rad, ClutterType type   /*0x11:静态  0x12:动态*/,
            int pulseNbr, int sampleNbr
        );

        QMap<WaveIndex, shared_ptr< DefClutterMemory> > radarClutterStatic;
        QMap<WaveIndex, shared_ptr< DefClutterMemory> > radarClutterDynamic ;
    };

}
#endif // DEFINE_H
