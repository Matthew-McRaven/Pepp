#include "renderer.hpp"
#include <QPainter>
#include <QTransform>
#include "shapes_one.hpp"
#include "shapes_two.hpp"

QMLOverlay::QMLOverlay(QRect location, QObject *parent) : QObject(parent), _location(location) {}

std::vector<Item> one_byte_geom() {
  using namespace OneByteShapes;
  using T = TextRectItem;
  using A = ArrowItem;
  using L = LineItem;
  using P = PolygonItem;
  using R = RectItem;

  std::vector<Item> _geom;
  QColor black(0, 0, 0, 255);
  QColor white(255, 255, 255, 255);
  QColor red(255, 0, 0);
  QColor green(0, 255, 0);
  QColor blue(0, 0, 255);
  QColor lblue(231, 234, 255);

  // Clocks
  _geom.emplace_back(A{.geom = ck_mar});
  _geom.emplace_back(A{.geom = ck_mdr});
  _geom.emplace_back(A{.geom = ck_load});
  _geom.emplace_back(L{.geom = ck_n});
  _geom.emplace_back(L{.geom = ck_z});
  _geom.emplace_back(L{.geom = ck_v});
  _geom.emplace_back(L{.geom = ck_c});
  _geom.emplace_back(L{.geom = ck_s});
  _geom.emplace_back(L{.geom = ck_memread});
  _geom.emplace_back(L{.geom = ck_memwrite});

  // Control Wires
  _geom.emplace_back(L{.geom = sel_muxcs});
  _geom.emplace_back(A{.geom = sel_muxa});
  _geom.emplace_back(A{.geom = sel_muxc});
  _geom.emplace_back(A{.geom = sel_c});
  _geom.emplace_back(A{.geom = sel_b});
  _geom.emplace_back(A{.geom = sel_a});
  _geom.emplace_back(A{.geom = sel_alu});

  // Buses
  _geom.emplace_back(P{.geom = bus_b, .bg = black, .fg = red});
  _geom.emplace_back(P{.geom = bus_a, .bg = black, .fg = green});
  _geom.emplace_back(P{.geom = bus_addr, .bg = black, .fg = blue});
  _geom.emplace_back(P{.geom = bus_addr_to_ddr, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_data, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_data_to_mdrmux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdrmux_to_mdr, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdr_to_data, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mar_to_addr, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_alu_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_nzvc_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdr_to_amux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_amux_to_alu, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_c, .bg = black, .fg = white});

  // Combinatorial Outputs
  _geom.emplace_back(A{.geom = logic_alu_nzvc});
  _geom.emplace_back(A{.geom = logic_c_to_nzvc});
  _geom.emplace_back(A{.geom = logic_c_to_csmux});
  _geom.emplace_back(A{.geom = logic_s_to_csmux});
  _geom.emplace_back(A{.geom = logic_cin});
  _geom.emplace_back(A{.geom = logic_z_to_nzvc});
  _geom.emplace_back(A{.geom = logic_v_to_nzvc});
  _geom.emplace_back(A{.geom = logic_n_to_nzvc});
  _geom.emplace_back(A{.geom = logic_andz_to_z});

  // Multiplexers
  _geom.emplace_back(R{.geom = mux_a, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_a, .text = "AMux", .color = black});
  _geom.emplace_back(R{.geom = mux_c, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_c, .text = "CMux", .color = black});
  _geom.emplace_back(R{.geom = mux_mdr, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_mdr, .text = "MDRMux", .color = black});
  _geom.emplace_back(R{.geom = mux_cs, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_cs, .text = "CSMux", .color = black});
  _geom.emplace_back(R{.geom = mux_andz, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_andz, .text = "AndZ", .color = black});

  // Register outlines & registers
  _geom.emplace_back(R{.geom = reg_bit_n, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_bit_n, .text = "N", .color = black});
  _geom.emplace_back(R{.geom = reg_bit_c, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_bit_c, .text = "C", .color = black});
  _geom.emplace_back(R{.geom = reg_bit_v, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_bit_v, .text = "V", .color = black});
  _geom.emplace_back(R{.geom = reg_bit_z, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_bit_z, .text = "Z", .color = black});
  _geom.emplace_back(R{.geom = reg_bit_s, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_bit_s, .text = "S", .color = black});
  _geom.emplace_back(R{.geom = reg_byte_mdr, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_mdr, .text = "MDR", .color = black});
  _geom.emplace_back(R{.geom = reg_byte_marb, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_marb, .text = "MARB", .color = black});
  _geom.emplace_back(R{.geom = reg_byte_mara, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_mara, .text = "MARA", .color = black});

  // Other large polys
  _geom.emplace_back(R{.geom = poly_regbank, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = poly_alu, .bg = blue, .fg = lblue});
  _geom.emplace_back(T{.geom = label_alu, .text = "ALU", .color = black});
  _geom.emplace_back(T{.geom = label_alu_function, .text = "A + B", .color = black});
  _geom.emplace_back(L{.geom = poly_nzvc_join, .color = black});

  return _geom;
};

