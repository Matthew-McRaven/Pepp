#ifndef PREFERENCE_HPP
#define PREFERENCE_HPP

#include <QObject>
#include <QColor>

#include "../preferences_global.hpp"

class PREFERENCE_EXPORT Preference
{
    Q_GADGET
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QColor foreground READ foreground WRITE setForeground)
    Q_PROPERTY(QColor background READ background WRITE setBackground)

    quint32 id_ = 0;
    quint32 parentId_ = 0;
    QString name_{};
    quint32 type_ = 0;
    QColor  foreground_{};
    QColor  background_{};
    bool    bold_ = false;
    bool    italics_ = false;
    bool    underline_ = false;

public:
    Preference() = default;
    ~Preference()= default;

    Preference(const quint32 id, const QString name, const quint32 type,
               const quint32 parentId, const QColor foreground, const QColor background,
               const bool bold = false, const bool italics = false, const bool underline = false);

    //	Disallow copying
    Preference( const Preference& ) = default;
    Preference& operator=( const Preference& ) = default;
    //	Moving OK
    Preference( Preference&& ) noexcept = default;
    Preference& operator=( Preference&& ) = default;

    //  Getter & Setter
    quint32 id() const          {return id_;}
    QString name() const        {return name_;}

    quint32 parentId() const    {return parentId_;}
    quint32 type() const        {return type_;}
    QColor  foreground() const  {return foreground_;}
    QColor  background() const  {return background_;}
    bool    bold() const        {return bold_;}
    bool    italics() const     {return italics_;}
    bool    underline() const   {return underline_;}

    void setParent( const quint32 parent){ parentId_ = parent;}
    void setForeground( const QColor foreground){ foreground_ = foreground;}
    void setBackground( const QColor background){ background_ = background;}
    void setBold( const bool bold){ bold_ = bold;}
    void setItalics( const bool italics){ italics_ = italics;}
    void setUnderline( const bool underline){ underline_ = underline;}

};

#endif // PREFERENCE_HPP
