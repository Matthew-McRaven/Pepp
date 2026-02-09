#include "renderer.hpp"
#include <QPainter>
#include "shapes_one.hpp"

CursedCPUCanvas::CursedCPUCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {
  using namespace OneByteShapes;
  using I = Item;
  QColor black(0, 0, 0, 255);
  // Commented out bits are labels and edits.
  // Coordinates are (generally) poorly named
  _geom.emplace_back(I{.geom = RegBank, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = loadCkCheckbox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cLineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = bLineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = bLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aLineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MARCkCheckbox, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MARBLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MARALabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MDRCkCheckbox, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MARIncrementer, .bg = black, .fg = black}); // Actually MARMUX
  ///_geom.emplace_back(I{.geom = MDRMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MDRMuxLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MDRMuxerDataLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MDRLabel, .bg = black, .fg = black}); // Actually the MDR reg
  ///_geom.emplace_back(I{.geom = aMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = aMuxLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = aMuxerDataLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = cMuxLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = cMuxerLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ALULineEdit, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ALULabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ALUFunctionLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = CSMuxLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = CSMuxerDataLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = CSMuxTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = SCkCheckBox, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = sBitLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = CCkCheckBox, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = cBitLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = VCkCheckBox, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = vBitLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = AndZLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = AndZTristateLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = AndZMuxLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = ZCkCheckBox, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = zBitLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = NCkCheckBox, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = nBitLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemWriteLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemWriteTristateLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemReadLabel, .bg = black, .fg = black});
  ///_geom.emplace_back(I{.geom = MemReadTristateLabel, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = AddrBus, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = DataBus, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MDRBusOutRect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = BBusRect, .bg = black, .fg = black});

  _geom.emplace_back(I{.geom = CSMuxSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = SBitSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = CBitSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = VBitSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = ZBitSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = NBitSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = NZVCDataLine, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MemReadSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = MemWriteSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = loadCkSelect, .bg = black, .fg = black});
  _geom.emplace_back(I{.geom = CSelect});
  _geom.emplace_back(I{.geom = BSelect});
  _geom.emplace_back(I{.geom = ASelect});
  _geom.emplace_back(I{.geom = MARCk});
  _geom.emplace_back(I{.geom = MDRCk});
  _geom.emplace_back(I{.geom = AMuxSelect});
  _geom.emplace_back(I{.geom = CMuxSelect});
  _geom.emplace_back(I{.geom = ALUSelect});
  _geom.emplace_back(I{.geom = ALUSelectOut});
  _geom.emplace_back(I{.geom = CBitToNZVC});
  _geom.emplace_back(I{.geom = CBitToCSMux});
  _geom.emplace_back(I{.geom = CInToALU});
  _geom.emplace_back(I{.geom = ZBitOut});
  _geom.emplace_back(I{.geom = VBitOut});
  _geom.emplace_back(I{.geom = NBitOut});
  _geom.emplace_back(I{.geom = AndZOut});

  _geom.emplace_back(I{
      .geom = AMuxBus,
  });
  _geom.emplace_back(I{CMuxBus});
  _geom.emplace_back(I{ALUPoly});
  _geom.emplace_back(I{MDRBusOutArrow});
  _geom.emplace_back(I{MARBus});
  _geom.emplace_back(I{NZVCDataPath});
  _geom.emplace_back(I{BBus1});
  _geom.emplace_back(I{ABus1});
  _geom.emplace_back(I{ABus2});
  _geom.emplace_back(I{CBus});
  _geom.emplace_back(I{AddrArrow});
  _geom.emplace_back(I{DataToMDRMuxBus});
  _geom.emplace_back(I{MDRToDataBus});
  _geom.emplace_back(I{ALUOutBus});
  _geom.emplace_back(I{MDRMuxOutBus});
  _geom.emplace_back(I{DataToMDRMuxBus});
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
      break;
    }
    }
  }
}
