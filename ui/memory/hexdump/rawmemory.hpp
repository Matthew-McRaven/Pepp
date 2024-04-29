#pragma once
#include "QtTypes"

#include <QObject>
#include <vector>

class ARawMemory : public QObject {
  Q_OBJECT
public:
  ARawMemory(QObject *parent = nullptr);
  virtual ~ARawMemory() = 0;
  virtual quint32 byteCount() const = 0;
  virtual quint8 read(quint32 address) const = 0;
  virtual void write(quint32 address, quint8 value) = 0;
  virtual void clear() = 0;
};

class EmptyRawMemory : public ARawMemory {
  Q_OBJECT
public:
  explicit EmptyRawMemory(quint32 size, QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;

private:
  quint32 _size;
};

class TableRawMemory : public ARawMemory {
  Q_OBJECT
public:
  explicit TableRawMemory(quint32 size, QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;

private:
  std::vector<quint8> _data;
};
