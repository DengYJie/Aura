#pragma once

#include <QString>

class ImageUtils {
public:
    /**
     * Reads a local image file, scales it down to a reasonable size for chat (e.g., max 400x400),
     * and returns a data URI: data:image/jpeg;base64,XXXXXX
     */
    static QString imageToDataUri(const QString& filePath);
};
