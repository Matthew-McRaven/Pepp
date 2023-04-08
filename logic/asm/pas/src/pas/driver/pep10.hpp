#pragma once
#include "./common.hpp"
namespace pas::driver::pep10 {
Q_NAMESPACE;
enum class Stage {
  Start,
  Parse,
  IncludeMacros,
  FlattenMacros,
  GroupNodes,
  RegisterExports,
  AssignAddresses,
  WholeProgramSanity,
  ExportToObject,
  End
};
Q_ENUM_NS(Stage);

class TransformParse : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class TransformIncludeMacros : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class TransformFlattenMacros : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

// Currently no-op
class TransformGroup : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};
class TransformRegisterExports : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};
class TransformAssignAddresses : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class TransformWholeProgramSanity : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

// No object representation (yet)
/*class TransformExportToObject : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>>) override;
  Stage toStage() override { return Stage::IncludeMacros; };
};*/

struct Features {
  bool isOS = false;
};

struct TargetDefinition {
  Features enabledFeatures = {};
  QString body;
  Stage to = Stage::Start;
};
QPair<QSharedPointer<Target<Stage>>, QList<QSharedPointer<Transform<Stage>>>>
pipeline(QString body, Features feats);

} // namespace pas::driver::pep10
