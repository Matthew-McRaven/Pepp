#pragma once

#include <QMetaType> // Required for Q_DECLARE_METATYPE
#include <QObject>
#include <QRect>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "diagramtype.hpp"

class DiagramProperty
{
    Q_GADGET

public:
    enum Role : quint32 {
        Name = Qt::DisplayRole,
        Id = Qt::UserRole + 1,
        ImageSource,
        Type,
        InputNo,
        OutputNo,

        Selected,
        Orientation,
        Rectangle,

        //  Indicates invalid state from parsing input files
        Invalid = 0xffffffff,
    };
    Q_ENUM(Role)
};

class DiagramProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(quint32 id READ id CONSTANT) // Read only
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString imageSource READ imageSource WRITE setImageSource NOTIFY imageChanged)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(quint16 inputNo READ inputNo WRITE setInputNo NOTIFY inputChanged)
    Q_PROPERTY(quint16 outputNo READ outputNo WRITE setOutputNo NOTIFY outputChanged)
    Q_PROPERTY(quint16 orientation READ orientation WRITE setOrientation NOTIFY imageChanged)

public:
    explicit DiagramProperties(QObject *parent = nullptr);

    QVariant get(int role) const;
    void set(int role, const QVariant &data);

    quint32 id() const { return _id; } //  Unique object id
    QString name() const { return _name; }
    QString imageSource() const { return _imageSrc; }
    int type() const { return static_cast<int>(_type); }
    quint16 inputNo() const { return _inputNo; }
    quint16 outputNo() const { return _outputNo; }
    QPixmap *image() const { return _pixMap; }

    void setName(const QString v);
    void setImageSource(const QString v);
    void setType(const int v);
    void setInputNo(const quint16 v);
    void setOutputNo(const quint16 v);

    //  Not added as Q_Property yet
    bool selected() const { return _isSelected; }
    void setSelected(const bool v);
    void setImage(QPixmap *v);

    int orientation() const { return _orientation; }
    void setOrientation(const quint32 v);

    QRect rectangle() const { return _rect; }
    void setRectangle(const QRect v);
    QRect gridRectangle() const { return _gridRect; }
    void setGridRectangle(const QRect v);

signals:
    void typeChanged();
    void nameChanged();
    void imageChanged();
    void inputChanged();
    void outputChanged();
    void selectedChanged();
    void dimensionsChanged();

private:
    quint32 _id;
    static quint32 _counter;
    quint32 _orientation = 0; // Pointing Left

    //  Selection logic
    bool _isSelected = false;

    //  Common properties
    QString _name;
    QString _imageSrc;

    //  Gate properties
    DiagramType::Type _type = DiagramType::Invalid;
    quint16 _inputNo{2};
    quint16 _outputNo{1};

    //  Line properties
    quint32 _start{0};
    quint32 _finish{0};

    //  Diagram grid dimensions & placement
    QRect _rect{};
    QRect _gridRect{};
    QPixmap *_pixMap = nullptr;
};
