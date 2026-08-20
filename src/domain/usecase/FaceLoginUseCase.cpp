#include "FaceLoginUseCase.h"
#include "data/local/QtCameraFrameProvider.h"

#include <QDebug>
#include <QJsonObject>
#include <chrono>
#include <future>
#include <utility>

FaceLoginUseCase::FaceLoginUseCase(std::shared_ptr<FaceAuthRepository> faceRepository,
                                   std::shared_ptr<UserRepository> userRepository,
                                   std::shared_ptr<AuditService> auditService)
    : m_faceRepository(std::move(faceRepository)),
      m_userRepository(std::move(userRepository)),
      m_auditService(std::move(auditService)),
      m_cameraProvider(std::make_unique<QtCameraFrameProvider>()) {}

FaceLoginUseCase::~FaceLoginUseCase() {
    stopFaceScan();
}

bool FaceLoginUseCase::startFaceScan(int cameraIndex) {
    if (m_running) {
        stopFaceScan();
    }

    m_running = true;
    m_isProcessingFrame = false;
    m_lastAuthSuccess = false;
    m_lastFaceBox = cv::Rect();
    m_lastMatchedUid = -1;
    m_traceId = AuditService::generateTraceId();
    m_lastAuditEventType.clear();
    m_lastAuditTime = QDateTime();

    auto startResult = m_cameraProvider->start(
        cameraIndex,
        [this](const QImage& previewFrame, cv::Mat inferenceFrame) {
            if (!m_running) {
                return;
            }

            if (m_frameCallback) {
                m_frameCallback(previewFrame);
            }

            if (!m_isProcessingFrame.exchange(true)) {
                if (m_frameThread.joinable()) {
                    m_frameThread.join();
                }
                m_frameThread = std::jthread([this, frame = std::move(inferenceFrame)](std::stop_token) mutable {
                    processFrameAsync(std::move(frame));
                });
            }
        },
        [this](const QString& message) {
            m_running = false;
            m_isProcessingFrame = false;
            logFaceEvent(QString::fromLatin1(audit::EventAuthFaceEngineError),
                         QString::fromLatin1(audit::LevelCritical),
                         QString::fromLatin1(audit::ResultError),
                         QString::fromLatin1(audit::ReasonModelError),
                         message.isEmpty() ? QStringLiteral("人脸登录失败：摄像头异常") : message);
            if (m_authCallback) {
                m_authCallback(AuthResult::Error, -1, "", QStringLiteral("无法打开摄像头，请检查设备权限或稍后重试"));
            }
        });

    if (!startResult.has_value()) {
        m_running = false;
        return false;
    }

    return true;
}

void FaceLoginUseCase::stopFaceScan() {
    m_running = false;
    if (m_cameraProvider) {
        m_cameraProvider->stop();
    }
    waitForFrameProcessing();
    if (m_frameThread.joinable()) {
        m_frameThread.request_stop();
        if (m_frameThread.get_id() != std::this_thread::get_id()) {
            m_frameThread.join();
        }
    }
}