std::vector<QRect> one_byte_overlays() {
  using namespace OneByteShapes;
  std::vector<QRect> ret;
  ret.push_back(loadCkCheckbox);
  ret.push_back(cLineEdit);
  ret.push_back(bLineEdit);
  ret.push_back(aLineEdit);
  ret.push_back(ck_marCheckbox);
  ret.push_back(ck_mdrCheckbox);
  ret.push_back(aMuxTristateLabel);
  ret.push_back(cMuxTristateLabel);
  ret.push_back(ALULineEdit);
  ret.push_back(CSMuxTristateLabel);
  ret.push_back(SCkCheckBox);
  ret.push_back(CCkCheckBox);
  ret.push_back(VCkCheckBox);
  ret.push_back(AndZTristateLabel);
  ret.push_back(ZCkCheckBox);
  ret.push_back(NCkCheckBox);
  ret.push_back(MemWriteTristateLabel);
  ret.push_back(MemReadTristateLabel);
  return ret;
}

std::vector<Item> two_byte_geom() {
  using namespace TwoByteShapes;
  using T = TextRectItem;
  using A = ArrowItem;
  using L = LineItem;
  using P = PolygonItem;
  using R = RectItem;

  std::vector<Item> _geom;
  QColor black(0, 0, 0, 255);
  QColor white(255, 255, 255, 255);
  QColor red(255, 0, 0);
  QColor green(0, 255, 0);
  QColor blue(0, 0, 255);
  QColor pink(255, 0, 255);

  // Clocks
  _geom.emplace_back(A{.geom = ck_mar});
  _geom.emplace_back(A{.geom = ck_mdre});
  _geom.emplace_back(A{.geom = ck_mdro});
  _geom.emplace_back(A{.geom = ck_load});
  _geom.emplace_back(L{.geom = ck_n});
  _geom.emplace_back(L{.geom = ck_z});
  _geom.emplace_back(L{.geom = ck_v});
  _geom.emplace_back(L{.geom = ck_c});
  _geom.emplace_back(L{.geom = ck_s});
  _geom.emplace_back(L{.geom = ck_memread});
  _geom.emplace_back(L{.geom = ck_memwrite});

  // Control Wires
  _geom.emplace_back(L{.geom = sel_muxcs});
  _geom.emplace_back(A{.geom = sel_muxa});
  _geom.emplace_back(A{.geom = sel_muxc});
  _geom.emplace_back(A{.geom = sel_c});
  _geom.emplace_back(A{.geom = sel_b});
  _geom.emplace_back(A{.geom = sel_a});
  _geom.emplace_back(A{.geom = sel_alu});
  _geom.emplace_back(A{.geom = sel_mux_mdre});
  _geom.emplace_back(A{.geom = sel_mux_mdro});
  _geom.emplace_back(A{.geom = sel_muxeo});
  _geom.emplace_back(A{.geom = sel_mux_mar});

  // Buses
  _geom.emplace_back(P{.geom = bus_b, .bg = black, .fg = blue});
  _geom.emplace_back(P{.geom = bus_a, .bg = black, .fg = green});
  _geom.emplace_back(P{.geom = bus_addr, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_addr_to_ddr, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_data, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdremux_to_mdre, .bg = black, .fg = red});
  _geom.emplace_back(P{.geom = bus_mdromux_to_mdro, .bg = black, .fg = green});
  _geom.emplace_back(P{.geom = bus_data_to_mdremux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_data_to_mdromux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdre_to_data, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdro_to_data, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mdre_to_eomux, .bg = black, .fg = red});
  _geom.emplace_back(P{.geom = bus_mdro_to_eomux, .bg = black, .fg = green});
  _geom.emplace_back(P{.geom = bux_marmux_to_mara, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_marmux_to_marb, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_mar_to_addr, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_alu_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_nzvc_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_eomux_to_amux, .bg = black, .fg = red});
  _geom.emplace_back(P{.geom = bus_amux_to_alu, .bg = black, .fg = white});
  _geom.emplace_back(P{.geom = bus_c, .bg = black, .fg = pink});

  // Combinatorial Outputs
  _geom.emplace_back(A{.geom = logic_alu_nzvc});
  _geom.emplace_back(A{.geom = logic_c_to_nzvc});
  _geom.emplace_back(A{.geom = logic_c_to_csmux});
  _geom.emplace_back(A{.geom = logic_s_to_csmux});
  _geom.emplace_back(A{.geom = logic_cin});
  _geom.emplace_back(A{.geom = logic_z_to_nzvc});
  _geom.emplace_back(A{.geom = logic_v_to_nzvc});
  _geom.emplace_back(A{.geom = logic_n_to_nzvc});
  _geom.emplace_back(A{.geom = logic_andz_to_z});

  // Multiplexers
  _geom.emplace_back(R{.geom = mux_a, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = mux_c, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = mux_mdre, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = mux_mdro, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = mux_eo, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = mux_cs, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = mux_andz, .bg = black, .fg = black});

  // Register outlines & registers
  _geom.emplace_back(R{.geom = reg_bit_n, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_bit_c, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_bit_v, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_bit_z, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_bit_s, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_byte_mdre, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_byte_mdro, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_byte_marb, .bg = black, .fg = black});
  _geom.emplace_back(R{.geom = reg_byte_mara, .bg = black, .fg = black});

  // Other large polys
  _geom.emplace_back(P{.geom = poly_regbank, .bg = black, .fg = black});
  _geom.emplace_back(P{.geom = poly_alu});
  _geom.emplace_back(T{.geom = ALULabel, .color = black});
  _geom.emplace_back(P{.geom = poly_marmux, .bg = black, .fg = black});

  // Commented UI controls, like editors and labels.
  ///_geom.emplace_back(I{.geom = loadCkCheckbox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cLineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = bLineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = bLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aLineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ck_marCheckbox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ck_mdrCheckbox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = poly_marmux, .bg = black, .fg = black}); // Actually MARMUX
  ///_geom.emplace_back(I{.geom = MDRMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MDRMuxLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aMuxLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cMuxLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ALULineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ALULabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = label_alu, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = CSMuxLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = CSMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = SCkCheckBox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = CCkCheckBox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = VCkCheckBox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = AndZLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = AndZTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ZCkCheckBox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = NCkCheckBox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemWriteLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemWriteTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemReadLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemReadTristateLabel, .bg = black, .fg = black});
  ///
  return _geom;
};

