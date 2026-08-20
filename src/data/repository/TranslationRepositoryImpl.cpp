#include "TranslationRepositoryImpl.h"

namespace data::repository {

TranslationRepositoryImpl::TranslationRepositoryImpl(std::shared_ptr<remote::RemoteTranslationDataSource> remoteDataSource)
    : m_remoteDataSource(std::move(remoteDataSource))
{}

void TranslationRepositoryImpl::translateText(const QString& text,
                                              const QString& targetLang,
                                              TranslationCallback callback)
{
    if (m_remoteDataSource) {
        m_remoteDataSource->translate(text, targetLang, std::move(callback));
    }
}

}  // namespace data::repository
