#include "cpu_canvas.hpp"
#include <QPainter>
#include <QTransform>
#include <QtQml/qqmlengine.h>
#include "qml_overlays.hpp"
#include "shapes_one.hpp"
#include "shapes_two.hpp"

namespace {
void add_regbank(std::vector<pepp::Item> &_geom) {
  using namespace pepp;
  using namespace OneByteShapes;
  using T = TextRectItem;
  using R = RectItem;
  static const QColor black(0, 0, 0, 255);
  static const QColor white(255, 255, 255, 255);
  static const QColor lblue(231, 234, 255);
  static const Qt::Alignment lalign = Qt::AlignLeft | Qt::AlignVCenter;
  static const Qt::Alignment ralign = Qt::AlignRight | Qt::AlignVCenter;

  // Register Bank
  _geom.emplace_back(R{.geom = poly_regbank, .bg = black, .fg = white});
  // Column 0
  _geom.emplace_back(R{.geom = reg_value_a_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_a_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_x_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_x_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_sp_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_sp_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_pc_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_pc_lo, .bg = black, .fg = lblue});
  // Column 1
  _geom.emplace_back(R{.geom = reg_value_is, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_os_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_os_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t1, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t2_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t2_lo, .bg = black, .fg = lblue});
  // Column 2
  _geom.emplace_back(R{.geom = reg_value_t3_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t3_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t4_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t4_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t5_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t5_lo, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t6_hi, .bg = black, .fg = lblue});
  _geom.emplace_back(R{.geom = reg_value_t6_lo, .bg = black, .fg = lblue});

  // Column 3
  _geom.emplace_back(R{.geom = reg_value_m1_hi, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m1_lo, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m2_hi, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m2_lo, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m3_hi, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m3_lo, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m4_hi, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m4_lo, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m5_hi, .bg = black, .fg = white});
  _geom.emplace_back(R{.geom = reg_value_m5_lo, .bg = black, .fg = white});
}
std::vector<pepp::Item> one_byte_geom() {
  using namespace pepp;
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
  add_regbank(_geom);
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
  _geom.emplace_back(P{.geom = poly_alu, .bg = blue, .fg = lblue});
  _geom.emplace_back(T{.geom = label_alu, .text = "ALU", .color = black});
  _geom.emplace_back(T{.geom = label_alu_function, .text = "A + B", .color = black});
  _geom.emplace_back(L{.geom = poly_nzvc_join, .color = black});

  return _geom;
};

std::vector<pepp::Item> two_byte_geom() {
  using namespace pepp;
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
  QColor lblue(231, 234, 255);

  add_regbank(_geom);

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
  _geom.emplace_back(P{.geom = mux_marmux, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_marmux, .text = "MARMux", .color = black});
  _geom.emplace_back(R{.geom = mux_a, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_a, .text = "AMux", .color = black});
  _geom.emplace_back(R{.geom = mux_c, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_c, .text = "CMux", .color = black});
  _geom.emplace_back(R{.geom = mux_mdre, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_mdre, .text = "MDREMux", .color = black});
  _geom.emplace_back(R{.geom = mux_mdro, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_mdro, .text = "MDROMux", .color = black});
  _geom.emplace_back(R{.geom = mux_eo, .bg = black, .fg = white});
  _geom.emplace_back(T{.geom = mux_eo, .text = "EOMux", .color = black});
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
  _geom.emplace_back(R{.geom = reg_byte_mdre, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_mdre, .text = "MDRE", .color = black});
  _geom.emplace_back(R{.geom = reg_byte_mdro, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_mdro, .text = "MDRO", .color = black});
  _geom.emplace_back(R{.geom = reg_byte_marb, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_marb, .text = "MARB", .color = black});
  _geom.emplace_back(R{.geom = reg_byte_mara, .bg = black, .fg = lblue});
  _geom.emplace_back(T{.geom = reg_byte_mara, .text = "MARA", .color = black});

  // Other large polys
  _geom.emplace_back(P{.geom = poly_alu, .bg = blue, .fg = lblue});
  _geom.emplace_back(T{.geom = label_alu, .text = "ALU", .color = black});
  _geom.emplace_back(T{.geom = label_alu_function, .text = "A + B", .color = black});
  _geom.emplace_back(L{.geom = poly_nzvc_join, .color = black});

  return _geom;
};

void add_clock(QObject *parent, QList<pepp::QMLOverlay *> &list, QString label, QRect location, float x_offset = 0,
               float y_offset = 0) {
  auto local = new pepp::ClockOverlay(location.translated(x_offset, y_offset), label, parent);
  QQmlEngine::setObjectOwnership(local, QQmlEngine::CppOwnership);
  list.push_back(local);
}
void add_tristate(QObject *parent, QList<pepp::QMLOverlay *> &list, QString label, QRect location, int max,
                  float x_offset = 0, float y_offset = 0) {
  auto local = new pepp::TristateOverlay(location.translated(x_offset, y_offset), label, max, parent);
  QQmlEngine::setObjectOwnership(local, QQmlEngine::CppOwnership);
  list.push_back(local);
}
void add_text(QObject *parent, QList<pepp::QMLOverlay *> &list, QString label, QRect location, Qt::Alignment align,
              float x_offset = 0, float y_offset = 0) {
  // Apply same margins as in PaintDispatch::operator(const pepp::TextRectItem)
  if (align & Qt::AlignLeft) location.translate(5, 0);
  else if (align & Qt::AlignRight) location.translate(-5, 0);
  auto local = new pepp::TextOverlay(location.translated(x_offset, y_offset), label, align, parent);
  QQmlEngine::setObjectOwnership(local, QQmlEngine::CppOwnership);
  list.push_back(local);
}

void add_regbank_qml(QObject *parent, QList<pepp::QMLOverlay *> &list, float x_offset = 0, float y_offset = 0) {
  using namespace OneByteShapes;
  // Column 0
  add_text(parent, list, "A", reg_label_a_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_a_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_a_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "0,1", reg_label_a_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "X", reg_label_x_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_x_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_x_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "2,3", reg_label_x_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "SP", reg_label_sp_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_sp_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_sp_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "4,5", reg_label_sp_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "PC", reg_label_pc_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_pc_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_pc_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "6,7", reg_label_pc_num, Qt::AlignLeft, x_offset, y_offset);

  // Column 1
  add_text(parent, list, "IR", reg_label_is_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_is, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "8", reg_label_is_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "00", reg_value_os_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_os_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "9,10", reg_label_os_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "T1", reg_label_t1_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t1, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "11", reg_label_t1_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "T2", reg_label_t2_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t2_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t2_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "12,13", reg_label_t2_num, Qt::AlignLeft, x_offset, y_offset);

  // Column 2
  add_text(parent, list, "T3", reg_label_t3_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t3_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t3_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "14,15", reg_label_t3_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "T4", reg_label_t4_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t4_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t4_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "16,17", reg_label_t4_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "T5", reg_label_t5_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t5_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t5_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "18,19", reg_label_t5_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "T6", reg_label_t6_name, Qt::AlignRight, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t6_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "00", reg_value_t6_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "20,21", reg_label_t6_num, Qt::AlignLeft, x_offset, y_offset);

  // Column 3
  add_text(parent, list, "00", reg_value_m1_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "01", reg_value_m1_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "22,23", reg_label_m1_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "02", reg_value_m2_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "03", reg_value_m2_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "24,25", reg_label_m2_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "04", reg_value_m3_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "08", reg_value_m3_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "26,27", reg_label_m3_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "F0", reg_value_m4_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "F6", reg_value_m4_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "28,29", reg_label_m4_num, Qt::AlignLeft, x_offset, y_offset);

  add_text(parent, list, "FE", reg_value_m5_hi, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "FF", reg_value_m5_lo, Qt::AlignCenter, x_offset, y_offset);
  add_text(parent, list, "30,31", reg_label_m5_num, Qt::AlignLeft, x_offset, y_offset);
}

QList<pepp::QMLOverlay *> one_byte_overlays(QObject *parent, float x_offset, float y_offset) {
  using namespace OneByteShapes;
  QList<pepp::QMLOverlay *> ret;
  add_regbank_qml(parent, ret, x_offset, y_offset);
  add_clock(parent, ret, "LoadCk", ext_ck_load, x_offset, y_offset);
  add_tristate(parent, ret, "C", ext_sel_c, 31, x_offset, y_offset);
  add_tristate(parent, ret, "B", ext_sel_b, 31, x_offset, y_offset);
  add_tristate(parent, ret, "A", ext_sel_a, 31, x_offset, y_offset);
  add_clock(parent, ret, "MARCk", ext_ck_mar, x_offset, y_offset);
  add_clock(parent, ret, "MDRCk", ext_ck_mdr, x_offset, y_offset);
  add_tristate(parent, ret, "AMux", ext_sel_mux_a, 1, x_offset, y_offset);
  add_tristate(parent, ret, "CMux", ext_sel_mux_c, 1, x_offset, y_offset);
  add_tristate(parent, ret, "ALU", ext_sel_alu, 15, x_offset, y_offset);
  add_tristate(parent, ret, "CSMux", ext_sel_mux_cs, 1, x_offset, y_offset);
  add_clock(parent, ret, "SCk", ext_ck_s, x_offset, y_offset);
  add_clock(parent, ret, "CCk", ext_ck_c, x_offset, y_offset);
  add_clock(parent, ret, "VCk", ext_ck_v, x_offset, y_offset);
  add_tristate(parent, ret, "AndZ", ext_sel_andz, 1, x_offset, y_offset);
  add_clock(parent, ret, "ZCk", ext_ck_z, x_offset, y_offset);
  add_clock(parent, ret, "NCk", ext_ck_n, x_offset, y_offset);
  add_clock(parent, ret, "MemWrite", ext_sel_memwrite, x_offset, y_offset);
  add_clock(parent, ret, "MemRead", ext_sel_memread, x_offset, y_offset);
  return ret;
}

QList<pepp::QMLOverlay *> two_byte_overlays(QObject *parent, float x_offset, float y_offset) {
  using namespace TwoByteShapes;
  QList<pepp::QMLOverlay *> ret;
  add_regbank_qml(parent, ret, x_offset, y_offset);
  add_clock(parent, ret, "LoadCk", ext_ck_load, x_offset, y_offset);
  add_tristate(parent, ret, "C", ext_sel_c, 31, x_offset, y_offset);
  add_tristate(parent, ret, "B", ext_sel_b, 31, x_offset, y_offset);
  add_tristate(parent, ret, "A", ext_sel_a, 31, x_offset, y_offset);
  add_tristate(parent, ret, "MARMux", ext_sel_mux_mar, 1, x_offset, y_offset);
  add_clock(parent, ret, "MARCk", ext_ck_mar, x_offset, y_offset);
  add_clock(parent, ret, "MDRECk", ext_ck_mdre, x_offset, y_offset);
  add_tristate(parent, ret, "MDREMux", ext_sel_mux_mdre, 1, x_offset, y_offset);
  add_clock(parent, ret, "MDROCk", ext_ck_mdro, x_offset, y_offset);
  add_tristate(parent, ret, "MDROMux", ext_sel_mux_mdro, 1, x_offset, y_offset);
  add_tristate(parent, ret, "EOMux", ext_sel_mux_eo, 1, x_offset, y_offset);
  add_tristate(parent, ret, "AMux", ext_sel_mux_a, 1, x_offset, y_offset);
  add_tristate(parent, ret, "CMux", ext_sel_mux_c, 1, x_offset, y_offset);
  add_tristate(parent, ret, "ALU", ext_sel_alu, 15, x_offset, y_offset);
  add_tristate(parent, ret, "CSMux", ext_sel_mux_cs, 1, x_offset, y_offset);
  add_clock(parent, ret, "SCk", ext_ck_s, x_offset, y_offset);
  add_clock(parent, ret, "CCk", ext_ck_c, x_offset, y_offset);
  add_clock(parent, ret, "VCk", ext_ck_v, x_offset, y_offset);
  add_tristate(parent, ret, "AndZ", ext_sel_andz, 1, x_offset, y_offset);
  add_clock(parent, ret, "ZCk", ext_ck_z, x_offset, y_offset);
  add_clock(parent, ret, "NCk", ext_ck_n, x_offset, y_offset);
  add_clock(parent, ret, "MemWrite", ext_sel_memwrite, x_offset, y_offset);
  add_clock(parent, ret, "MemRead", ext_sel_memread, x_offset, y_offset);
  return ret;
}

} // namespace

