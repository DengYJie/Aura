#include "FaceEnrollUseCase.h"
#include "data/local/QtCameraFrameProvider.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QMetaObject>
#include <chrono>
#include <utility>

FaceEnrollUseCase::FaceEnrollUseCase(std::shared_ptr<FaceAuthRepository> faceRepository,
                                     std::shared_ptr<UserRepository> userRepository,
                                     std::shared_ptr<AuditService> auditService)
    : m_faceRepository(std::move(faceRepository)),
      m_userRepository(std::move(userRepository)),
      m_auditService(std::move(auditService)),
      m_cameraProvider(std::make_unique<QtCameraFrameProvider>()) {}

FaceEnrollUseCase::~FaceEnrollUseCase() {
    stopFaceEnroll();
}

void FaceEnrollUseCase::runOnMainThread(std::function<void()> fn) {
    QMetaObject::invokeMethod(qApp, std::move(fn), Qt::QueuedConnection);
}

bool FaceEnrollUseCase::startFaceEnroll(int uid, int cameraIndex,
                                        std::function<void(bool, QString)> callback) {
    if (m_isEnrolling) return false; // 已在录入中
    m_traceId = AuditService::generateTraceId();
    if (uid <= 0) {
        logEnrollEvent(QString::fromLatin1(audit::EventFaceEnrollFailed),
                       QString::fromLatin1(audit::LevelWarn),
                       QString::fromLatin1(audit::ResultFailed),
                       QString::fromLatin1(audit::ReasonBadAccount),
                       QStringLiteral("人脸录入失败：用户 ID 无效"));
        callback(false, QStringLiteral("当前登录状态异常，请重新登录后再试"));
        return false;
    }

    m_isEnrolling = true;
    m_isProcessingFrame = false;
    m_activeUid = uid;
    m_resultCallback = std::move(callback);

    auto startResult = m_cameraProvider->start(
        cameraIndex,
        [this](const QImage& previewFrame, cv::Mat inferenceFrame) {
            if (!m_isEnrolling) {
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
            Q_UNUSED(message);
            finishEnroll(false, QStringLiteral("无法打开摄像头，请检查设备权限或稍后重试"));
        });

    if (!startResult.has_value()) {
        m_isEnrolling = false;
        m_isProcessingFrame = false;
        m_activeUid = -1;
        m_resultCallback = nullptr;
        logEnrollEvent(QString::fromLatin1(audit::EventFaceEnrollFailed),
                       QString::fromLatin1(audit::LevelWarn),
                       QString::fromLatin1(audit::ResultError),
                       QString::fromLatin1(audit::ReasonModelError),
                       QStringLiteral("人脸录入失败：摄像头启动失败"));
        return false;
    }

    m_timeoutThread = std::jthread([this](std::stop_token stopToken) {
        const auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        while (!stopToken.stop_requested() && m_isEnrolling) {
            if (std::chrono::steady_clock::now() >= timeout) {
                logEnrollEvent(QString::fromLatin1(audit::EventFaceEnrollTimeout),
                               QString::fromLatin1(audit::LevelWarn),
                               QString::fromLatin1(audit::ResultFailed),
                               QString::fromLatin1(audit::ReasonTimeout),
                               QStringLiteral("人脸录入超时"));
                finishEnroll(false, QStringLiteral("录入超时，请重试"));
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    return true;
}

void FaceEnrollUseCase::stopFaceEnroll() {
    m_isEnrolling = false;

    if (m_cameraProvider) {
        m_cameraProvider->stop();
    }

    if (m_timeoutThread.joinable()) {
        m_timeoutThread.request_stop();
        if (m_timeoutThread.get_id() != std::this_thread::get_id()) {
            m_timeoutThread.join();
        }
    }

    waitForFrameProcessing();
    if (m_frameThread.joinable()) {
        m_frameThread.request_stop();
        if (m_frameThread.get_id() != std::this_thread::get_id()) {
            m_frameThread.join();
        }
    }

    m_resultCallback = nullptr;
    m_activeUid = -1;
}

void FaceEnrollUseCase::waitForFrameProcessing() {
    for (int i = 0; i < 100 && m_isProcessingFrame.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void FaceEnrollUseCase::processFrameAsync(cv::Mat frame) {
    if (!m_isEnrolling || !m_faceRepository) {
        m_isProcessingFrame = false;
        return;
    }

    m_faceRepository->detectFaceAsync(frame, [this, frame](std::expected<FaceDetectionResult, AuthError> detectRes) {
        if (!m_isEnrolling) {
            m_isProcessingFrame = false;
            return;
        }
        if (!detectRes.has_value()) {
            m_isProcessingFrame = false;
            return;
        }

        FaceBox faceBox = detectRes.value().face;
        m_faceRepository->checkLivenessAsync(frame, faceBox, m_livenessThreshold,
            [this, frame, faceBox](std::expected<float, AuthError> livenessRes) {
                if (!m_isEnrolling) {
                    m_isProcessingFrame = false;
                    return;
                }
                if (!livenessRes.has_value() || livenessRes.value() < m_livenessThreshold) {
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
                    [this](std::expected<std::vector<float>, AuthError> featureRes) {
                        if (!m_isEnrolling) {
                            m_isProcessingFrame = false;
                            return;
                        }
                        if (!featureRes.has_value() || featureRes->empty()) {
                            m_isProcessingFrame = false;
                            return;
                        }

                        const int uid = m_activeUid;
                        m_userRepository->saveUserFaceFeatureAsync(uid, *featureRes,
                            [this](bool success) {
                                if (success) {
                                    finishEnroll(true, QStringLiteral("人脸录入成功"));
                                } else {
                                    logEnrollEvent(QString::fromLatin1(audit::EventFaceEnrollFailed),
                                                   QString::fromLatin1(audit::LevelWarn),
                                                   QString::fromLatin1(audit::ResultError),
                                                   QString::fromLatin1(audit::ReasonSaveFailed),
                                                   QStringLiteral("人脸录入失败：保存人脸特征失败"));
                                    finishEnroll(false, QStringLiteral("保存失败，请稍后重试"));
                                }
                            });
                    });
            });
    });
}

void FaceEnrollUseCase::finishEnroll(bool success, QString message) {
    if (!m_isEnrolling.exchange(false)) {
        return;
    }

    m_isProcessingFrame = false;

    if (m_cameraProvider) {
        m_cameraProvider->stop();
    }

    if (m_timeoutThread.joinable()) {
        m_timeoutThread.request_stop();
        if (m_timeoutThread.get_id() != std::this_thread::get_id()) {
            m_timeoutThread.join();
        }
    }

    auto callback = std::move(m_resultCallback);
    m_resultCallback = nullptr;
    m_activeUid = -1;

    if (callback) {
        runOnMainThread([callback = std::move(callback), success, message = std::move(message)]() mutable {
            callback(success, std::move(message));
        });
    }

    if (success) {
        logEnrollEvent(QString::fromLatin1(audit::EventFaceEnrollSuccess),
                       QString::fromLatin1(audit::LevelInfo),
                       QString::fromLatin1(audit::ResultSuccess),
                       QString(),
                       QStringLiteral("人脸录入成功"));
    }
}

void FaceEnrollUseCase::logEnrollEvent(const QString& eventType,
                                       const QString& level,
                                       const QString& result,
                                       const QString& reasonCode,
                                       const QString& message) {
    if (!m_auditService) {
        return;
    }
    auto context = AuditService::makeContext(QStringLiteral("auth"), QStringLiteral("FaceEnrollUseCase"));
    context.traceId = m_traceId;
    if (m_activeUid > 0) {
        context.uid = m_activeUid;
    }

    audit::AuditEvent event;
    event.eventType = eventType;
    event.eventLevel = level;
    event.eventResult = result;
    event.reasonCode = reasonCode;
    event.message = message;
    event.metadata = QJsonObject{{QStringLiteral("flow"), QStringLiteral("face_enroll")}};
    m_auditService->logAsync(context, event);
}
