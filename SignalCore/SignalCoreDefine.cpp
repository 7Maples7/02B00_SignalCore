#include "SignalCoreDefine.h"
#include<QObject>
namespace SignalCore
{

    bool DefEchoSaveConfig::isAngleInScope(double azi_rad, double ele_rad)
    {
        if(azi_rad < azi_deg_min * M_PI / 180.0)
        {
            return false;
        }
        if(azi_rad > azi_deg_max * M_PI / 180.0)
        {
            return false;
        }
        if(ele_rad < ele_deg_min * M_PI / 180.0)
        {
            return false;
        }
        if(ele_rad > ele_deg_max * M_PI / 180.0)
        {
            return false;
        }
        return true;
    }

    int DefEchoSaveConfig::isDronInScope(PointRAE, double, double)
    {
        return 0x00;
    }

    QJsonObject DefEchoSaveConfig::toJsonObj()
    {
        QJsonObject obj;
        obj.insert("mIsSaveRawData", (qint64)mIsSaveRawData);
        obj.insert("mDataSaveBy", mDataSaveBy);
        obj.insert("azi_deg_min", azi_deg_min);
        obj.insert("azi_deg_max", azi_deg_max);
        obj.insert("ele_deg_min", ele_deg_min);
        obj.insert("ele_deg_max", ele_deg_max);
        obj.insert("rootFolder", rootFolder);
        return obj;
    }

    void DefEchoSaveConfig::fromJson(QJsonObject obj)
    {
        if(obj.contains("mIsSaveRawData"))
        {
            this->mIsSaveRawData = obj.value("mIsSaveRawData").toInt();
        }
        if(obj.contains("mDataSaveBy"))
        {
            this->mDataSaveBy = obj.value("mDataSaveBy").toInt();
        }
        if(obj.contains("azi_deg_min"))
        {
            this->azi_deg_min = obj.value("azi_deg_min").toDouble();
        }
        if(obj.contains("azi_deg_max"))
        {
            this->azi_deg_max = obj.value("azi_deg_max").toDouble();
        }
        if(obj.contains("ele_deg_min"))
        {
            this->ele_deg_min = obj.value("ele_deg_min").toDouble();
        }
        if(obj.contains("ele_deg_max"))
        {
            this->ele_deg_max = obj.value("ele_deg_max").toDouble();
        }
        if(obj.contains("rootFolder"))
        {
            this->rootFolder = obj.value("rootFolder").toString();
        }
    }

    DefClutterMemory::DefClutterMemory(int pulseNbr, int sampleNbr)
    {
        this->pulseNbr = pulseNbr;
        this->sampleNbr = sampleNbr;
        this->array = new float*();
        *this->array = ( float* )( malloc( sampleNbr * pulseNbr * sizeof (float) ) );
        memset( *this->array, 0, sampleNbr * pulseNbr );
    }

    DefClutterMemory::~DefClutterMemory()
    {
        free(*this->array );
        delete this->array;
    }

    bool DefClutterMemory::valid(int pulseNbr, int sampleNbr)
    {
        if( this->pulseNbr != pulseNbr || this->sampleNbr != sampleNbr )
        {
            return false;
        }
        return true;
    }

    int DefClutterMemory::indexValid(int pulseIdx, int sampleIdx)
    {
        if( pulseIdx < 0 || pulseIdx >= pulseNbr )
        {
            return 0x01;
        }
        if( sampleIdx < 0 || sampleIdx >= sampleNbr )
        {
            return 0x02;
        }
        return 0x00;
    }

    int DefClutterMemory::update(int pulseIdx, int sampleIdx, int amp)
    {
        if( this->indexValid(pulseIdx, sampleIdx) != 0x00 )
        {
            return this->indexValid(pulseIdx, sampleIdx);
        }
        this->array[pulseIdx][sampleIdx] *= (1 - rateWeight) ;
        this->array[pulseIdx][sampleIdx] += amp * rateWeight;
        return 0x00;
    }

