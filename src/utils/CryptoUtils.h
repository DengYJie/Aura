#pragma once

#include <QString>
#include <QByteArray>

class CryptoUtils {
public:
    // 简单的对称加密模拟 (XOR + Base64)
    static QString encrypt(const QString& plaintext, const QString& key = "aura_secret_key");
    static QString decrypt(const QString& ciphertext, const QString& key = "aura_secret_key");
};
