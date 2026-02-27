#pragma once
#include <QImage>
#include <QLine>
#include <QPair>
#include <QPoint>
#include <QPolygon>
#include <QVector>

#include <QVector>

struct Arrowhead {
  QPoint point;
  Qt::ArrowType orient = Qt::ArrowType::LeftArrow;
};
static const auto UP = Qt::ArrowType::UpArrow;
static const auto DOWN = Qt::ArrowType::DownArrow;
static const auto LEFT = Qt::ArrowType::LeftArrow; // Default
static const auto RIGHT = Qt::ArrowType::RightArrow;

class Arrow {
public:
  Arrow(QVector<Arrowhead> arrowheads, QVector<QLine> lines) {
    _arrowheads = arrowheads;
    _lines = lines;
  }

  QVector<Arrowhead> _arrowheads;
  QVector<QLine> _lines;

  Arrow translated(int dx, int dy) const {
    QVector<Arrowhead> arrowHeads;
    QVector<QLine> lines;

    for (const auto &ah : _arrowheads) arrowHeads.append({ah.point + QPoint(dx, dy), ah.orient});
    for (const auto &line : _lines) lines.append(line.translated(dx, dy));

    Arrow retVal(arrowHeads, lines);
    return retVal;
  }
};

namespace OneByteShapes {

// generic shapes:
enum Shapes {
  checkW = 60,
  checkH = 20,
  check2W = 80,
  check2H = checkH,
  labelW = 42,
  labelH = 20,
  dataLabelW = 69,
  dataLabelH = 19,
  labelTriW = 25,
  labelTriH = labelH,
  lineEditW = 25,
  lineEditH = 21,
  regLineEditW = 60,
  regHalfLineEditW = 30,
  regLineEditH = 19,

  arrowHDepth = 20, // really 15, but 20 with arrowHOffset
  arrowHOffset = 5,
  arrowLeftOff = 12,
  selectYOffset = 9,
  selectSlashOffset = 5,
  incrementerOffset = 10,