void FaceLoginUseCase::waitForFrameProcessing() {
    for (int i = 0; i < 100 && m_isProcessingFrame.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

float FaceLoginUseCase::calculateIoU(const cv::Rect& box1, const cv::Rect& box2) const {
    cv::Rect inter = box1 & box2;
    int interArea = inter.area();
    int unionArea = box1.area() + box2.area() - interArea;
    if (unionArea <= 0) return 0.0f;
    return static_cast<float>(interArea) / static_cast<float>(unionArea);
}

void FaceLoginUseCase::processFrameAsync(cv::Mat frame) {
    if (!m_running || !m_faceRepository) {
        m_isProcessingFrame = false;
        return;
    }

    m_faceRepository->detectFaceAsync(frame, [this, frame](std::expected<FaceDetectionResult, AuthError> detectRes) {
        if (!m_running) {
            m_isProcessingFrame = false;
            return;
        }
        if (!detectRes.has_value()) {
            m_lastAuthSuccess = false;
            m_lastFaceBox = cv::Rect();
            AuthError err = detectRes.error();
            if (m_authCallback) {
                switch (err) {
                    case AuthError::NoFace:
                        logFaceEvent(QString::fromLatin1(audit::EventAuthFaceFailed),
                                     QString::fromLatin1(audit::LevelWarn),
                                     QString::fromLatin1(audit::ResultFailed),
                                     QString::fromLatin1(audit::ReasonNoFace),
                                     QStringLiteral("人脸登录失败：未检测到人脸"));
                        m_authCallback(AuthResult::Failed, -1, "", QStringLiteral("未检测到人脸"));
                        break;
                    case AuthError::MultipleFaces:
                        logFaceEvent(QString::fromLatin1(audit::EventAuthFaceFailed),
                                     QString::fromLatin1(audit::LevelWarn),
                                     QString::fromLatin1(audit::ResultFailed),
                                     QString::fromLatin1(audit::ReasonMultipleFaces),
                                     QStringLiteral("人脸登录失败：检测到多张人脸"));
                        m_authCallback(AuthResult::Failed, -1, "", QStringLiteral("检测到多张人脸，请保持单人在框内"));
                        break;
                    case AuthError::ModelError:
                        logFaceEvent(QString::fromLatin1(audit::EventAuthFaceEngineError),
                                     QString::fromLatin1(audit::LevelCritical),
                                     QString::fromLatin1(audit::ResultError),
                                     QString::fromLatin1(audit::ReasonModelError),
                                     QStringLiteral("人脸登录失败：检测模型未就绪"));
                        m_authCallback(AuthResult::Error, -1, "", QStringLiteral("人脸识别暂不可用，请稍后重试或使用密码登录"));
                        m_running = false;
                        break;
                    default:
                        break;
                }
            }
            m_isProcessingFrame = false;
            return;
        }

        FaceBox faceBox = detectRes.value().face;

        if (!m_lastFaceBox.empty()) {
            float iou = calculateIoU(faceBox.box, m_lastFaceBox);
            if (iou >= m_iouThreshold) {
                m_lastFaceBox = faceBox.box;
                if (m_lastAuthSuccess && m_lastMatchedUid != -1) {
                    if (m_authCallback) {
                        m_authCallback(AuthResult::Verifying, -1, "", QStringLiteral("正在确认身份..."));
                    }
                    m_userRepository->getUserByIdAsync(m_lastMatchedUid, [this](std::optional<User> userOpt) {
                        if (userOpt && m_authCallback) {
                            m_authCallback(AuthResult::Success, userOpt->uid(), userOpt->username(), QStringLiteral("识别成功，欢迎回来！"));
                        }
                    });
                }
                m_isProcessingFrame = false;
                return;
            }
        }

        m_lastFaceBox = faceBox.box;

        if (m_authCallback) {
            m_authCallback(AuthResult::Verifying, -1, "", QStringLiteral("正在确认是否为本人..."));
        }

        m_faceRepository->checkLivenessAsync(frame, faceBox, m_livenessThreshold,
            [this, frame, faceBox](std::expected<float, AuthError> livenessRes) {
                if (!m_running) {
                    m_isProcessingFrame = false;
                    return;
                }
                if (!livenessRes.has_value()) {
                    m_lastAuthSuccess = false;
                    logFaceEvent(QString::fromLatin1(audit::EventAuthFaceEngineError),
                                 QString::fromLatin1(audit::LevelCritical),
                                 QString::fromLatin1(audit::ResultError),
                                 QString::fromLatin1(audit::ReasonModelError),
                                 QStringLiteral("人脸登录失败：活体检测引擎异常"));
                    if (m_authCallback) {
                        m_authCallback(AuthResult::Error, -1, "", QStringLiteral("人脸识别暂不可用，请稍后重试或使用密码登录"));
                    }
                    m_isProcessingFrame = false;
                    return;
                }

                float livenessScore = livenessRes.value();
                if (livenessScore < m_livenessThreshold) {
                    m_lastAuthSuccess = false;
                    logFaceEvent(QString::fromLatin1(audit::EventAuthFaceSpoofBlocked),
                                 QString::fromLatin1(audit::LevelWarn),
                                 QString::fromLatin1(audit::ResultBlocked),
                                 QString::fromLatin1(audit::ReasonLowLiveness),
                                 QStringLiteral("人脸登录被拦截：活体分数过低"),
                                 std::nullopt,
                                 livenessScore);
                    if (m_authCallback) {
                        m_authCallback(AuthResult::SpoofingDetected, -1, "",
                                       QStringLiteral("当前人脸验证未通过，请使用本人面部重试"));
                    }
                    m_isProcessingFrame = false;
                    return;
                }

                cv::Mat alignedFace;
                try {
                    alignedFace = frame(faceBox.box).clone();
                } catch (...) {
                    m_isProcessingFrame = false;
                    return;
                }

                m_faceRepository->extractFeatureAsync(alignedFace,
                    [this, faceBox](std::expected<std::vector<float>, AuthError> featureRes) {
                        if (!m_running) {
                            m_isProcessingFrame = false;
                            return;
                        }
                        if (!featureRes.has_value()) {
                            m_lastAuthSuccess = false;
                            logFaceEvent(QString::fromLatin1(audit::EventAuthFaceEngineError),
                                         QString::fromLatin1(audit::LevelCritical),
                                         QString::fromLatin1(audit::ResultError),
                                         QString::fromLatin1(audit::ReasonModelError),
                                         QStringLiteral("人脸登录失败：特征提取模型异常"));
                            if (m_authCallback) {
                                m_authCallback(AuthResult::Error, -1, "", QStringLiteral("人脸识别暂不可用，请稍后重试或使用密码登录"));
                            }
                            m_isProcessingFrame = false;
                            return;
                        }

                        std::vector<float> feature = featureRes.value();

                        m_faceRepository->matchFeatureAsync(feature, m_similarityThreshold,
                            [this, faceBox](bool isMatched, int matchedUid) {
                                if (!m_running) {
                                    m_isProcessingFrame = false;
                                    return;
                                }
                                if (isMatched) {
                                    m_userRepository->getUserByIdAsync(matchedUid, [this, faceBox, matchedUid](std::optional<User> userOpt) {
                                        if (!m_running) {
                                            m_isProcessingFrame = false;
                                            return;
                                        }
                                        if (userOpt) {
                                            m_lastAuthSuccess = true;
                                            m_lastFaceBox = faceBox.box;
                                            m_lastMatchedUid = matchedUid;
                                            logFaceEvent(QString::fromLatin1(audit::EventAuthFaceSuccess),
                                                         QString::fromLatin1(audit::LevelInfo),
                                                         QString::fromLatin1(audit::ResultSuccess),
                                                         QString(),
                                                         QStringLiteral("人脸登录成功"),
                                                         matchedUid);
                                            if (m_authCallback) {
                                                m_authCallback(AuthResult::Success, userOpt->uid(), userOpt->username(),
                                                               QStringLiteral("识别成功，欢迎回来！"));
                                            }
                                        }
                                        m_isProcessingFrame = false;
                                    });
                                } else {
                                    m_lastAuthSuccess = false;
                                    m_lastMatchedUid = -1;
                                    logFaceEvent(QString::fromLatin1(audit::EventAuthFaceUnrecognized),
                                                 QString::fromLatin1(audit::LevelWarn),
                                                 QString::fromLatin1(audit::ResultFailed),
                                                 QString::fromLatin1(audit::ReasonNoMatch),
                                                 QStringLiteral("人脸登录失败：未匹配到注册特征"));
                                    if (m_authCallback) {
                                        m_authCallback(AuthResult::Unrecognized, -1, "",
                                                       QStringLiteral("人脸不匹配，请重试或使用密码登录"));
                                    }
                                    m_isProcessingFrame = false;
                                }
                            });
                    });
            });
    });
}

bool FaceLoginUseCase::hasUserFace(const QString& account) const {
    if (!m_userRepository) return false;
    std::promise<bool> promise;
    m_userRepository->hasUserFaceAsync(account, [&promise](bool result) {
        promise.set_value(result);
    });
    return promise.get_future().get();
}

bool FaceLoginUseCase::hasUserFace(int uid) const {
    if (!m_userRepository) return false;
    std::promise<bool> promise;
    m_userRepository->hasUserFaceAsync(uid, [&promise](bool result) {
        promise.set_value(result);
    });
    return promise.get_future().get();
}

void FaceLoginUseCase::logFaceEvent(const QString& eventType,
                                    const QString& level,
                                    const QString& result,
                                    const QString& reasonCode,
                                    const QString& message,
                                    std::optional<int> uid,
                                    std::optional<float> riskScore) {
    if (!m_auditService) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    if (eventType == m_lastAuditEventType && m_lastAuditTime.isValid() &&
        m_lastAuditTime.msecsTo(now) < 2000) {
        return;
    }
    m_lastAuditEventType = eventType;
    m_lastAuditTime = now;

    auto context = AuditService::makeContext(QStringLiteral("auth"), QStringLiteral("FaceLoginUseCase"), uid);
    context.traceId = m_traceId;

    audit::AuditEvent event;
    event.eventType = eventType;
    event.eventLevel = level;
    event.eventResult = result;
    event.reasonCode = reasonCode;
    event.message = message;
    event.riskScore = riskScore;
    event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("face")}};
    m_auditService->logAsync(context, event);
}
