#include "preference.hpp"

Preference::Preference(const quint32 id, const QString name) :
    id_(id),
    name_(name)
{}


Preference::Preference(const quint32 id, const QString name, const quint32 parentId,
                       const QRgb foreground, const QRgb background,
                       const bool bold,const bool italics,
                       const bool underline, const bool strikethrough) :
    id_(id),
    name_(name),
    foreground_(foreground),
    background_(background)
{
    font_.setBold(bold);
    font_.setItalic(italics);
    font_.setUnderline(underline);
    font_.setStrikeOut(strikethrough);
}