  aluSelOff = 57,
  selLineOff = 15,

};
const auto regbank_rowheight = 28;
const auto regbank_y_pad = regbank_rowheight - regLineEditH;
// We added another row of register to the regbank.
// A row is 28px, which is 2 x ypad, and we need 1x ypad for the bottom margin.
const auto regbank_y_offset = regbank_rowheight + regbank_y_pad;
const auto regbank_x_offset = 25 + 15 + regbank_y_pad;
enum RegPos {
  rowHeight = regbank_rowheight, // Original was 28
  columnWidth = 123 + 10,        // Original was 123
  Row1Y = 6 + regbank_y_pad - regbank_y_offset,
  Row2Y = Row1Y + rowHeight,
  Row3Y = Row2Y + rowHeight,
  Row4Y = Row3Y + rowHeight,
  Row5Y = Row4Y + rowHeight,
  Col1X = regbank_y_pad - regbank_x_offset,
  Col2X = Col1X + columnWidth,
  Col3X = Col2X + columnWidth,
  Col4X = Col3X + columnWidth,
};

enum CommonPositions {
  ctrlInputX = 570,
  ctrlLabelX = ctrlInputX + 29,
  interfaceRegsX = 175,
  statusBitsX = 476,
};

const QRect poly_regbank = QRect(5 - regbank_x_offset,
                                 // Y==6 is a magic constant derived by recompiling the program with a bunch of
                                 // different offsets and picking the one that is correct visually.
                                 6 - regbank_y_offset, Col4X - Col1X + columnWidth + 2 * regbank_y_pad,
                                 Row5Y - Row1Y + 1 * regbank_rowheight + regbank_y_pad);

// input/label/control section:
const QRect ext_ck_load = QRect(ctrlInputX, 18, check2W, check2H);
const QRect ext_sel_c = QRect(ctrlInputX, 39, lineEditW, lineEditH);
const QRect ext_sel_b = QRect(ctrlInputX, 61, lineEditW, lineEditH);
const QRect ext_sel_a = QRect(ctrlInputX, 83, lineEditW, lineEditH);

const QRect ext_ck_mar = QRect(ctrlInputX, 169, check2W, check2H);
const QRect reg_byte_marb = QRect(interfaceRegsX, 132, dataLabelW, dataLabelH);
const QRect reg_byte_mara = QRect(interfaceRegsX, 202, dataLabelW, dataLabelH);
const QRect ext_ck_mdr = QRect(ctrlInputX, 225, check2W, check2H);

const QRect mux_mdr = QRect(interfaceRegsX, 293, dataLabelW, dataLabelH);
const QRect reg_byte_mdr = QRect(interfaceRegsX, 254, dataLabelW, dataLabelH);

const QRect ext_sel_mux_a = QRect(ctrlInputX, 295, labelTriW, 21);
const QRect mux_a = QRect(306, 293, dataLabelW, dataLabelH);
const QRect ext_sel_mux_c = QRect(ctrlInputX, 348, labelTriW, labelTriH);
const QRect mux_c = QRect(250, 374, dataLabelW, dataLabelH);
const QRect ext_sel_alu = QRect(ctrlInputX, 368, 26, lineEditH);
const QRect label_alu = QRect(332, 350, 98, 20);
const QRect label_alu_function = QRect(332, 369, 98, 20);

const QRect mux_cs = QRect(statusBitsX + 19 - 69, 399, dataLabelW, dataLabelH);
const QRect ext_sel_mux_cs = QRect(ctrlInputX, 399, 25, 21);
const QRect ext_ck_s = QRect(ctrlInputX, 437, checkW, checkH);
const QRect reg_bit_s = QRect(statusBitsX, 437, 19, dataLabelH);
const QRect ext_ck_c = QRect(ctrlInputX, 464, checkW, checkH);
const QRect reg_bit_c = QRect(statusBitsX, 463, 19, dataLabelH);
const QRect ext_ck_v = QRect(ctrlInputX, 491, checkW, checkH);
const QRect reg_bit_v = QRect(statusBitsX, 491, 19, dataLabelH);
const QRect ext_sel_andz = QRect(ctrlInputX, 517, labelTriW, labelTriH);
const QRect mux_andz = QRect(416, 544, 41, 21);
const QRect ext_ck_z = QRect(ctrlInputX, 544, 60, 20);
const QRect reg_bit_z = QRect(statusBitsX, 544, 19, dataLabelH);
const QRect ext_ck_n = QRect(ctrlInputX, 586, checkW, checkH);
const QRect reg_bit_n = QRect(statusBitsX, 586, 19, dataLabelH);

const QRect ext_sel_memwrite = QRect(ctrlInputX, 611, labelTriW, labelTriH);
const QRect ext_sel_memread = QRect(ctrlInputX, 631, labelTriW, labelTriH);

// registers

// lines and shapes:
const QRect bus_addr = QRect(40, 151, 20, 500);
const QRect bus_data = QRect(bus_addr.x() + bus_addr.width(), bus_addr.top() + 100, 10, 400);

const Arrow ck_load =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset - 1, 24), LEFT},
          QVector<QLine>() << QLine(ctrlInputX - 7, ext_ck_load.y() + 9, poly_regbank.right() + arrowHOffset,
                                    ext_ck_load.y() + 9));
const Arrow sel_c =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset - 1, ext_sel_c.y() + 6)},
          QVector<QLine>() << QLine(ctrlInputX - 7, ext_sel_c.y() + selectYOffset, poly_regbank.right() + arrowHOffset,
                                    ext_sel_c.y() + selectYOffset)
                           << QLine(poly_regbank.right() + arrowHDepth + 6, ext_sel_c.y() + selectYOffset - 5,
                                    poly_regbank.right() + arrowHDepth + 16, ext_sel_c.y() + selectYOffset + 5));
const Arrow sel_b = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset - 1, ext_sel_b.y() + 6)},
    QVector<QLine>() << QLine(ctrlInputX - 7, ext_sel_b.y() + selectYOffset, poly_regbank.right() + arrowHOffset,
                              ext_sel_b.y() + selectYOffset)
                     << QLine(poly_regbank.right() + arrowHDepth + 6, ext_sel_b.y() + selectYOffset - selectSlashOffset,
                              poly_regbank.right() + arrowHDepth + 16,
                              ext_sel_b.y() + selectYOffset + selectSlashOffset));
