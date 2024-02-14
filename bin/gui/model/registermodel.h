#ifndef REGISTERMODEL_H
#define REGISTERMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>

//  Register is a reserved name. Prepended Pep to avoid naming conflicts
class PepRegister {
    Q_GADGET
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(quint32 address READ address WRITE setAddress)
    Q_PROPERTY(QString data READ data WRITE setData)

    QString name_{};
    quint32 address_ = 0;
    QString data_{};

public:
    PepRegister() = default;
    PepRegister(const QString name, const quint32 address, const QString data);
    ~PepRegister()= default;

    //	Disallow copying
    PepRegister( const PepRegister& ) = default;
    PepRegister& operator=( const PepRegister& ) = default;
    //	Moving OK
    PepRegister( PepRegister&& ) noexcept = default;
    PepRegister& operator=( PepRegister&& ) = default;

    //  Getter & Setter
    QString name() const {return name_;}
    quint32 address() const {return address_;}
    QString data() const {return data_;}

    void setName( const QString name){ name_ = name;}
    void setAddress( const quint32 address){ address_ = address;}
    void setData( const QString data){ data_ = data;}
};

//  Read only class for change in Register values
class RegisterModel : public QAbstractListModel
{
    Q_OBJECT

public:
    // Define the role names to be used
    enum RoleNames {
        Name = Qt::UserRole + 1,
        Address,
        Data,
    };

    explicit RegisterModel(QObject *parent = nullptr);
    ~RegisterModel() = default;

    //  Data loaded on construction. This reloads
    void reload();

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Fetch data dynamically:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

public slots:
    void updateTestData();

protected:  //  Role Names must be under protected
    QHash<int, QByteArray> roleNames() const override;

private:
    void load();

    QHash<int, QByteArray>  roleNames_;
    QList<PepRegister>      registers_;
};

#endif // REGISTERMODEL_H
