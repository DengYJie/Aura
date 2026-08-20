#pragma once

#include <functional>
#include <memory>
#include <QObject>
#include <QString>

namespace data::remote {

class RemoteAiDataSource;

class RemoteTranslationDataSource : public QObject {
    Q_OBJECT

public:
    explicit RemoteTranslationDataSource(std::shared_ptr<RemoteAiDataSource> remoteAiDataSource,
                                         QObject* parent = nullptr);

    using TranslateCallback = std::function<void(const QString& translated, bool isFinished)>;

    void translate(const QString& text,
                   const QString& targetLang,
                   TranslateCallback callback);

private:
    std::shared_ptr<RemoteAiDataSource> m_remoteAiDataSource;
};

}  // namespace data::remote