const Arrow sel_a = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset - 1, ext_sel_a.y() + 6)},
    QVector<QLine>() << QLine(ctrlInputX - 7, ext_sel_a.y() + selectYOffset, poly_regbank.right() + arrowHOffset,
                              ext_sel_a.y() + selectYOffset)
                     << QLine(poly_regbank.right() + arrowHDepth + 6, ext_sel_a.y() + selectYOffset - selectSlashOffset,
                              poly_regbank.right() + arrowHDepth + 16,
                              ext_sel_a.y() + selectYOffset + selectSlashOffset));
const Arrow ck_mar = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(232, 155), UP} << Arrowhead{QPoint(232, 191), DOWN},
                           QVector<QLine>() << QLine(ctrlInputX - 7, 177, 235, 177) << QLine(235, 163, 235, 191));
const Arrow ck_mdr = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(207, 241), DOWN},
                           QVector<QLine>() << QLine(ctrlInputX - 7, 233, 210, 233) << QLine(210, 233, 210, 241));
const Arrow sel_muxa = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(380, 300)},
                             QVector<QLine>() << QLine(ctrlInputX - 7, 303, 388, 303));
const QPolygon bus_amux_to_alu =
    QPolygon(QVector<QPoint>() << QPoint(336, 312) << QPoint(336, 331) << QPoint(331, 331) << QPoint(341, 341)
                               << QPoint(351, 331) << QPoint(346, 331) << QPoint(346, 312));
const Arrow sel_muxc = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(mux_c.left() + 7, mux_c.top() - 12), DOWN},
                             QVector<QLine>() << QLine(260, 355, ctrlInputX - 7, 355) << QLine(260, 355, 260, 365));
const QPolygon bus_c =
    QPolygon(QVector<QPoint>() << QPoint(290, 374) << QPoint(290, 130) << QPoint(295, 130) << QPoint(285, 120)
                               << QPoint(275, 130) << QPoint(280, 130) << QPoint(280, 334) << QPoint(240, 334)
                               << QPoint(240, 326) << QPoint(245, 326) << QPoint(235, 316) << QPoint(225, 326)
                               << QPoint(230, 326) << QPoint(230, 344) << QPoint(280, 344) << QPoint(280, 374));

// Use an enumeration for points that other shapes (like AMux) rely on, so that it is easier to re-arrange the diagram.
enum poly_aluNumbers {
  ALUUpperLeftLine_LeftPoint = 314,
  ALUUpperLeftLine_RightPoint = 366,
  ALUUpperRightLine_LeftPoint = 394,
  ALUUpperRightLine_RightPoint = 447,
  ALUTopBound = 342,
  ALUBottomBound = 394,
};
const QPolygon poly_alu =
    QPolygon(QVector<QPoint>() << QPoint(ALUUpperLeftLine_LeftPoint, ALUTopBound)
                               << QPoint(ALUUpperLeftLine_RightPoint, ALUTopBound) << QPoint(370, 353)
                               << QPoint(390, 353) << QPoint(ALUUpperRightLine_LeftPoint, ALUTopBound)
                               << QPoint(ALUUpperRightLine_RightPoint, ALUTopBound) << QPoint(421, ALUBottomBound)
                               << QPoint(340, ALUBottomBound));

const QPolygon bus_mdr_to_amux =
    QPolygon(QVector<QPoint>() << QPoint(244, 258) << QPoint(326, 258) << QPoint(326, 280) << QPoint(331, 280)
                               << QPoint(321, 290) << QPoint(311, 280) << QPoint(316, 280) << QPoint(316, 268)
                               << QPoint(244, 268));