CursedCPUCanvas::CursedCPUCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {
  _geom = one_byte_geom();
  for (const auto &geom : one_byte_overlays()) _overlays.push_back(new QMLOverlay(geom, this));

  auto svg_path = ":/qt/qml/CPUPaint/svg/arrow.svg";
  QImage svg_image(svg_path);
  _arrows[Qt::NoArrow] = QPixmap(QSize(0, 0));
  _arrows[Qt::LeftArrow] = QPixmap::fromImage(svg_image);
  _arrows[Qt::RightArrow] = QPixmap::fromImage(svg_image.flipped(Qt::Orientation::Horizontal));
  _arrows[Qt::UpArrow] = QPixmap::fromImage(svg_image.transformed(QTransform().rotate(90.0)));
  _arrows[Qt::DownArrow] = QPixmap::fromImage(svg_image.transformed(QTransform().rotate(270.0)));
}

CursedCPUCanvas::~CursedCPUCanvas() noexcept {
  for (auto &ptr : _overlays) delete ptr;
}

struct PaintDispatch {
  CursedCPUCanvas *canvas;
  QPainter *painter;
  QFont font{"Times New Roman", 14};
  void operator()(const LineItem &item) {
    painter->setPen(item.color);
    painter->drawLine(item.geom);
  }
  void operator()(const ArrowItem &item) {
    painter->setPen(item.color);
    const auto &arrow = item.geom;
    for (const QLine &line : arrow._lines) painter->drawLine(line);
    for (const Arrowhead &head : arrow._arrowheads) {
      painter->save();
      painter->translate(head.point);
      int orient = static_cast<int>(head.orient);
      painter->drawPixmap(0, 0, canvas->_arrows[orient]);
      painter->restore();
    }
  }
  void operator()(const RectItem &item) {
    painter->setBrush(item.fg);
    painter->setPen(item.bg);
    painter->drawRect(item.geom);
  }
  void operator()(const PolygonItem &item) {
    painter->setBrush(item.fg);
    painter->setPen(item.bg);
    painter->drawConvexPolygon(item.geom);
  }
  void operator()(const TextRectItem &item) {
    painter->setPen(item.color);
    painter->setFont(font);
    painter->drawText(item.geom, Qt::AlignCenter, item.text);
  }
};

void CursedCPUCanvas::paint(QPainter *painter) {
  PaintDispatch disp{.canvas = this, .painter = painter};
  for (const auto &item : _geom) std::visit(disp, item);
}
