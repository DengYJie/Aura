#pragma once

#include <QColor>
#include <QString>

namespace domain::model {

struct Conversation {
    QString id;
    QString name;
    QString lastMessage;
    QString time;
    QString glyph;
    QColor avatarBg;
    int unreadCount = 0;

    bool operator==(const Conversation& other) const
    {
        return id == other.id &&
               name == other.name &&
               lastMessage == other.lastMessage &&
               time == other.time &&
               glyph == other.glyph &&
               avatarBg == other.avatarBg &&
               unreadCount == other.unreadCount;
    }
};

}  // namespace domain::model
