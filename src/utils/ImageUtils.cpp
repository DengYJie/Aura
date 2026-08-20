#include "ImageUtils.h"
#include <QImage>
#include <QBuffer>

QString ImageUtils::imageToDataUri(const QString& filePath)
{
    QImage img(filePath);
    if (img.isNull()) {
        return QString();
    }
    
    // Scale down to max 300x300 while keeping aspect ratio to save bandwidth
    if (img.width() > 300 || img.height() > 300) {
        img = img.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    // Use JPEG for smaller size
    img.save(&buffer, "JPG", 80);
    
    return QStringLiteral("data:image/jpeg;base64,") + QString::fromLatin1(ba.toBase64());
}
