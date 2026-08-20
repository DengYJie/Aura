#pragma once

#include <functional>
#include <memory>
#include <QString>

#include "../repository/ITranslationRepository.h"

namespace domain::usecase {

class TranslateUseCase {
public:
    explicit TranslateUseCase(std::shared_ptr<repository::ITranslationRepository> repository);

    using ResultCallback = std::function<void(const QString& translatedText, bool isFinished)>;
    void execute(const QString& sourceText,
                 const QString& targetLang,
                 ResultCallback callback) const;

private:
    std::shared_ptr<repository::ITranslationRepository> m_repository;
};

}  // namespace domain::usecase
