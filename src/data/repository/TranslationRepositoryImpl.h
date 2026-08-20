#pragma once

#include <memory>

#include "domain/repository/ITranslationRepository.h"
#include "../remote/RemoteTranslationDataSource.h"

namespace data::repository {

class TranslationRepositoryImpl : public domain::repository::ITranslationRepository {
public:
    explicit TranslationRepositoryImpl(std::shared_ptr<remote::RemoteTranslationDataSource> remoteDataSource);

    void translateText(const QString& text,
                       const QString& targetLang,
                       TranslationCallback callback) override;

private:
    std::shared_ptr<remote::RemoteTranslationDataSource> m_remoteDataSource;
};

}  // namespace data::repository
