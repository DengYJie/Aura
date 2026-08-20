#include "TranslateUseCase.h"

namespace domain::usecase {

TranslateUseCase::TranslateUseCase(std::shared_ptr<repository::ITranslationRepository> repository)
    : m_repository(std::move(repository))
{}

void TranslateUseCase::execute(const QString& sourceText,
                              const QString& targetLang,
                              ResultCallback callback) const
{
    if (!m_repository) {
        if (callback) {
            callback(sourceText, false);
        }
        return;
    }
    m_repository->translateText(sourceText, targetLang, std::move(callback));
}

}  // namespace domain::usecase
