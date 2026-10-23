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

    /**
     * @brief 回波数据录取配置通信协议类型。
     * @details Query、Report、Command、Confirm 组成完整配置闭环，协议号仅在 SignalCore 软件内使用。
     * @date      2026-08-18 00:00:00
     * @author    roiade
     * @note
     */
    enum class ProdectProtocolType : uint16_t
    {
        EchoDataCaptureConfigQuery = 0x0225,   /**< 查询当前生效的回波数据录取配置。 */
        EchoDataCaptureConfigReport = 0x0226,  /**< 上报当前生效的回波数据录取配置。 */
        EchoDataCaptureConfigCommand = 0x0227, /**< 下发完整的回波数据录取配置快照。 */
        EchoDataCaptureConfigConfirm = 0x0228  /**< 返回回波数据录取配置处理结果。 */
    };

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
        QString   rootFolder     = "./dataSave/信号处理回波保存/" ; //原始数据保存根目录
        bool isAngleInScope(double azi_rad, double ele_rad);
        int isDronInScope(PointRAE /*dronLoc*/, double /*azi_rad*/, double /*ele_rad*/ );

        /**
         * @brief 将回波数据录取配置转换为 JSON 对象。
         * @details 输出字段与网页配置服务使用的 settings 对象保持一致，其中 mIsSaveRawData 为设备生成的启用时间戳或 0。
         * @return 包含完整回波数据录取配置的 JSON 对象。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        QJsonObject toJsonObj();

        /**
         * @brief 从 JSON 对象更新回波数据录取配置。
         * @details 仅更新 JSON 中存在的字段；启用时间戳按 64 位整数读取，避免长时间运行后重启加载时发生截断。
         * @param[in] obj 包含回波数据录取配置字段的 JSON 对象。
         * @return 无。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
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
