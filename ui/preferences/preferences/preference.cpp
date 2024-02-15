#include "preference.hpp"

Preference::Preference(const quint32 id, const QString name, const quint32 type,
                       const quint32 parentId, const QColor foreground, const QColor background,
                       const bool bold,const bool italics,const bool underline) :
    id_(id),
    name_(name),
    type_(type),
    foreground_(foreground),
    background_(background),
    bold_(bold),
    italics_(italics),
    underline_(underline)
{}

