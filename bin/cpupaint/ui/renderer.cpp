#include "renderer.hpp"
#include <QPainter>
#include <QTransform>
#include "shapes_one.hpp"
#include "shapes_two.hpp"

std::vector<Item> one_byte_geom() {
  using namespace OneByteShapes;
  using I = Item;

  std::vector<Item> _geom;
  QColor black(0, 0, 0, 255);
  QColor white(255, 255, 255, 255);
  QColor red(255, 0, 0);
  QColor green(0, 255, 0);
  QColor blue(255, 0, 255);

  // Clocks
  _geom.emplace_back(I{.geom = ck_mar});
  _geom.emplace_back(I{.geom = ck_mdr});
  _geom.emplace_back(I{.geom = ck_load});
  _geom.emplace_back(I{.geom = ck_n});
  _geom.emplace_back(I{.geom = ck_z});
  _geom.emplace_back(I{.geom = ck_v});
  _geom.emplace_back(I{.geom = ck_c});
  _geom.emplace_back(I{.geom = ck_s});
  _geom.emplace_back(I{.geom = ck_memread});
  _geom.emplace_back(I{.geom = ck_memwrite});

  // Control Wires
  _geom.emplace_back(I{.geom = sel_muxcs});
  _geom.emplace_back(I{.geom = sel_muxa});
  _geom.emplace_back(I{.geom = sel_muxc});
  _geom.emplace_back(I{.geom = sel_c});
  _geom.emplace_back(I{.geom = sel_b});
  _geom.emplace_back(I{.geom = sel_a});
  _geom.emplace_back(I{.geom = sel_alu});

  // Buses
  _geom.emplace_back(I{.geom = bus_b, .bg = black, .fg = red});
  _geom.emplace_back(I{.geom = bus_a, .bg = black, .fg = green});
  _geom.emplace_back(I{.geom = bus_addr, .bg = black, .fg = blue});
  _geom.emplace_back(I{.geom = bus_addr_to_ddr, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_data, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_data_to_mdrmux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdrmux_to_mdr, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdr_to_data, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mar_to_addr, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_alu_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_nzvc_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdr_to_amux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_amux_to_alu, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_c, .bg = black, .fg = white});

  // Combinatorial Outputs
  _geom.emplace_back(I{.geom = logic_alu_nzvc});
  _geom.emplace_back(I{.geom = logic_c_to_nzvc});
  _geom.emplace_back(I{.geom = logic_c_to_csmux});
  _geom.emplace_back(I{.geom = logic_s_to_csmux});
  _geom.emplace_back(I{.geom = logic_cin});
  _geom.emplace_back(I{.geom = logic_z_to_nzvc});
  _geom.emplace_back(I{.geom = logic_v_to_nzvc});
  _geom.emplace_back(I{.geom = logic_n_to_nzvc});
  _geom.emplace_back(I{.geom = logic_andz_to_z});

  // Multiplexers
  _geom.emplace_back(I{.geom = mux_a, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_c, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_mdr, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_cs, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_andz, .bg = black, .fg = black});

  // Register outlines & registers
  _geom.emplace_back(I{.geom = reg_bit_n, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_c, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_v, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_z, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_s, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_mdr, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_marb, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_mara, .bg = black, .fg = black});

  // Other large polys
  _geom.emplace_back(I{.geom = poly_regbank, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = poly_alu});
  _geom.emplace_back(I{.geom = poly_nzvc_join, .bg = black, .fg = black});

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
  ///_geom.emplace_back(I{.geom = ALUFunctionLabel, .bg = black, .fg = black});
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

std::vector<Item> two_byte_geom() {
  using namespace TwoByteShapes;
  using I = Item;

  std::vector<Item> _geom;
  QColor black(0, 0, 0, 255);
  QColor white(255, 255, 255, 255);
  QColor red(255, 0, 0);
  QColor green(0, 255, 0);
  QColor blue(0, 0, 255);
  QColor pink(255, 0, 255);

  // Clocks
  _geom.emplace_back(I{.geom = ck_mar});
  _geom.emplace_back(I{.geom = ck_mdre});
  _geom.emplace_back(I{.geom = ck_mdro});
  _geom.emplace_back(I{.geom = ck_load});
  _geom.emplace_back(I{.geom = ck_n});
  _geom.emplace_back(I{.geom = ck_z});
  _geom.emplace_back(I{.geom = ck_v});
  _geom.emplace_back(I{.geom = ck_c});
  _geom.emplace_back(I{.geom = ck_s});
  _geom.emplace_back(I{.geom = ck_memread});
  _geom.emplace_back(I{.geom = ck_memwrite});

  // Control Wires
  _geom.emplace_back(I{.geom = sel_muxcs});
  _geom.emplace_back(I{.geom = sel_muxa});
  _geom.emplace_back(I{.geom = sel_muxc});
  _geom.emplace_back(I{.geom = sel_c});
  _geom.emplace_back(I{.geom = sel_b});
  _geom.emplace_back(I{.geom = sel_a});
  _geom.emplace_back(I{.geom = sel_alu});
  _geom.emplace_back(I{.geom = sel_mux_mdre});
  _geom.emplace_back(I{.geom = sel_mux_mdro});
  _geom.emplace_back(I{.geom = sel_muxeo});
  _geom.emplace_back(I{.geom = sel_mux_mar});

  // Buses
  _geom.emplace_back(I{.geom = bus_b, .bg = black, .fg = blue});
  _geom.emplace_back(I{.geom = bus_a, .bg = black, .fg = green});
  _geom.emplace_back(I{.geom = bus_addr, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_addr_to_ddr, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_data, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdremux_to_mdre, .bg = black, .fg = red});
  _geom.emplace_back(I{.geom = bus_mdromux_to_mdro, .bg = black, .fg = green});
  _geom.emplace_back(I{.geom = bus_data_to_mdremux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_data_to_mdromux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdre_to_data, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdro_to_data, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mdre_to_eomux, .bg = black, .fg = red});
  _geom.emplace_back(I{.geom = bus_mdro_to_eomux, .bg = black, .fg = green});
  _geom.emplace_back(I{.geom = bux_marmux_to_mara, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_marmux_to_marb, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_mar_to_addr, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_alu_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_nzvc_to_cmux, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_eomux_to_amux, .bg = black, .fg = red});
  _geom.emplace_back(I{.geom = bus_amux_to_alu, .bg = black, .fg = white});
  _geom.emplace_back(I{.geom = bus_c, .bg = black, .fg = pink});

  // Combinatorial Outputs
  _geom.emplace_back(I{.geom = logic_alu_nzvc});
  _geom.emplace_back(I{.geom = logic_c_to_nzvc});
  _geom.emplace_back(I{.geom = logic_c_to_csmux});
  _geom.emplace_back(I{.geom = logic_s_to_csmux});
  _geom.emplace_back(I{.geom = logic_cin});
  _geom.emplace_back(I{.geom = logic_z_to_nzvc});
  _geom.emplace_back(I{.geom = logic_v_to_nzvc});
  _geom.emplace_back(I{.geom = logic_n_to_nzvc});
  _geom.emplace_back(I{.geom = logic_andz_to_z});

  // Multiplexers
  _geom.emplace_back(I{.geom = mux_a, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_c, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_mdre, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_mdro, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_eo, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_cs, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = mux_andz, .bg = black, .fg = black});

  // Register outlines & registers
  _geom.emplace_back(I{.geom = reg_bit_n, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_c, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_v, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_z, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_bit_s, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_mdre, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_mdro, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_marb, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = reg_byte_mara, .bg = black, .fg = black});

  // Other large polys
  _geom.emplace_back(I{.geom = poly_regbank, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = poly_alu});
  _geom.emplace_back(I{.geom = poly_nzvc_join, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = poly_marmux, .bg = black, .fg = black});

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
  ///_geom.emplace_back(I{.geom = ALUFunctionLabel, .bg = black, .fg = black});
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
  _geom = two_byte_geom();
  auto svg_path = ":/qt/qml/CPUPaint/svg/arrow.svg";
  QImage svg_image(svg_path);
  _arrows[Qt::NoArrow] = QPixmap(QSize(0, 0));
  _arrows[Qt::LeftArrow] = QPixmap::fromImage(svg_image);
  _arrows[Qt::RightArrow] = QPixmap::fromImage(svg_image.flipped(Qt::Orientation::Horizontal));
  _arrows[Qt::UpArrow] = QPixmap::fromImage(svg_image.transformed(QTransform().rotate(90.0)));
  _arrows[Qt::DownArrow] = QPixmap::fromImage(svg_image.transformed(QTransform().rotate(270.0)));
}

void CursedCPUCanvas::paint(QPainter *painter) {

  for (const auto &item : _geom) {
    if (!item.enabled) continue;
    painter->setBrush(item.fg);
    painter->setPen(item.bg);
    switch (item.geom.index()) {
    case 0: painter->drawRect(std::get<QRect>(item.geom)); break;
    case 1: painter->drawConvexPolygon(std::get<QPolygon>(item.geom)); break;
    case 2: painter->drawLine(std::get<QLine>(item.geom)); break;
    case 3: {
      const auto &arrow = std::get<Arrow>(item.geom);
      for (const QLine &line : arrow._lines) painter->drawLine(line);
      for (const Arrowhead &head : arrow._arrowheads) {
        painter->save();
        painter->translate(head.point);
        int orient = static_cast<int>(head.orient);
        painter->drawPixmap(0, 0, _arrows[orient]);
        painter->restore();
      }
      break;
    }
    }
  }
}
