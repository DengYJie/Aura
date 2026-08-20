#include "RemoteTranslationDataSource.h"
#include "RemoteAiDataSource.h"

namespace data::remote {

RemoteTranslationDataSource::RemoteTranslationDataSource(std::shared_ptr<RemoteAiDataSource> remoteAiDataSource,
                                                         QObject* parent)
    : QObject(parent)
    , m_remoteAiDataSource(std::move(remoteAiDataSource))
{}

void RemoteTranslationDataSource::translate(const QString& text,
                                            const QString& targetLang,
                                            TranslateCallback callback)
{
    if (!callback) {
        return;
    }

    if (text.trimmed().isEmpty()) {
        callback(QString(), true);
        return;
    }

    if (!m_remoteAiDataSource) {
        callback(QStringLiteral("翻译服务未初始化。"), true);
        return;
    }

    const QString prompt = QStringLiteral("Target Language: %1\n\nText:\n%2").arg(targetLang, text);

    m_remoteAiDataSource->streamGenerate(
        prompt,
        QString(), // Empty lets server route to the configured translation provider
        [callback](const QString& chunk, bool isFinished) {
            callback(chunk, isFinished);
        },
        QStringLiteral("translate")
    );
}

}  // namespace data::remote