const QPolygon bus_mar_to_addr =
    QPolygon(QVector<QPoint>() << QPoint(205, 151)
                               << QPoint(205, 167)
                               // arrow:
                               << QPoint(bus_addr.x() + bus_addr.width() + arrowHDepth, 167)
                               << QPoint(bus_addr.x() + bus_addr.width() + arrowHDepth, 162)
                               << QPoint(bus_addr.x() + bus_addr.width() + arrowHOffset, 177)
                               << QPoint(bus_addr.x() + bus_addr.width() + arrowHDepth, 192)
                               << QPoint(bus_addr.x() + bus_addr.width() + arrowHDepth, 187) << QPoint(205, 187)
                               << QPoint(205, 202)
                               << QPoint(215, 202)
                               // black line in the middle:
                               << QPoint(215, 151) << QPoint(215, 177)
                               << QPoint(OneByteShapes::bus_addr.right() + arrowHDepth, 177) << QPoint(215, 177)
                               << QPoint(215, 151));
const QPolygon bus_nzvc_to_cmux =
    QPolygon(QVector<QPoint>() << QPoint(310, 513) << QPoint(269, 513) << QPoint(269, 407) << QPoint(274, 407)
                               << QPoint(264, 397) << QPoint(254, 407) << QPoint(259, 407) << QPoint(259, 523)
                               << QPoint(310, 523));

