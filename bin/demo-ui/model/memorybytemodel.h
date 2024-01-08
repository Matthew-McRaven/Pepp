#ifndef MEMORYBYTEMODEL_H
#define MEMORYBYTEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QSet>

class MemoryColumns : public QObject {
    Q_OBJECT

    Q_PROPERTY(quint8 LineNo        READ LineNo     CONSTANT)
    Q_PROPERTY(quint8 Border1       READ Border1    CONSTANT)
    Q_PROPERTY(quint8 Border2       READ Border2    CONSTANT)
    Q_PROPERTY(quint8 CellStart     READ CellStart  CONSTANT)
    Q_PROPERTY(quint8 CellEnd       READ CellEnd    CONSTANT)
    Q_PROPERTY(quint8 Ascii         READ Ascii      CONSTANT)

    const quint8 lineNo_    = 0;
    const quint8 border1_   = 1;
    const quint8 cellStart_ = 2;
    quint8       cellEnd_   = cellStart_ + 7;
    quint8       border2_   = cellEnd_ + 1;
    quint8       ascii_     = border2_ + 1;
    quint8       total_     = ascii_ + 1;

public: //  Required by QML

    //  QML Properties
    quint8 LineNo() const   {return lineNo_;};
    quint8 Border1() const  {return border1_;};
    quint8 Border2() const  {return border2_;};
    quint8 CellStart() const{return cellStart_;};
    quint8 CellEnd() const  {return cellEnd_;};
    quint8 Ascii() const    {return ascii_;};
    quint8 Total() const    {return total_;};

    //  If model changes width, adjust column numbers
    void setNumBytesPerLine(const quint8 bytesPerLine) {
        cellEnd_    = cellStart_ + bytesPerLine-1;
        border2_    = cellEnd_ + 1;
        ascii_      = border2_ + 1;
        total_      = ascii_ + 1;
    }
};

class MemoryByteModel : public QAbstractTableModel
{
    Q_OBJECT

    //  Statistics on memory size and layout
    const qint32 size_;    //  64K memory model for Pep, but can be up to 4gig
    quint8 width_   = 8;   //  Default to 8 columns
    qint32 height_  = 0;   //  Calculated at startup

    QHash<int, QByteArray>          roleNames_;

    std::unique_ptr<quint8[]>       newData_;
    std::unique_ptr<quint8[]>       oldData_;
    QSet<quint8>                    selected_;
    qint32                          editing_ = -1;
    std::unique_ptr<MemoryColumns>  column_;

    Q_PROPERTY(MemoryColumns* Column READ column CONSTANT)
    Q_PROPERTY(int BytesPerRow READ rowCount NOTIFY dimensionsChanged)
    Q_PROPERTY(int BytesPerColumn READ columnCount NOTIFY dimensionsChanged)

public:

    // Define the role names to be used
    enum RoleNames {
        Byte = Qt::UserRole + 1,
        Selected,
        Editing,
        TextColor,
        BackgroundColor,
        Border,
    };
    Q_ENUM(RoleNames)

    explicit MemoryByteModel(QObject *parent = nullptr,
                             const quint32 totalBytes = (1 << 16),
                             const quint8 bytesPerRow =8);
    ~MemoryByteModel() = default;

    //  Required for access in Qml
    MemoryColumns* column() {
        return column_.get();
    }

    //  Helper functions for other C++ functions to call and affect the
    //  memory model without knowledge of the model layout.
    quint8 readByte(quint32 address) const;
    void writeByte(quint32 address, quint8 value);

    // Set the number of bytes displayed per line.
    // Should be a power of 2 between [1-32].
    // A number that is not a power of two will be rounded to the nearest power,
    // and the number will be clamped to 32.
    void setNumBytesPerLine(const quint8 bytesPerLine);

    //  Functions required by the QML model
    //  Header: Set read column and row headers
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    //  Basic functionality: dimentions of table in QML view
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //  Read table data
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    //  Editable:
    //  Set data through model
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    //  Indicate what operations can occur on a given column or cell
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //  Custom functions available to Qml
    Q_INVOKABLE bool isEditMode() { return (editing_ != -1);};


    //  Functions are based on index in Qml.
    Q_INVOKABLE void clearSelected(const QModelIndex& index, const RoleNames type = Selected);
    Q_INVOKABLE QVariant setSelected(const QModelIndex& index,const RoleNames type = Selected);

    //Q_INVOKABLE QModelIndex modelCellIndex(const int offset) const;
    //Q_INVOKABLE QVariant selectedCell(const int offset, const RoleNames type = Selected) const;
    //Q_INVOKABLE QVariant setSelectedCell(const int offset,const RoleNames type = Selected);
    //Q_INVOKABLE void clearSelectedCell(const int offset, const RoleNames type = Selected);

signals:
    void dimensionsChanged();

public slots:
    //  For testing only: creates random data
    void updateTestData();

protected:  //  Role Names must be under protected
    //  Columns available to Qml model. Represents a translation from
    //  Name in bytearray and RoleNames enum. Enum is used as role in
    //  callbacks to model.
    QHash<int, QByteArray> roleNames() const override;

private:
    //  Conversion functions
    std::size_t memoryOffset(const QModelIndex &index) const;
    QModelIndex memoryIndex(std::size_t index);

    //  Return ascii text for row
    QString ascii(const int row) const;

    //  Clear model
    void clear();

    //  Private functions fo manipulate selection directly by model
    QVariant selected(const QModelIndex& index,const RoleNames type = Selected) const;
    //QVariant setSelected(const QModelIndex& index,const RoleNames type = Selected);
    //void clearSelected(const QModelIndex& index, const RoleNames type = Selected);

    //Likely to be deleted
    void clearSelectedCell(const int offset, const RoleNames type = Selected);
};

#endif // MEMORYBYTEMODEL_H
