#pragma once

#include <functional>
#include <QObject>
#include <QString>
#include <QHash>

namespace data::remote {

class RemoteAiDataSource : public QObject {
    Q_OBJECT
public:
    using StreamCallback = std::function<void(const QString& fullText, bool isFinished)>;

    explicit RemoteAiDataSource(QObject* parent = nullptr);

    void streamGenerate(const QString& prompt,
                        const QString& modelName,
                        StreamCallback callback,
                        const QString& taskType = QStringLiteral("chat"));

private:
    QString resolveTargetId(const QString& modelName) const;

    QHash<QString, StreamCallback> m_callbacks;
    QHash<QString, QString> m_accumulators;
};

}  // namespace data::remote
