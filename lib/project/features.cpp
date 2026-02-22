#include "features.hpp"
#include "core/math/bitmanip/enums.hpp"

pepp::FeatureHelper::FeatureHelper(QObject *parent) : QObject(parent) {}

namespace {

static const char *ONE_BYTE = "OneByte";
static const char *TWO_BYTE = "TwoByte";
static const char *NO_OS = "NoOS";
QString to_string_helper(pepp::FeatureHelper::Features f) {
  QStringList en;
  using namespace bits;
  if (any(f & pepp::FeatureHelper::Features::OneByte)) en.append(ONE_BYTE);
  if (any(f & pepp::FeatureHelper::Features::TwoByte)) en.append(TWO_BYTE);
  if (any(f & pepp::FeatureHelper::Features::NoOS)) en.append(NO_OS);
  return en.join(", ");
}
} // namespace

QString pepp::FeatureHelper::string(Features f) const { return to_string_helper(f); }

QString pepp::featuresAsPrettyString(FeatureHelper::Features f) { return to_string_helper(f); }

pepp::Features pepp::parseFeatures(const QString &str) {
  using namespace bits;
  static const std::map<std::string, Features> featmap{
      {ONE_BYTE, Features::OneByte}, {TWO_BYTE, Features::TwoByte}, {NO_OS, Features::NoOS}};
  pepp::Features ret = pepp::FeatureHelper::Features::None;
  const auto splits = str.split(", ", Qt::SplitBehaviorFlags::SkipEmptyParts);
  for (const auto &split : splits) {
    if (auto it = featmap.find(split.toStdString()); it == featmap.end())
      qDebug() << "Ignoring invalid feature" << split;
    else ret |= it->second;
  }
  return ret;
}
