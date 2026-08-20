#include "AuditMetadataSanitizer.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

namespace {

QJsonValue sanitizeValue(const QString& key, const QJsonValue& value) {
  const QString normalizedKey = key.toLower();
  if (normalizedKey.contains(QStringLiteral("password")) ||
      normalizedKey.contains(QStringLiteral("face")) ||
      normalizedKey.contains(QStringLiteral("feature"))) {
    return QStringLiteral("[REDACTED]");
  }

  if (value.isString()) {
    return AuditMetadataSanitizer::sanitizeText(value.toString());
  }
  if (value.isObject()) {
    return AuditMetadataSanitizer::sanitize(value.toObject());
  }
  if (value.isArray()) {
    QJsonArray source = value.toArray();
    QJsonArray sanitized;
    for (const QJsonValue& item : source) {
      sanitized.push_back(sanitizeValue(key, item));
    }
    return sanitized;
  }
  return value;
}

}  // namespace

QJsonObject AuditMetadataSanitizer::sanitize(const QJsonObject& metadata) {
  QJsonObject result;
  for (auto it = metadata.begin(); it != metadata.end(); ++it) {
    result.insert(it.key(), sanitizeValue(it.key(), it.value()));
  }
  return result;
}

QString AuditMetadataSanitizer::sanitizeText(const QString& input) {
  QString output = input;
  output.replace(QRegularExpression(QStringLiteral(R"((\b\d{3})\d{4}(\d{4}\b))")), QStringLiteral("\\1****\\2"));

  const QRegularExpression emailRegex(QStringLiteral(R"(([A-Za-z0-9._%+-]{1,})@([A-Za-z0-9.-]+\.[A-Za-z]{2,}))"));
  QRegularExpressionMatchIterator it = emailRegex.globalMatch(output);
  while (it.hasNext()) {
    const auto match = it.next();
    output.replace(match.captured(0), maskEmail(match.captured(0)));
  }
  return output;
}

QString AuditMetadataSanitizer::maskEmail(const QString& email) {
  const int atIndex = email.indexOf('@');
  if (atIndex <= 1) {
    return QStringLiteral("***%1").arg(email.mid(atIndex));
  }
  return QStringLiteral("%1***%2").arg(email.left(1), email.mid(atIndex));
}

QString AuditMetadataSanitizer::maskPhone(const QString& phone) {
  if (phone.size() < 7) {
    return QStringLiteral("***");
  }
  return QStringLiteral("%1****%2").arg(phone.left(3), phone.right(4));
}
