#pragma once

#include <functional>
#include <QString>

namespace domain::repository {

class ITranslationRepository {
public:
    virtual ~ITranslationRepository() = default;

    using TranslationCallback = std::function<void(const QString& translatedText, bool isFinished)>;
    virtual void translateText(const QString& text,
                               const QString& targetLang,
                               TranslationCallback callback) = 0;
};

}  // namespace domain::repository