namespace pepp {
struct PaintDispatch {
  pepp::PaintedCPUCanvas *canvas;
  QPainter *painter;
  QFont font{"Times New Roman", 14};
  void operator()(const pepp::LineItem &item) {
    painter->setPen(item.color);
    painter->drawLine(item.geom);
  }
  void operator()(const pepp::ArrowItem &item) {
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
  void operator()(const pepp::RectItem &item) {
    painter->setBrush(item.fg);
    painter->setPen(item.bg);
    painter->drawRect(item.geom);
  }
  void operator()(const pepp::PolygonItem &item) {
    painter->setBrush(item.fg);
    painter->setPen(item.bg);
    painter->drawConvexPolygon(item.geom);
  }
  void operator()(const pepp::TextRectItem &item) {
    painter->setPen(item.color);
    painter->setFont(font);
    // Add 5 px margin on left and right of text
    auto geom = item.geom;
    // Margins must be mirrored in add_text
    if (item.alignment & Qt::AlignLeft) geom.translate(5, 0);
    else if (item.alignment & Qt::AlignRight) geom.translate(-5, 0);
    painter->drawText(item.geom, item.alignment, item.text);
  }
};

struct BoundingBox {
  QPointF _min = QPointF(0, 0), _max = QPointF(0, 0);
  BoundingBox &extend(const BoundingBox &other) {
    _min.setX(std::min(_min.x(), other._min.x()));
    _min.setY(std::min(_min.y(), other._min.y()));
    _max.setX(std::max(_max.x(), other._max.x()));
    _max.setY(std::max(_max.y(), other._max.y()));
    return *this;
  }
};

// Compute the min and max coordinates of an item for bounding box calculation
struct BoundingBoxVisitor {
  BoundingBox operator()(const pepp::LineItem &item) {
    return BoundingBox{._min = item.geom.p1(), ._max = item.geom.p2()};
  }
  // Ignores the x/y of the arrow, because this visitor does not know the size of the arrow graphic.
  // I assume that an arrow will never be on the edge of the scene's bounding box, because this is true for the graphic
  // as authored for the book.
  BoundingBox operator()(const pepp::ArrowItem &item) {
    QPointF min = item.geom._lines.empty() ? QPointF(0, 0) : item.geom._lines[0].p1();
    QPointF max = min;
    for (const auto &line : item.geom._lines) {
      min.setX(std::min<float>(min.x(), std::min(line.p1().x(), line.p2().x())));
      min.setY(std::min<float>(min.y(), std::min(line.p1().y(), line.p2().y())));
      max.setX(std::max<float>(max.x(), std::max(line.p1().x(), line.p2().x())));
      max.setY(std::max<float>(max.y(), std::max(line.p1().y(), line.p2().y())));
    }
    return BoundingBox{._min = min, ._max = max};
  }
  BoundingBox operator()(const pepp::RectItem &item) {
    return BoundingBox{._min = item.geom.topLeft(), ._max = item.geom.bottomRight()};
  }
  BoundingBox operator()(const pepp::PolygonItem &item) {
    QPointF min = item.geom[0];
    QPointF max = min;
    for (const auto &line : item.geom) {
      min.setX(std::min<float>(min.x(), std::min(line.x(), line.x())));
      min.setY(std::min<float>(min.y(), std::min(line.y(), line.y())));
      max.setX(std::max<float>(max.x(), std::max(line.x(), line.x())));
      max.setY(std::max<float>(max.y(), std::max(line.y(), line.y())));
    }
    return BoundingBox{._min = min, ._max = max};
  }
  BoundingBox operator()(const pepp::TextRectItem &item) {
    return BoundingBox{._min = item.geom.topLeft(), ._max = item.geom.bottomRight()};
  }
};

} // namespace pepp

pepp::Painted1ByteCanvas::Painted1ByteCanvas(QQuickItem *parent) : PaintedCPUCanvas(Which::Pep9OneByte, parent) {}

pepp::Painted2ByteCanvas::Painted2ByteCanvas(QQuickItem *parent) : PaintedCPUCanvas(Which::Pep9TwoByte, parent) {}

pepp::PaintedCPUCanvas::PaintedCPUCanvas(Which which, QQuickItem *parent) : QQuickPaintedItem(parent) {
  switch (which) {
  case Which::Pep9OneByte:
    _geom = one_byte_geom();
    _overlays = one_byte_overlays(this, OneByteShapes::regbank_x_offset, OneByteShapes::regbank_y_offset);
    break;
  case Which::Pep9TwoByte:
    _geom = two_byte_geom();
    _overlays = two_byte_overlays(this, OneByteShapes::regbank_x_offset, OneByteShapes::regbank_y_offset);
    break;
  }
  BoundingBox minmax;
  BoundingBoxVisitor mmv;
  for (const auto &i : _geom) minmax.extend(std::visit(mmv, i));
  _w = minmax._max.x() - minmax._min.x();
  _h = minmax._max.y() - minmax._min.y();

  auto svg_path = ":/logi/arrow.svg";
  QImage svg_image(svg_path);
  _arrows[Qt::NoArrow] = QPixmap(QSize(0, 0));
  _arrows[Qt::LeftArrow] = QPixmap::fromImage(svg_image);
  _arrows[Qt::RightArrow] = QPixmap::fromImage(svg_image.flipped(Qt::Orientation::Horizontal));
  _arrows[Qt::UpArrow] = QPixmap::fromImage(svg_image.transformed(QTransform().rotate(90.0)));
  _arrows[Qt::DownArrow] = QPixmap::fromImage(svg_image.transformed(QTransform().rotate(270.0)));
}

pepp::PaintedCPUCanvas::~PaintedCPUCanvas() noexcept {
  for (auto &ptr : _overlays) delete ptr;
}

void pepp::PaintedCPUCanvas::paint(QPainter *painter) {
  painter->save();
  painter->translate(OneByteShapes::regbank_x_offset, OneByteShapes::regbank_y_offset);
  PaintDispatch disp{.canvas = this, .painter = painter};
  for (const auto &item : _geom) std::visit(disp, item);
  painter->restore();
}