    float DefClutterMemory::getValue(int pulseIdx, int sampleIdx)
    {
        if(this->indexValid( pulseIdx, sampleIdx ) != 0x00)
        {
            return this->indexValid(pulseIdx, sampleIdx);
        }
        return this->array[pulseIdx][sampleIdx];
    }

    float* DefClutterMemory::getArray()
    {
        return this->array[0];
    }

    int32_t DefClutterMemory::size()
    {
        return  sampleNbr * pulseNbr;
    }

    DefClutterMemoryMng::DefClutterMemoryMng()
    {
    }

    DefClutterMemoryMng::~DefClutterMemoryMng()
    {
        this->radarClutterStatic.clear();
        this->radarClutterDynamic.clear();
    }

    shared_ptr<DefClutterMemory> DefClutterMemoryMng::getClutter(int arrayIdx, float bc_azi_rad,
            float bc_ele_rad, DefClutterMemoryMng::ClutterType type, int pulseNbr, int sampleNbr)
    {
        if(radarClutterStatic.size() >= 1000)
        {
            qDebug() << "杂波图数量过大" << radarClutterStatic.size();
        }
        if(radarClutterDynamic.size() >= 1000)
        {
            qDebug() << "杂波图数量过大" << radarClutterDynamic.size();
        }
        QMap<WaveIndex, shared_ptr< DefClutterMemory> >* map = nullptr;
        switch (type)
        {
            case ClutterType::TStatic:
                map = &this->radarClutterStatic;
                break;
            case ClutterType::TDynamic:
                map = &this->radarClutterDynamic;
                break;
            default:
                break;
        }
        assert( map != nullptr );
        shared_ptr< DefClutterMemory> result = nullptr;
        WaveIndex index( arrayIdx, pulseNbr, sampleNbr, bc_azi_rad, bc_ele_rad );
        if( map->contains( index ) )
        {
            bool valid = (*map)[index]->valid(pulseNbr, sampleNbr);
            if( !valid )
            {
//                qDebug()<<"index:"<< index.toStr();
                qDebug() << "重置杂波图单元";
//                qDebug()<<"old info:"<<(*map)[index]->pulseNbr << (*map)[index]->sampleNbr;
//                qDebug()<< index.toStr()<<"pulseNbr sampleNbr:"  << pulseNbr << sampleNbr;
                map->remove( index );
                (*map)[index] = DefClutterMemory::_create( MacLoc, pulseNbr, sampleNbr );
//                qDebug()<<"new info:"<<(*map)[index]->pulseNbr << (*map)[index]->sampleNbr;
//                qDebug()<<  (*map)[index]->valid(pulseNbr, sampleNbr);
            }
            else
            {
            }
        }
        else
        {
            (*map)[index] = DefClutterMemory::_create( MacLoc, pulseNbr, sampleNbr );
        }
        result = (*map)[index];
        assert(result != nullptr);
        return result;
    }

    WaveIndex::WaveIndex(int arrayIdx, int pulseNbr, int sampleNbr, float bc_azi_rad, float bc_ele_rad)
    {
        this->arrayIndex = arrayIdx;
        this->bc_ele_rad = bc_ele_rad;
        this->bc_azi_rad = bc_azi_rad;
        this->pulseAmount = pulseNbr;
        this->sampleNbr = sampleNbr;
    }


    QString WaveIndex::toStr(qint32 reservedWidth)
    {
        QString str;
        str += QString("[arrayIndex:%0]").arg(arrayIndex, 6 + reservedWidth, 10, QLatin1Char(' '));
        str += QString("[pulseAmount:%0]").arg(pulseAmount, 6 + reservedWidth, 10, QLatin1Char(' '));
        str += QString("[bc_azi_deg:%0]").arg(rad2deg(bc_azi_rad), 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[bc_ele_deg:%0]").arg(rad2deg(bc_ele_rad), 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        return str;
    }



}
