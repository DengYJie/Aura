#pragma once

#include <QJsonObject>
#include <QString>

class AuditMetadataSanitizer {
 public:
  static QJsonObject sanitize(const QJsonObject& metadata);
  static QString sanitizeText(const QString& input);

 private:
  static QString maskEmail(const QString& email);
  static QString maskPhone(const QString& phone);
};
