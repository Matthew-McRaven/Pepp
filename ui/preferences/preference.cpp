#include "preference.hpp"

Preference::Preference(const quint32 id, const QString name, const quint32 type) :
    id_(id),
    name_(name),
    type_(type)
{}


Preference::Preference(const quint32 id, const QString name, const quint32 type,
                       const quint32 parentId, const QRgb foreground, const QRgb background,
                       const bool bold,const bool italics,
                       const bool underline, const bool strikethrough) :
    id_(id),
    name_(name),
    type_(type),
    foreground_(foreground),
    background_(background)
{
    font_.setBold(bold);
    font_.setItalic(italics);
    font_.setUnderline(underline);
    font_.setStrikeOut(strikethrough);
}

