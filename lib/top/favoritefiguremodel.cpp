#include "favoritefiguremodel.hpp"
#include "figure.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
FavoriteFigureModel::FavoriteFigureModel(QObject *parent) : QAbstractListModel(parent) {
  _registry = helpers::builtins_registry(false);
  auto _6e = helpers::book(6, &*_registry);
  static const std::vector<std::pair<const char *, const char *>> figs = {
      {"04", "20"}, {"04", "24"}, {"04", "26"}, {"04", "27"}};
  for (const auto &[ch, fig] : figs) {
    auto f = _6e->findFigure(QString::fromLatin1(ch), QString::fromLatin1(fig));
    if (f) _figures.append(f);
  }
}

int FavoriteFigureModel::rowCount(const QModelIndex &parent) const { return _figures.size(); }

QVariant FavoriteFigureModel::data(const QModelIndex &index, int role) const {
  using namespace Qt::StringLiterals;
  if (!index.isValid() || index.row() < 0 || index.row() >= _figures.size()) return {};
  auto fig = _figures.at(index.row());
  switch (role) {
  case (int)Roles::FigurePtrRole: return QVariant::fromValue(fig.get());
  case (int)Roles::NameRole: return u"%1.%2"_s.arg(fig->chapterName(), fig->figureName());
  case (int)Roles::TypeRole: return fig->defaultFragmentName();
  case (int)Roles::DescriptionRole: return fig->description();
  }
  return QVariant();
}

QHash<int, QByteArray> FavoriteFigureModel::roleNames() const {
  static auto base = QAbstractListModel::roleNames();
  base[(int)Roles::FigurePtrRole] = "figure";
  base[(int)Roles::NameRole] = "name";
  base[(int)Roles::TypeRole] = "type";
  base[(int)Roles::DescriptionRole] = "description";
  return base;
}