// Note that *_num is 1.5x width. Technically, it could overlap with the adjacent column's name.
// However, name+number is always < 2*regHalfLineEditW, so no overlap can occur in practice.
const QRect reg_label_a_name = QRect(Col1X, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_a_hi = QRect(Col1X + 1 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_a_lo = QRect(Col1X + 2 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_a_num = QRect(Col1X + 3 * regHalfLineEditW, Row1Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_x_name = QRect(Col1X, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_x_hi = QRect(Col1X + 1 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_x_lo = QRect(Col1X + 2 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_x_num = QRect(Col1X + 3 * regHalfLineEditW, Row2Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_sp_name = QRect(Col1X, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_sp_hi = QRect(Col1X + 1 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_sp_lo = QRect(Col1X + 2 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_sp_num = QRect(Col1X + 3 * regHalfLineEditW, Row3Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_pc_name = QRect(Col1X, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_pc_hi = QRect(Col1X + 1 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_pc_lo = QRect(Col1X + 2 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_pc_num = QRect(Col1X + 3 * regHalfLineEditW, Row4Y, regHalfLineEditW * 1.5, regLineEditH);

const QRect reg_label_is_name = QRect(Col2X, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_is = QRect(Col2X + 1 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_is_num = QRect(Col2X + 2 * regHalfLineEditW, Row1Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_os_name = QRect(Col2X, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_os_hi = QRect(Col2X + 1 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_os_lo = QRect(Col2X + 2 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_os_num = QRect(Col2X + 3 * regHalfLineEditW, Row2Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_t1_name = QRect(Col2X, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t1 = QRect(Col2X + 1 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_t1_num = QRect(Col2X + 2 * regHalfLineEditW, Row3Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_t2_name = QRect(Col2X, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t2_hi = QRect(Col2X + 1 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t2_lo = QRect(Col2X + 2 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_t2_num = QRect(Col2X + 3 * regHalfLineEditW, Row4Y, regHalfLineEditW * 1.5, regLineEditH);

const QRect reg_label_t3_name = QRect(Col3X, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t3_hi = QRect(Col3X + 1 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t3_lo = QRect(Col3X + 2 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_t3_num = QRect(Col3X + 3 * regHalfLineEditW, Row1Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_t4_name = QRect(Col3X, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t4_hi = QRect(Col3X + 1 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t4_lo = QRect(Col3X + 2 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_t4_num = QRect(Col3X + 3 * regHalfLineEditW, Row2Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_t5_name = QRect(Col3X, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t5_hi = QRect(Col3X + 1 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t5_lo = QRect(Col3X + 2 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_t5_num = QRect(Col3X + 3 * regHalfLineEditW, Row3Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_t6_name = QRect(Col3X, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t6_hi = QRect(Col3X + 1 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_t6_lo = QRect(Col3X + 2 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_t6_num = QRect(Col3X + 3 * regHalfLineEditW, Row4Y, regHalfLineEditW * 1.5, regLineEditH);

const QRect reg_label_m1_name = QRect(Col4X, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m1_hi = QRect(Col4X + 1 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m1_lo = QRect(Col4X + 2 * regHalfLineEditW, Row1Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_m1_num = QRect(Col4X + 3 * regHalfLineEditW, Row1Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_m2_name = QRect(Col4X, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m2_hi = QRect(Col4X + 1 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m2_lo = QRect(Col4X + 2 * regHalfLineEditW, Row2Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_m2_num = QRect(Col4X + 3 * regHalfLineEditW, Row2Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_m3_name = QRect(Col4X, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m3_hi = QRect(Col4X + 1 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m3_lo = QRect(Col4X + 2 * regHalfLineEditW, Row3Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_m3_num = QRect(Col4X + 3 * regHalfLineEditW, Row3Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_m4_name = QRect(Col4X, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m4_hi = QRect(Col4X + 1 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m4_lo = QRect(Col4X + 2 * regHalfLineEditW, Row4Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_m4_num = QRect(Col4X + 3 * regHalfLineEditW, Row4Y, regHalfLineEditW * 1.5, regLineEditH);
const QRect reg_label_m5_name = QRect(Col4X, Row5Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m5_hi = QRect(Col4X + 1 * regHalfLineEditW, Row5Y, regHalfLineEditW, regLineEditH);
const QRect reg_value_m5_lo = QRect(Col4X + 2 * regHalfLineEditW, Row5Y, regHalfLineEditW, regLineEditH);
const QRect reg_label_m5_num = QRect(Col4X + 3 * regHalfLineEditW, Row5Y, regHalfLineEditW * 1.5, regLineEditH);

const QPolygon bus_b =
    QPolygon(QVector<QPoint>() << QPoint(417, 118) << QPoint(417, 136) << QPoint(258, 136) << QPoint(258, 131)
                               << QPoint(248, 141) << QPoint(258, 151) << QPoint(258, 146) << QPoint(280, 146)
                               << QPoint(366, 146) << QPoint(417, 146) << QPoint(417, 331) << QPoint(412, 331)
                               << QPoint(422, 341) << QPoint(432, 331) << QPoint(427, 331) << QPoint(427, 118));

const QPolygon bus_a =
    QPolygon(QVector<QPoint>() << QPoint(356, 118) << QPoint(356, 207) << QPoint(258, 207) << QPoint(258, 202)
                               << QPoint(248, 212) << QPoint(258, 222) << QPoint(258, 217) << QPoint(356, 217)
                               << QPoint(356, 280) << QPoint(351, 280) << QPoint(361, 290) << QPoint(371, 280)
                               << QPoint(366, 280) << QPoint(366, 118));

const QPolygon bus_addr_to_ddr = QPolygon(QVector<QPoint>()
                                          // arrowhead
                                          << QPoint(18, 330) << QPoint(18, 325) << QPoint(3, 340) << QPoint(18, 355)
                                          << QPoint(18, 350)
                                          // blunt end at the bus:
                                          << QPoint(40, 350) << QPoint(40, 330));

const QPolygon bus_data_to_mdrmux = QPolygon(QVector<QPoint>()
                                             // foot:
                                             << QPoint(190, 344) << QPoint(70, 344) << QPoint(70, 334)
                                             << QPoint(180, 334)
                                             // arrowhead:
                                             << QPoint(180, 326) << QPoint(175, 326) << QPoint(185, 316)
                                             << QPoint(195, 326) << QPoint(190, 326));

const QPolygon bus_mdr_to_data =
    QPolygon(QVector<QPoint>() << QPoint(175, 258) << QPoint(83, 258) << QPoint(83, 253) << QPoint(73, 263)
                               << QPoint(83, 273) << QPoint(83, 268) << QPoint(175, 268));

const QPolygon bus_alu_to_cmux =
    QPolygon(QVector<QPoint>() << QPoint(346, 395) << QPoint(346, 414) << QPoint(314, 414) << QPoint(314, 407)
                               << QPoint(319, 407) << QPoint(309, 397) << QPoint(299, 407) << QPoint(304, 407)
                               << QPoint(304, 424) << QPoint(356, 424) << QPoint(356, 395));

const QPolygon bus_mdrmux_to_mdr =
    QPolygon(QVector<QPoint>() << QPoint(205, mux_mdr.y()) // 293
                               << QPoint(205, 286) << QPoint(200, 286) << QPoint(210, 276) << QPoint(220, 286)
                               << QPoint(215, 286) << QPoint(215, mux_mdr.y()));

const Arrow sel_alu = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(poly_alu.boundingRect().right() - 13,
                                                                     poly_alu.boundingRect().bottom() - 21)},
                            QVector<QLine>() << QLine(439, 376, ctrlInputX - 7, ext_sel_alu.y() + selectYOffset - 1)
                                             << QLine(ctrlInputX - 17, ext_sel_alu.y() + 13, ctrlInputX - 27,
                                                      ext_sel_alu.y() + 3)); // diagonal line

const Arrow logic_alu_nzvc = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(reg_bit_n.left() - arrowLeftOff, reg_bit_n.y() + arrowHOffset + 1), RIGHT}
                         <<                                                                            // N
        Arrowhead{QPoint(mux_andz.left() - arrowLeftOff, reg_bit_z.y() + arrowHOffset + 1), RIGHT} <<  // Z
        Arrowhead{QPoint(reg_bit_v.left() - arrowLeftOff, reg_bit_v.y() + arrowHOffset + 1), RIGHT} << // V
        Arrowhead{QPoint(reg_bit_c.left() - arrowLeftOff, reg_bit_c.y() + arrowHOffset + 1), RIGHT} << // C
        Arrowhead{QPoint(reg_bit_s.left() - arrowLeftOff, reg_bit_s.y() + arrowHOffset + 1), RIGHT},   // S
    QVector<QLine>() <<
        // N
        QLine(poly_alu.boundingRect().left() + aluSelOff, poly_alu.boundingRect().bottom(),
              poly_alu.boundingRect().left() + aluSelOff,
              reg_bit_n.y() + selectYOffset)
                     << // 586+8
        QLine(poly_alu.boundingRect().left() + aluSelOff, reg_bit_n.y() + selectYOffset,
              reg_bit_n.left() - arrowLeftOff,
              reg_bit_n.y() + selectYOffset)
                     << // 586+8
        // Z
        QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff, poly_alu.boundingRect().bottom(),
              poly_alu.boundingRect().left() + aluSelOff + selLineOff, reg_bit_z.y() + selectYOffset)
                     << QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff, reg_bit_z.y() + selectYOffset,
                              mux_andz.left() - arrowLeftOff, reg_bit_z.y() + selectYOffset)
                     <<

        // V
        QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff * 2, poly_alu.boundingRect().bottom(),
              poly_alu.boundingRect().left() + aluSelOff + selLineOff * 2, reg_bit_v.y() + selectYOffset)
                     << QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff * 2,
                              reg_bit_v.y() + selectYOffset, reg_bit_v.left() - arrowLeftOff,
                              reg_bit_v.y() + selectYOffset)
                     <<

        // C
        QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff * 3, poly_alu.boundingRect().bottom(),
              poly_alu.boundingRect().left() + aluSelOff + selLineOff * 3, reg_bit_c.y() + selectYOffset)
                     << QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff * 3,
                              reg_bit_c.y() + selectYOffset, reg_bit_c.left() - arrowLeftOff,
                              reg_bit_c.y() + selectYOffset)
                     <<
        // S
        QLine(poly_alu.boundingRect().left() + aluSelOff + selLineOff * 3, reg_bit_s.y() + selectYOffset,
              reg_bit_s.left() - arrowLeftOff, reg_bit_s.y() + selectYOffset));

const QLine sel_muxcs = QLine(mux_cs.right() + arrowHOffset, ext_sel_mux_cs.y() + selectYOffset + 1, ctrlInputX - 7,
                              ext_sel_mux_cs.y() + selectYOffset + 1);
const QLine ck_s = QLine(reg_bit_s.right() + arrowHOffset, reg_bit_s.y() + selectYOffset, ctrlInputX - 7,
                         reg_bit_s.y() + selectYOffset);
const QLine ck_c = QLine(reg_bit_c.right() + arrowHOffset, reg_bit_c.y() + selectYOffset, ctrlInputX - 7,
                         reg_bit_c.y() + selectYOffset);
const QLine ck_v = QLine(reg_bit_v.right() + arrowHOffset, reg_bit_v.y() + selectYOffset, ctrlInputX - 7,
                         reg_bit_v.y() + selectYOffset);
const QLine ck_z = QLine(reg_bit_z.right() + arrowHOffset, reg_bit_z.y() + selectYOffset, ctrlInputX - 7,
                         reg_bit_z.y() + selectYOffset);
const QLine ck_n = QLine(reg_bit_n.right() + arrowHOffset, reg_bit_n.y() + selectYOffset, ctrlInputX - 7,
                         reg_bit_n.y() + selectYOffset);

const QLine poly_nzvc_join = QLine(310, 477, 310, 559);
const Arrow logic_c_to_nzvc =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(314, 483)},
          QVector<QLine>() << QLine(487, 482, 487, 486) << QLine(330, 486, 322, 486) << QLine(330, 486, 487, 486));
const Arrow logic_c_to_csmux =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(431, 421), UP}, QVector<QLine>() << QLine(434, 426, 434, 486));
const Arrow logic_cin = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(428, 386)},
                              QVector<QLine>() << QLine(461, 389, 433, 389) << QLine(461, 399, 461, 389));
const Arrow logic_s_to_csmux =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(484, 421), UP}, QVector<QLine>() << QLine(487, 437, 487, 429));
const Arrow logic_z_to_nzvc =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(314, 503)} << Arrowhead{QPoint(434, 566), UP},
          QVector<QLine>() << QLine(487, 563, 487, 582) // vertical line to Z bit
                           << QLine(487, 582, 341, 582) // long horizontal line
                           << QLine(341, 582, 341, 506) // vertical line closest to arrowhead
                           << QLine(341, 506, 322, 506) // line from arrowhead on left
                           << QLine(437, 582, 437, 574));
const Arrow logic_v_to_nzvc = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(314, 493)},
                                    QVector<QLine>() << QLine(487, 510, 487, 514) // bitty bit
                                                     << QLine(487, 514, 352, 514) << QLine(352, 513, 352, 496)
                                                     << QLine(352, 496, 322, 496)); // short line from the arrow
const Arrow logic_n_to_nzvc = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(314, 514)},
                                    QVector<QLine>() << QLine(487, 605, 487, 609) << QLine(487, 609, 330, 609)
                                                     << QLine(330, 609, 330, 517) << QLine(330, 517, 322, 517));

const Arrow logic_andz_to_z = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(mux_andz.x() + mux_andz.width() / 2 - arrowHOffset / 2,
                                             mux_andz.top() - selectYOffset - 3),
                                      DOWN}
                         << Arrowhead{QPoint(reg_bit_z.x() - 12, mux_andz.y() + mux_andz.height() / 2 - 3), RIGHT},
    QVector<QLine>()
        // Connects arrow head to horizontal line
        << QLine(mux_andz.x() + mux_andz.width() / 2, ext_sel_andz.y() + ext_sel_andz.height() / 2,
                 mux_andz.x() + mux_andz.width() / 2, mux_andz.y() - arrowHOffset)
        // Horizontal line from label to arrowhead.
        << QLine(mux_andz.x() + mux_andz.width() / 2, ext_sel_andz.y() + ext_sel_andz.height() / 2, ctrlInputX - 7,
                 ext_sel_andz.y() + ext_sel_andz.height() / 2)
        // Line from ANDZ circuit to Z bit.
        << QLine(mux_andz.right(), mux_andz.y() + mux_andz.height() / 2, reg_bit_z.left() - arrowHOffset,
                 mux_andz.y() + mux_andz.height() / 2));

const QLine ck_memread = QLine(bus_data.right() + arrowHOffset, ext_sel_memread.y() + selectYOffset, ctrlInputX - 7,
                               ext_sel_memread.y() + selectYOffset);
const QLine ck_memwrite = QLine(bus_data.right() + arrowHOffset, ext_sel_memwrite.y() + selectYOffset, ctrlInputX - 7,
                                ext_sel_memwrite.y() + selectYOffset);

} // namespace OneByteShapes
