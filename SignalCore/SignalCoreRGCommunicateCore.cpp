#include "SignalCoreRGCommunicateCore.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <cmath>
namespace SignalCore
{
    namespace
    {
        const QString kEchoDataCaptureConfigId = QStringLiteral("echo-data-capture"); /**< 回波数据录取配置对象标识。 */
        const int kEchoDataCaptureSchemaVersion = 1; /**< 回波数据录取配置载荷版本。 */
        QMutex gEchoDataCaptureConfigMutex; /**< 保护配置服务的查询、版本校验和应用流程。 */
        qint64 gEchoDataCaptureRevision = 1; /**< 当前回波数据录取配置版本，从 1 开始递增。 */
        qint64 gEchoDataCaptureUpdatedAtMs = 0; /**< 最近一次成功生效的 UTC 毫秒时间戳。 */

        /**
         * @brief 构造回波数据录取配置的统一响应报文。
         * @details 响应使用当前 SignalCore 软件标识作为发送方，并将原请求发送方作为接收方；负载固定为紧凑 JSON。
         * @param[in] requestHeader 原始请求通用报头。
         * @param[in] protocolType 响应协议类型。
         * @param[in] jsonPayload 待封装的 JSON 负载。
         * @return 包含通用报头和 JSON 负载的完整响应报文。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        QByteArray buildEchoDataCaptureResponse(const IRadarV51::UniversalDataHeader& requestHeader,
                                                uint16_t protocolType,
                                                const QByteArray& jsonPayload)
        {
            IRadarV51::UniversalDataHeader responseHeader;
            responseHeader.headerVersion.fields.main_ver = 0x05;
            responseHeader.headerVersion.fields.sub_ver = 0x01;
            responseHeader.dataVersion = GlobalVal::GetInstance()->getSoftwareVersionP();
            responseHeader.senderCategory = GlobalVal::GetInstance()->getSoftwareName();
            responseHeader.receiverCategory = requestHeader.senderCategory;
            responseHeader.utcTimeMs = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
            responseHeader.protocolType = protocolType;
            responseHeader.dataType = static_cast<uint16_t>(IRadarV51::UniversalDataHeader::DataTypeValue::DataTypeJson);
            responseHeader.setPayloadLength(static_cast<uint32_t>(jsonPayload.size()));
            QByteArray responseData;
            const auto responseHeaderData = responseHeader.data();
            responseData.append(responseHeaderData.data(), responseHeaderData.size());
            responseData.append(jsonPayload);
            return responseData;
        }

        /**
         * @brief 向配置客户端发送回波数据录取配置响应。
         * @details 响应主题固定为目标软件的上行配置主题，确保网页可通过配置转发链路收到响应。
         * @param[in] communicateCore 当前通信核心对象。
         * @param[in] responseData 已封装的完整响应报文。
         * @return 无。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        void sendEchoDataCaptureResponse(SignalCoreRGCommunicateCore& communicateCore,
                                         QByteArray& responseData)
        {
            const std::string responseTheme = QString("/config/up/%0")
                                              .arg(QString::fromStdString(GlobalVal::GetInstance()->getSoftwareName().fields.toString()))
                                              .toStdString();
            communicateCore.RFWriteDataPragram(responseData.data(), responseData.size(), responseTheme);
        }

        /**
         * @brief 从请求报文中解析 JSON 对象负载。
         * @details 先校验声明负载长度不超过实际报文长度，再使用 Qt JSON 解析器读取对象，防止截断报文和非对象负载进入配置处理流程。
         * @param[in] data 请求报文首地址。
         * @param[in] size 请求报文总长度，单位为字节。
         * @param[in] header 已校验存在的通用报头。
         * @param[out] jsonObject 解析成功后的 JSON 对象。
         * @param[out] errorMessage 失败原因。
         * @return true 表示 JSON 对象解析成功；false 表示报文长度或 JSON 格式无效。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        bool parseEchoDataCaptureJson(const char* data,
                                      int size,
                                      const IRadarV51::UniversalDataHeader& header,
                                      QJsonObject& jsonObject,
                                      QString& errorMessage)
        {
            const int headerSize = static_cast<int>(sizeof(IRadarV51::UniversalDataHeader));
            const int availablePayloadSize = size - headerSize;
            if(header.payloadLength > static_cast<uint32_t>(availablePayloadSize))
            {
                errorMessage = QStringLiteral("payload_length_invalid");
                return false;
            }
            QJsonParseError parseError;
            const QByteArray payload(data + headerSize, static_cast<int>(header.payloadLength));
            const QJsonDocument jsonDocument = QJsonDocument::fromJson(payload, &parseError);
            if(parseError.error != QJsonParseError::NoError || jsonDocument.isObject() == false)
            {
                errorMessage = QStringLiteral("invalid_json");
                return false;
            }
            jsonObject = jsonDocument.object();
            return true;
        }

        /**
         * @brief 追加单个配置字段校验错误。
         * @details 错误对象字段名与网页字段路径保持一致，使客户端能够准确显示被拒绝的参数及原因。
         * @param[in,out] fieldErrors 配置字段错误数组。
         * @param[in] fieldPath 出错字段路径。
         * @param[in] message 出错原因。
         * @return 无。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        void appendEchoDataCaptureFieldError(QJsonArray& fieldErrors,
                                             const QString& fieldPath,
                                             const QString& message)
        {
            QJsonObject fieldError;
            fieldError.insert(QStringLiteral("fieldPath"), fieldPath);
            fieldError.insert(QStringLiteral("message"), message);
            fieldErrors.append(fieldError);
        }

        /**
         * @brief 校验并转换完整回波数据录取配置。
         * @details 仅接收网页下发的可配置 settings 字段，校验录取方式、角度范围和所有数值字段。rootFolder 为设备本地只读状态，不接受远程修改；开启录取时由本机生成启用时间戳。
         * @param[in] settingsJson 网页下发的可配置 settings 对象。
         * @param[in] currentConfig 当前生效配置，用于保留已开启录取的启用时间戳。
         * @param[out] candidateConfig 通过校验后可持久化的候选配置。
         * @param[out] fieldErrors 参数校验失败明细。
         * @return true 表示候选配置有效；false 表示存在字段错误。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        bool parseEchoDataCaptureSettings(const QJsonObject& settingsJson,
                                          const DefEchoSaveConfig& currentConfig,
                                          DefEchoSaveConfig& candidateConfig,
                                          QJsonArray& fieldErrors)
        {
            const QStringList requiredNumberFields =
            {
                QStringLiteral("mIsSaveRawData"), QStringLiteral("mDataSaveBy"),
                QStringLiteral("azi_deg_min"), QStringLiteral("azi_deg_max"),
                QStringLiteral("ele_deg_min"), QStringLiteral("ele_deg_max")
            };
            for(const QString& fieldName : requiredNumberFields)
            {
                const QJsonValue fieldValue = settingsJson.value(fieldName);
                if(fieldValue.isDouble() == false || std::isfinite(fieldValue.toDouble()) == false)
                {
                    appendEchoDataCaptureFieldError(fieldErrors,
                                                    QStringLiteral("settings.") + fieldName,
                                                    QStringLiteral("必须为有限数值"));
                }
            }
            if(fieldErrors.isEmpty() == false)
            {
                return false;
            }
            const double modeValue = settingsJson.value(QStringLiteral("mDataSaveBy")).toDouble();
            if(std::floor(modeValue) != modeValue || modeValue < 0 || modeValue > 6)
            {
                appendEchoDataCaptureFieldError(fieldErrors,
                                                QStringLiteral("settings.mDataSaveBy"),
                                                QStringLiteral("录取方式必须为 0 至 6 的整数"));
            }
            const double aziMin = settingsJson.value(QStringLiteral("azi_deg_min")).toDouble();
            const double aziMax = settingsJson.value(QStringLiteral("azi_deg_max")).toDouble();
            const double eleMin = settingsJson.value(QStringLiteral("ele_deg_min")).toDouble();
            const double eleMax = settingsJson.value(QStringLiteral("ele_deg_max")).toDouble();
            if(aziMin < -360.0 || aziMin > 360.0 || aziMax < -360.0 || aziMax > 360.0 || aziMin > aziMax)
            {
                appendEchoDataCaptureFieldError(fieldErrors,
                                                QStringLiteral("settings.azi_deg_min"),
                                                QStringLiteral("方位范围必须在 -360 至 360 度且下限不大于上限"));
            }
            if(eleMin < -90.0 || eleMin > 90.0 || eleMax < -90.0 || eleMax > 90.0 || eleMin > eleMax)
            {
                appendEchoDataCaptureFieldError(fieldErrors,
                                                QStringLiteral("settings.ele_deg_min"),
                                                QStringLiteral("俯仰范围必须在 -90 至 90 度且下限不大于上限"));
            }
            const bool captureEnabled = settingsJson.value(QStringLiteral("mIsSaveRawData")).toDouble() > 0.0;
            const int dataSaveBy = static_cast<int>(modeValue);
            if(captureEnabled && dataSaveBy == 0)
            {
                appendEchoDataCaptureFieldError(fieldErrors,
                                                QStringLiteral("settings.mDataSaveBy"),
                                                QStringLiteral("开启录取时不能使用无效录取方式"));
            }
            if(fieldErrors.isEmpty() == false)
            {
                return false;
            }
            candidateConfig = currentConfig;
            candidateConfig.mDataSaveBy = dataSaveBy;
            candidateConfig.azi_deg_min = aziMin;
            candidateConfig.azi_deg_max = aziMax;
            candidateConfig.ele_deg_min = eleMin;
            candidateConfig.ele_deg_max = eleMax;
            if(captureEnabled)
            {
                candidateConfig.mIsSaveRawData = currentConfig.mIsSaveRawData > 0
                                                 ? currentConfig.mIsSaveRawData
                                                 : qMax<qint64>(1, RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms());
            }
            else
            {
                candidateConfig.mIsSaveRawData = 0;
            }
            return true;
        }

        /**
         * @brief 构造当前生效回波数据录取配置快照。
         * @details 快照包含网页显示所需的配置标识、版本、生效时间、更新来源和 DefEchoSaveConfig 原始字段。
         * @param[in] config 当前生效的回波数据录取配置。
         * @param[in] revision 当前配置版本。
         * @param[in] updatedAtMs 最近成功生效的 UTC 毫秒时间戳。
         * @return 可作为 Report 负载的配置快照对象。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        QJsonObject buildEchoDataCaptureSnapshot(DefEchoSaveConfig config,
                qint64 revision,
                qint64 updatedAtMs)
        {
            QJsonObject snapshot;
            snapshot.insert(QStringLiteral("schemaVersion"), kEchoDataCaptureSchemaVersion);
            snapshot.insert(QStringLiteral("configId"), kEchoDataCaptureConfigId);
            snapshot.insert(QStringLiteral("revision"), revision);
            snapshot.insert(QStringLiteral("updatedAtMs"), updatedAtMs);
            snapshot.insert(QStringLiteral("updatedBy"), QStringLiteral("SignalCore"));
            snapshot.insert(QStringLiteral("settings"), config.toJsonObj());
            return snapshot;
        }

        /**
         * @brief 构造回波数据录取配置确认结果。
         * @details 确认结果固定返回请求标识、状态码、处理信息、实际版本、重启标志和字段错误，供网页显示及后续回读判断。
         * @param[in] requestId 客户端请求标识。
         * @param[in] statusCode 处理状态码。
         * @param[in] message 处理说明。
         * @param[in] appliedRevision 实际生效配置版本。
         * @param[in] fieldErrors 字段校验错误明细。
         * @return 可作为 Confirm 负载的结果对象。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        QJsonObject buildEchoDataCaptureConfirm(const QString& requestId,
                                                const QJsonValue& statusCode,
                                                const QString& message,
                                                qint64 appliedRevision,
                                                const QJsonArray& fieldErrors)
        {
            QJsonObject confirmObject;
            confirmObject.insert(QStringLiteral("requestId"), requestId);
            confirmObject.insert(QStringLiteral("statusCode"), statusCode);
            confirmObject.insert(QStringLiteral("message"), message);
            confirmObject.insert(QStringLiteral("appliedRevision"), appliedRevision);
            confirmObject.insert(QStringLiteral("needRestart"), false);
            confirmObject.insert(QStringLiteral("fieldErrors"), fieldErrors);
            return confirmObject;
        }
    }

    SignalCoreRGCommunicateCore::SignalCoreRGCommunicateCore()
    {
        this->active();
    }

    SignalCoreRGCommunicateCore::~SignalCoreRGCommunicateCore()
    {
        qDebug() << "SignalCoreRGCommunicateCore::~SignalCoreRGCommunicateCore() start";
        this->destroy();
        qDebug() << "SignalCoreRGCommunicateCore::~SignalCoreRGCommunicateCore() end";
    }


    void SignalCoreRGCommunicateCore::RFReadDataPragram(char* data, int size, std::string theme)
    {
        if(theme == "/config/down/ff000" || theme == "/config/down/FF000")
        {
            if( size >= (int)sizeof(IRadarV51::UniversalDataHeader) )
            {
                IRadarV51::UniversalDataHeader* header = reinterpret_cast<IRadarV51::UniversalDataHeader*>(data);
                //配置文件获取
                if( header->protocolType == (uint16_t)IRadarV51::UniversalDataHeader::CommonProtocolType::ConfigPackageDownloadQuery &&
                        header->receiverCategory.value == IRadarV51::UniversalDataHeader::software_category_from_str("ff000")
                  )
                {
                    DefConfigPkg::DefConfigSingle configUnit;
                    HANDLE_RADAR_CONFIG_DOWNLOAD_RESPONSE_V5(configUnit);//配置文件
                }
            }
        }
        if(theme == QString("/config/down/%0").arg(QString::fromStdString(GlobalVal::GetInstance()->occupySoftwareName()->fields.toString())).toStdString())
        {
            if( size >= (int)sizeof(IRadarV51::UniversalDataHeader) )
            {
                IRadarV51::UniversalDataHeader* header = reinterpret_cast<IRadarV51::UniversalDataHeader*>(data);
                if(header->isValid() == false)
                {
                    qWarning() << "拒绝无效通用报头的回波数据录取配置请求";
                    return;
                }
                HANDLE_RADAR_UNIVERSAL_V5_MSG(header, data, size);
                const bool isEchoDataCaptureProtocol = header->protocolType == static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigQuery)
                                                       || header->protocolType == static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigCommand);
                if(header->receiverCategory.value != GlobalVal::GetInstance()->getSoftwareName().value || isEchoDataCaptureProtocol == false)
                {
                    return;
                }
                if(header->dataType != static_cast<uint16_t>(IRadarV51::UniversalDataHeader::DataTypeValue::DataTypeJson))
                {
                    if(header->protocolType == static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigCommand))
                    {
                        const QJsonObject confirmObject = buildEchoDataCaptureConfirm(QString(),
                                                          QStringLiteral("invalid_data_type"),
                                                          QStringLiteral("回波数据录取配置必须使用 JSON 负载"),
                                                          gEchoDataCaptureRevision,
                                                          QJsonArray());
                        QByteArray responseData = buildEchoDataCaptureResponse(*header,
                                                  static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigConfirm),
                                                  QJsonDocument(confirmObject).toJson(QJsonDocument::Compact));
                        sendEchoDataCaptureResponse(*this, responseData);
                    }
                    return;
                }
                QJsonObject requestObject;
                QString parseErrorMessage;
                if(parseEchoDataCaptureJson(data, size, *header, requestObject, parseErrorMessage) == false)
                {
                    if(header->protocolType == static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigCommand))
                    {
                        const QJsonObject confirmObject = buildEchoDataCaptureConfirm(QString(),
                                                          parseErrorMessage,
                                                          QStringLiteral("回波数据录取配置请求格式无效"),
                                                          gEchoDataCaptureRevision,
                                                          QJsonArray());
                        QByteArray responseData = buildEchoDataCaptureResponse(*header,
                                                  static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigConfirm),
                                                  QJsonDocument(confirmObject).toJson(QJsonDocument::Compact));
                        sendEchoDataCaptureResponse(*this, responseData);
                    }
                    return;
                }
                if(header->protocolType == static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigQuery))
                {
                    QMutexLocker locker(&gEchoDataCaptureConfigMutex);
                    if(gEchoDataCaptureUpdatedAtMs == 0)
                    {
                        gEchoDataCaptureUpdatedAtMs = QDateTime::currentMSecsSinceEpoch();
                    }
                    const QJsonObject snapshot = buildEchoDataCaptureSnapshot(GlobalVal::GetInstance()->getRadarDataSaveInfo(),
                                                 gEchoDataCaptureRevision,
                                                 gEchoDataCaptureUpdatedAtMs);
                    QByteArray responseData = buildEchoDataCaptureResponse(*header,
                                              static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigReport),
                                              QJsonDocument(snapshot).toJson(QJsonDocument::Compact));
                    sendEchoDataCaptureResponse(*this, responseData);
                    return;
                }
                const QString requestId = requestObject.value(QStringLiteral("requestId")).toString();
                QString statusCode;
                QString statusMessage;
                QJsonArray fieldErrors;
                qint64 appliedRevision = gEchoDataCaptureRevision;
                {
                    QMutexLocker locker(&gEchoDataCaptureConfigMutex);
                    appliedRevision = gEchoDataCaptureRevision;
                    if(requestObject.value(QStringLiteral("commandType")).toString() != QStringLiteral("Apply"))
                    {
                        statusCode = QStringLiteral("invalid_command_type");
                        statusMessage = QStringLiteral("命令类型必须为 Apply");
                    }
                    else if(requestObject.value(QStringLiteral("configId")).toString() != kEchoDataCaptureConfigId)
                    {
                        statusCode = QStringLiteral("invalid_config_id");
                        statusMessage = QStringLiteral("配置对象标识不匹配");
                    }
                    else if(requestObject.value(QStringLiteral("baseRevision")).isDouble() == false ||
                            std::floor(requestObject.value(QStringLiteral("baseRevision")).toDouble()) != requestObject.value(QStringLiteral("baseRevision")).toDouble() ||
                            static_cast<qint64>(requestObject.value(QStringLiteral("baseRevision")).toDouble()) != gEchoDataCaptureRevision)
                    {
                        statusCode = QStringLiteral("version_conflict");
                        statusMessage = QStringLiteral("配置版本已变化，请重新查询后再下发");
                    }
                    else if(requestObject.value(QStringLiteral("snapshot")).isObject() == false ||
                            requestObject.value(QStringLiteral("snapshot")).toObject().value(QStringLiteral("settings")).isObject() == false)
                    {
                        statusCode = QStringLiteral("invalid_snapshot");
                        statusMessage = QStringLiteral("缺少完整配置快照");
                    }
                    else
                    {
                        const QJsonObject snapshotObject = requestObject.value(QStringLiteral("snapshot")).toObject();
                        if(snapshotObject.value(QStringLiteral("schemaVersion")).toInt() != kEchoDataCaptureSchemaVersion)
                        {
                            statusCode = QStringLiteral("unsupported_schema");
                            statusMessage = QStringLiteral("不支持的配置快照版本");
                        }
                        else if(snapshotObject.value(QStringLiteral("configId")).toString() != kEchoDataCaptureConfigId)
                        {
                            statusCode = QStringLiteral("invalid_snapshot");
                            statusMessage = QStringLiteral("配置快照对象标识不匹配");
                        }
                        else
                        {
                            DefEchoSaveConfig candidateConfig;
                            const DefEchoSaveConfig currentConfig = GlobalVal::GetInstance()->getRadarDataSaveInfo();
                            if(parseEchoDataCaptureSettings(snapshotObject.value(QStringLiteral("settings")).toObject(),
                                                            currentConfig,
                                                            candidateConfig,
                                                            fieldErrors) == false)
                            {
                                statusCode = QStringLiteral("validation_failed");
                                statusMessage = QStringLiteral("配置参数校验失败");
                            }
                            else
                            {
//                                const int persistResult = GlobalVal::GetInstance()->ApplyEchoDataCaptureConfig(candidateConfig);
//                                if(persistResult != 0)
//                                {
//                                    statusCode = QStringLiteral("persist_failed");
//                                    statusMessage = QStringLiteral("配置持久化失败，当前生效配置未改变");
//                                }
//                                else
//                                {
//                                    ++gEchoDataCaptureRevision;
//                                    gEchoDataCaptureUpdatedAtMs = QDateTime::currentMSecsSinceEpoch();
//                                    appliedRevision = gEchoDataCaptureRevision;
//                                    qDebug() << "回波数据录取配置已生效，版本" << appliedRevision;
//                                }
                                GlobalVal::GetInstance()->setRadarDataSaveInfo(candidateConfig);
                            }
                        }
                    }
                }
                const QJsonValue confirmStatus = statusCode.isEmpty() ? QJsonValue(0) : QJsonValue(statusCode);
                const QString confirmMessage = statusMessage.isEmpty() ? QStringLiteral("ok") : statusMessage;
                const QJsonObject confirmObject = buildEchoDataCaptureConfirm(requestId,
                                                  confirmStatus,
                                                  confirmMessage,
                                                  appliedRevision,
                                                  fieldErrors);
                QByteArray responseData = buildEchoDataCaptureResponse(*header,
                                          static_cast<uint16_t>(ProdectProtocolType::EchoDataCaptureConfigConfirm),
                                          QJsonDocument(confirmObject).toJson(QJsonDocument::Compact));
                sendEchoDataCaptureResponse(*this, responseData);
            }
        }
    }

    int64_t SignalCoreRGCommunicateCore::RFWriteDataPragram( char* data, int size, std::string theme )
    {
        if( this->mWriteDataPragram )
        {
            this->mWriteDataPragram(data, size, theme);
        }
        if(this->mSocketPub)
        {
            this->mSocketPub->writeDatagram(data, size, theme);
        }
        return 0x00;
    }


    bool SignalCoreRGCommunicateCore::variableApplication()
    {
        this->mSocketSendPoints = new ZeromqServer();
        this->mSocketSendPoints->connect("tcp://127.0.0.1:13426");
        if(GlobalVal::GetInstance()->getIsUsingWindows() == false &&
                GlobalVal::GetInstance()->getIsMainProgram() == true    )
        {
            mSocketSub = new ZeromqSocket( );
            string addr = QString("tcp://%0:%1").arg(GlobalVal::GetInstance()->dataSubAddr().IPStr()).arg( GlobalVal::GetInstance()->dataSubAddr().port()).toStdString();
            std::list<std::string> themes;
            for(auto v : GlobalVal::GetInstance()->dataSubThemeLst().array)
            {
                themes.push_back( v.topic.toStdString() );
            }
            mSocketSub->connect( addr, themes );
            //
            addr = QString("tcp://%0:%1").arg(GlobalVal::GetInstance()->dataPubAddr().IPStr()).arg( GlobalVal::GetInstance()->dataPubAddr().port()).toStdString();
            mSocketPub = new ZeromqServer();
            mSocketPub->connect(addr);
        }
        return true;
    }


    bool SignalCoreRGCommunicateCore::internalSignalConnect()
    {
        return true;
    }


    bool SignalCoreRGCommunicateCore::exec()
    {
        std::thread t( std::bind( &SignalCoreRGCommunicateCore::run, this ) );
        t.detach();
        return true;
    }


    void SignalCoreRGCommunicateCore::destroy()
    {
        this->isStop = true;
        while (this->mIsRuning)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
        this->mSocketSendPoints->stop();
    }


    void SignalCoreRGCommunicateCore::active()
    {
        this->variableApplication();
        this->internalSignalConnect();
        this->exec();
    }


    void SignalCoreRGCommunicateCore::run()
    {
        this->isStop = false;
        this->mIsRuning = true;
        while (this->isStop == false)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
            static int64_t time4SendHeartInfo = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
            if( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - time4SendHeartInfo >= 10000 )
            {
                SEND_RADAR_HEARTBEAT_V5();
                time4SendHeartInfo = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
            }
            if(this->mSocketSub != nullptr)
            {
                while(this->mSocketSub->pendingDatagramSize())
                {
                    shared_ptr<ZmqByte> data_recv;
                    if(this->mSocketSub->readDatagram(data_recv) == 0x00)
                    {
                        this->RFReadDataPragram(data_recv->data(), data_recv->size(), data_recv->getTheme());
                    }
                }
            }
        }
//        QString headrt = "心跳发送";
        this->mIsRuning = false;
//        this->mSocketSendPoints->writeDatagram( headrt.toUtf8().data(), headrt.toUtf8().size(), "heart" );
    }

    bool SignalCoreRGCommunicateCore::RFWaveInfoCheck(DefWaveInfo& waveInfo)
    {
        Common::WaveBaseInfo info;
        info.is_mix_wave = (waveInfo.mPulseWidthMix > 1e-6);
        info.pw_us = waveInfo.pulseWidthUs();
        info.prt_us = waveInfo.pRTUs();
        info.pulseNbr = waveInfo.pulseAmount();
        info.sampleNbr = waveInfo.sampleNbr();
        info.sampleFreqMHz = waveInfo.sampleFreqHz() / 1e6;
        info.waveBandMHz = waveInfo.bandwidthHz() / 1e6;
        bool flg =  GlobalVal::GetInstance()->isExistWaveBaseInfo( info );
        if( flg == false )
        {
            GlobalVal::GetInstance()->appendWaveInfo(info);
            qDebug() << "重载波形;";
            emit GlobalSignal::GetInstance()->SignalWaveScheduleReload();
            return false;
        }
        return true;
    }

    void SignalCoreRGCommunicateCore::RFSendMicroDopplerData(DefMicroDopplerMatrix& dopCls)
    {
    }

    int SignalCoreRGCommunicateCore::RFSendPoints(shared_ptr<RawData2SP> echoInfo,
            QList<IRadarV51::SignalProcPoint>& pointsFrmSP,
            bool angleIsDeg)
    {
        if( this->isStop == true)
        {
            return 0x01;
        }
        QByteArray data;
        //插入雷达类型和雷达序号
        DefSPPointsTrancPre pre;
        pre.radarType = echoInfo->head->radarType;
        pre.radarIdx = echoInfo->head->radarIndex;
        pre.freq = echoInfo->head->mWaveInfo.mRadarFreq;
        pre.atti_diff = echoInfo->head->mArrayAttitude;
        pre.atti_gd = echoInfo->head->mInertialNavAttitude;
        pre.trkingInfo = echoInfo->head->mTrkingInfo;
        data.append( reinterpret_cast<char*>(&pre), sizeof(DefSPPointsTrancPre) );
        //
        IRadarV51::SignalProcPointPackageWithHeader pointPkg;
        //将无人机信息整理为信号处理点
//        if(echoInfo->head->mDronInfo.Purpose == 0x13 &&  pointsFrmSP.size())
//        {
//            if(echoInfo->head->mDronInfo.Range < 10000 && echoInfo->head->mDronInfo.Range > 80)
//            {
//                IRadarV51::SignalProcPoint pnt = pointsFrmSP.first();
//                pnt.range_array = int(echoInfo->head->mDronInfo.Range / 3) * 3;
//                pnt.azimuth_station = echoInfo->head->mDronInfo.Azi + GaussianDist(0, deg2rad(0.1));
//                pnt.elevation_station = echoInfo->head->mDronInfo.Ele + GaussianDist(0, deg2rad(0.1));
//                pnt.cycleIndex = echoInfo->head->mCurrentCycle;
//                pnt.beamPosIndex = echoInfo->head->mBeamCode;
//                pnt.azi_resolution = echoInfo->head->mWaveInfo.aziPrecisionRad();
//                pnt.ele_resolution = echoInfo->head->mWaveInfo.elePrecisionRad();
//                pnt.dop_resolution = echoInfo->head->mWaveInfo.mVecQuanti;
//                pnt.rng_resolution = echoInfo->head->mWaveInfo.mRngQuanti;
//                pnt.radarId = echoInfo->head->radarIndex;
//                pnt.arrayId = echoInfo->head->radarIndex;
//                pnt.amplitude = 1000;
//                pnt.mean_row = 1e4;
//                pnt.mean_col = 1e4;
//                pnt.ampStaticClutter = 1e2;
//                pnt.ampDynamicClutter = 1e2;
//                pnt.amplitude = 1e8;
//                pnt.RCS = MeanDist(0.001, 0.1);
//                pnt.coordFlags = 0xff;
//                pointPkg.payload.points.push_back(pnt);
//            }
//        }
        for(auto v : pointsFrmSP)
        {
            if( angleIsDeg  )
            {
                v.azimuth_array = v.azimuth_array * M_PI / 180.0;
                v.elevation_array = v.elevation_array * M_PI / 180.0;
                v.beamAzimuth = deg2rad(v.beamAzimuth);
                v.beamElevation = deg2rad(v.beamElevation);
            }
//            将阵面坐标系旋转为站心坐标系
            PointRAE arrayRae(1, v.azimuth_array, v.elevation_array);
//            qDebug() << echoInfo->head->mArrayAttitude.toStr() << echoInfo->head->mInertialNavAttitude.toStr();
            auto station_xyz = Array2Station(  Array2Station( RAE2xyz(arrayRae), Attitude(echoInfo->head->mArrayAttitude) * -1 )
                                               , echoInfo->head->mInertialNavAttitude * -1);
            auto station_rae = xyz2RAE( station_xyz);
            v.azimuth_station = station_rae.A;
            v.elevation_station = station_rae.E;
            v.range_station = v.range_array;//TODO
            v.cycleIndex = echoInfo->head->mCurrentCycle;
            v.beamPosIndex = echoInfo->head->mBeamCode;
            //修正跟踪搜索标志
            if( echoInfo->head->mTrkingInfo.Purpose == 0x11 )
            {
                v.pointType = echoInfo->head->mTrkingInfo.Purpose;
                v.trackId = echoInfo->head->mTrkingInfo.TrackNumber ;
            }
            else if( echoInfo->head->mTrkingInfo.Purpose == 0x42 )
            {
                v.pointType = echoInfo->head->mTrkingInfo.Purpose;
                v.trackId = echoInfo->head->mTrkingInfo.TrackNumber ;
            }
            /************************************|精度  |************************************/
            //精度
            v.azi_resolution = echoInfo->head->mWaveInfo.aziPrecisionRad();
            v.ele_resolution = echoInfo->head->mWaveInfo.elePrecisionRad();
            v.dop_resolution = echoInfo->head->mWaveInfo.mVecQuanti;
            v.rng_resolution = echoInfo->head->mWaveInfo.mRngQuanti;
            v.radarId = echoInfo->head->radarIndex;
            v.arrayId = echoInfo->head->radarIndex;
//            static int saveEchoIdx = 0;
//            SaveData(reinterpret_cast<char*>(v.mtdEcho.data()), sizeof(float)*v.mtdEcho.size(), QString("./log/row/mtd_echo_%0.dat").arg(saveEchoIdx++));
            /************************************| |************************************/
            pointPkg.payload.points.push_back(v);
        }
        bool is_send = false;
        auto dataSpPkg = pointPkg.data();
        IRadarV51::SignalProcPointPackageWithHeader test;
        int flg = test.fromData( dataSpPkg.data(), dataSpPkg.size() );
        if( flg != 0x00 )
        {
            qDebug() << "数据解析识别" << QString::number( flg, 16);
        }
        else
        {
        }
        data.append( dataSpPkg.data(), dataSpPkg.size());
//        qDebug()<<"SP POINTS";
        this->mSocketSendPoints->writeDatagram( data.data(), data.size(), "LocalPointsSend");
        is_send = true;
        if(is_send == false)
        {
            qDebug() << "发送失败";
            return 0x02;
        }
        return 0x00;
    }

    void SignalCoreRGCommunicateCore::setWriteDataPragram(const function<int64 (char* data, int size, std::string theme)>& writeDataPragram)
    {
        mWriteDataPragram = writeDataPragram;
    }

}
