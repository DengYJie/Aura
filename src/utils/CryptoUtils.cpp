#include "CryptoUtils.h"

QString CryptoUtils::encrypt(const QString& plaintext, const QString& key)
{
    if (plaintext.isEmpty()) return plaintext;
    
    QByteArray textBytes = plaintext.toUtf8();
    QByteArray keyBytes = key.toUtf8();
    
    QByteArray encrypted;
    for (int i = 0; i < textBytes.size(); ++i) {
        encrypted.append(textBytes[i] ^ keyBytes[i % keyBytes.size()]);
    }
    
    return QString::fromLatin1(encrypted.toBase64());
}

QString CryptoUtils::decrypt(const QString& ciphertext, const QString& key)
{
    if (ciphertext.isEmpty()) return ciphertext;
    
    QByteArray encryptedBytes = QByteArray::fromBase64(ciphertext.toLatin1());
    QByteArray keyBytes = key.toUtf8();
    
    QByteArray decrypted;
    for (int i = 0; i < encryptedBytes.size(); ++i) {
        decrypted.append(encryptedBytes[i] ^ keyBytes[i % keyBytes.size()]);
    }
    
    return QString::fromUtf8(decrypted);
}
