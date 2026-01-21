#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT
#include <QtQmlIntegration>

class DiagramProperty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(quint32 id READ id CONSTANT) // Read only
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString imageSource READ imageSource WRITE setImageSource NOTIFY imageChanged)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(quint16 inputNo READ inputNo WRITE setInputNo NOTIFY inputChanged)
    Q_PROPERTY(quint16 outputNo READ outputNo WRITE setOutputNo NOTIFY outputChanged)

public:
    enum class DiagramRole : quint32 {
        //  Gate enums
        ANDGateRole = 0,
        NANDGateRole,
        ORGateRole,
        NORGateRole,
        InverterGateRole,
        XORGateRole,
        XNORGateRole,
        DiagramTotal, // Must be last diagram

        //  Line enums
        LineRole,
        MultiLineRole,
        BusRole,

        Total, // Must be last valid role
        //  Indicates invalid state from parsing input files
        Invalid = 0xffffffff,
    };
    Q_ENUM(DiagramRole)
    explicit DiagramProperty(QObject *parent = nullptr);

    quint32 id() const { return _id; } //  Unique object id
    QString name() const { return _name; }
    QString imageSource() const { return _imageSrc; }
    int type() const { return static_cast<int>(_type); }
    quint16 inputNo() const { return _inputNo; }
    quint16 outputNo() const { return _outputNo; }

    void setName(const QString v) { _name = v; }
    void setImageSource(const QString v) { _imageSrc = v; }
    void setType(const int v) { _type = static_cast<DiagramRole>(v); }
    void setInputNo(const quint16 v) { _inputNo = v; }
    void setOutputNo(const quint16 v) { _outputNo = v; }

signals:
    void typeChanged();
    void nameChanged();
    void imageChanged();
    void inputChanged();
    void outputChanged();

private:
    quint32 _id;
    static quint32 _counter;

    //  Common properties
    QString _name;
    QString _imageSrc;

    //  Gate properties
    DiagramRole _type{DiagramRole::Invalid};
    quint16 _inputNo{2};
    quint16 _outputNo{1};

    //  Line properties
    quint32 _start{0};
    quint32 _finish{0};
};

class DiagramPropertyModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    QList<DiagramProperty *> _properties;

public:
    explicit DiagramPropertyModel(QObject *parent = nullptr);

    Q_INVOKABLE DiagramProperty *createDiagram();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    //bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Add data:
    //bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    //bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

signals:
    void newDiagramChanged();

private:
};
