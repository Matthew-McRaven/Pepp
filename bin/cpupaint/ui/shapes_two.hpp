#pragma once

#include <QImage>
#include <QLine>
#include <QPair>
#include <QPoint>
#include <QPolygon>
#include <QVector>

#include <QVector>

#include "shapes_one.hpp"

namespace TwoByteShapes {
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
  regLineEditH = 19,

  arrowHDepth = 20, // 20 with arrowHOffset, "really" 15
  arrowHOffset = 5,
  arrowLeftOff = 12,
  iRegXOffset = 50,
  MDREOOffset = 100,
  selectYOffset = 9,
  selectSlashOffset = 5,
  incrementerOffset = 10,

  aluSelOff = 57,
  aluOffsetY = 190,
  selLineOff = 15,

};

enum CommonPositions {
  controlOffsetX = 50,
  ctrlLabelX = 579 + controlOffsetX,
  ctrlInputX = 550 + controlOffsetX,
  interfaceRegsX = 175,                          // x-center of MARB, MARA, ...
  combCircX = interfaceRegsX - iRegXOffset - 20, // Combinational circuits need to be moved further left to fit.
  combCircY = 157,                               // Memory Combinational circuits start at this height. Originally 132
  statusBitsX = 526,                             // 476,
  BottomOfAlu = OneByteShapes::ALUBottomBound + aluOffsetY, // Y coordinate of the bottom of the ALU
  ALUUpperRightLineMidpoint =
      (OneByteShapes::ALUUpperRightLine_LeftPoint + OneByteShapes::ALUUpperRightLine_RightPoint) / 2 + controlOffsetX,
  bus_bRightArrowTipX = ctrlInputX, // How far the B bus goes in the direction of the control section of the CPU.
  DataArrowMidpointY = 380,
  DataArrowLeftX = 3,
  DataArrowRightX = 39,
  DataArrowDepth = 15,
};

// Enumeration that controls the distance between certain items in the diagram. Hopefully this makes spacing easier to
// adjust.
enum CommonOffsets {
  AMuxYOffsetFrompoly_alu = 40,  // The number of pixels between AMux and the ALU Polygon
  MARMUXOffestFromMARA = 25,     // Number of pixels between MARMux and (MARA, MARB) horizontally.
  MARAOffsetFromMARB = 60,       // Number of pixels vertically between MARA and MARB
  MDREOffsetFromCombY = 115,     // Number of pixels vertically between MDRO register and the combCircY origin.
  MDRORegOffsetFromMDREMux = 52, // Number of pixels vertically between MDROMux and MDREMux
  MDRRegOffsetFromMDRMux =
      20,                 // Number of pixels between the bottom of MDRO,MDRE registers and the top of MDROMux, MDREMux
  SCKYOffsetFromALU = 43, // Number of pixel between bottom of ALU and top of SCk controls
  CCkYOffsetFromALU = 70, // Bottom of ALU and top of CCk
  VCkYOffsetFromALU = 97, // Bottom of ALU and top of VCk
  ANDZYOffsetFromALU = 123,     // Bottom of ALU and top of ANDZ
  ZCkYOffsetFromALU = 150,      // Bottom of ALU and top of ZCk
  NCkYOffsetFromALU = 192,      // Bottom of ALU and top of NCk
  MemReadYOffsetFromALU = 237,  // Bottom of ALU to the MemReadLine
  MemWriteYOffsetFromALU = 217, // Bottom of ALU to the MemWriteLine
  ALULabelYOffsetFromALU = -25, // Bottom of ALU to top of the ALULineEdit
  EOMuxOffsetFromMDREMux = 10,  // Top of MDREMux to top of EOMux
  bus_cToMDREMuxLength = 60,    // Number of pixels between the branch of C bus and MDREMux
  DataArrowOuterYSpread = 15,
  DataArrowInnerYSpread = 10,

};

// registers
const QRect poly_regbank = OneByteShapes::poly_regbank;
const QRect aRegLineEdit = OneByteShapes::aRegLineEdit;
const QRect xRegLineEdit = OneByteShapes::xRegLineEdit;
const QRect spRegLineEdit = OneByteShapes::spRegLineEdit;
const QRect pcRegLineEdit = OneByteShapes::pcRegLineEdit;
const QRect irRegLineEdit = OneByteShapes::irRegLineEdit;
const QRect t1RegLineEdit = OneByteShapes::t1RegLineEdit;
const QRect t2RegLineEdit = OneByteShapes::t2RegLineEdit;
const QRect t3RegLineEdit = OneByteShapes::t3RegLineEdit;
const QRect t4RegLineEdit = OneByteShapes::t4RegLineEdit;
const QRect t5RegLineEdit = OneByteShapes::t5RegLineEdit;
const QRect t6RegLineEdit = OneByteShapes::t6RegLineEdit;
const QRect m1RegLabel = OneByteShapes::m1RegLabel;
const QRect m2RegLabel = OneByteShapes::m2RegLabel;
const QRect m3RegLabel = OneByteShapes::m3RegLabel;
const QRect m4RegLabel = OneByteShapes::m4RegLabel;
const QRect m5RegLabel = OneByteShapes::m5RegLabel;

// ALU
const QPolygon poly_alu = OneByteShapes::poly_alu.translated(controlOffsetX, aluOffsetY);

// input/label/control section:
const QRect bus_addr = QRect(QPoint(40, 151), QPoint(40 + 20, BottomOfAlu + MemReadYOffsetFromALU + 15));
const QRect bus_data = QRect(QPoint(bus_addr.x() + bus_addr.width(), bus_addr.top() + 100),
                            QPoint(bus_addr.x() + bus_addr.width() + 20, BottomOfAlu + MemReadYOffsetFromALU + 15));

// LoadCk and its control
const QRect loadCkCheckbox = QRect(ctrlInputX, 18, check2W, check2H);
const Arrow ck_load =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset, 24)},
          QVector<QLine>() << QLine(ctrlInputX - 7, loadCkCheckbox.y() + selectYOffset,
                                    poly_regbank.right() + arrowHOffset, loadCkCheckbox.y() + selectYOffset));

// C and its control
const QRect cLineEdit = QRect(ctrlInputX, 39, lineEditW, lineEditH);
const QRect cLabel = QRect(ctrlLabelX, 41, labelW, labelH);
const Arrow sel_c =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset, 47)},
          QVector<QLine>() << QLine(ctrlInputX - 7, cLabel.y() + selectYOffset, poly_regbank.right() + arrowHOffset,
                                    cLabel.y() + selectYOffset)
                           << QLine(poly_regbank.right() + arrowHDepth + 6, cLabel.y() + selectYOffset - 5,
                                    poly_regbank.right() + arrowHDepth + 16, cLabel.y() + selectYOffset + 5));

// B and its control
const QRect bLineEdit = QRect(ctrlInputX, 61, lineEditW, lineEditH);
const QRect bLabel = QRect(ctrlLabelX, 63, labelW, labelH);
const Arrow sel_b = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset, 69)},
    QVector<QLine>() << QLine(ctrlInputX - 7, bLabel.y() + selectYOffset, poly_regbank.right() + arrowHOffset,
                              bLabel.y() + selectYOffset)
                     << QLine(poly_regbank.right() + arrowHDepth + 6, bLabel.y() + selectYOffset - selectSlashOffset,
                              poly_regbank.right() + arrowHDepth + 16, bLabel.y() + selectYOffset + selectSlashOffset));

// A and its control
const QRect aLineEdit = QRect(ctrlInputX, 83, lineEditW, lineEditH);
const QRect aLabel = QRect(ctrlLabelX, 85, labelW, labelH);
const Arrow sel_a = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(poly_regbank.right() + arrowHOffset, 91)},
    QVector<QLine>() << QLine(ctrlInputX - 7, aLabel.y() + selectYOffset, poly_regbank.right() + arrowHOffset,
                              aLabel.y() + selectYOffset)
                     << QLine(poly_regbank.right() + arrowHDepth + 6, aLabel.y() + selectYOffset - selectSlashOffset,
                              poly_regbank.right() + arrowHDepth + 16, aLabel.y() + selectYOffset + selectSlashOffset));

// MARMux and its control
const QRect poly_marmux =
    QRect((combCircX + dataLabelW) + MARMUXOffestFromMARA, combCircY, dataLabelH + MARAOffsetFromMARB,
          dataLabelH + MARAOffsetFromMARB); // 89 x 89 square from bottom of MARA to top of MARB
const QRect MARMuxTristateLabel = QRect(ctrlInputX, poly_marmux.y() - 28, labelTriW, labelTriH);
const QRect MARMuxLabel = QRect(ctrlLabelX, MARMuxTristateLabel.y(), labelW + 20, labelH);
const Arrow sel_mux_mar = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(poly_marmux.x() + poly_marmux.width() / 2 - 3, poly_marmux.y() - 12),
                                      DOWN},
    QVector<QLine>()
        // Horizontal line from middle of MARMux to the tristate label
        << QLine(poly_marmux.x() + poly_marmux.width() / 2, MARMuxTristateLabel.y() + MARMuxTristateLabel.height() / 2,
                 ctrlInputX - 7, MARMuxTristateLabel.y() + MARMuxTristateLabel.height() / 2)
        // Vertical line connecting the arrowhead to the horizontal line
        << QLine(poly_marmux.x() + poly_marmux.width() / 2, MARMuxTristateLabel.y() + MARMuxTristateLabel.height() / 2,
                 poly_marmux.x() + poly_marmux.width() / 2, poly_marmux.y() - 12));

// ck_mar and its control
const QRect ck_marCheckbox =
    QRect(ctrlInputX, poly_marmux.y() + poly_marmux.height() / 2 - check2H / 2, check2W, check2H);
const QRect reg_byte_mara = QRect(combCircX, combCircY + MARAOffsetFromMARB, dataLabelW, dataLabelH); // MARA register.
const QRect reg_byte_marb = QRect(combCircX, combCircY, dataLabelW, dataLabelH);                      // MARB register

const Arrow ck_mar =
    Arrow(QVector<Arrowhead>()
              // The Arrows intersecting MAR,MARB should be roughly 5/7 of the way down the circuits.
              << Arrowhead{QPoint(combCircX + 5 * dataLabelW / 7 + 7, combCircY + dataLabelH + 3), UP}
              << Arrowhead{QPoint(combCircX + 5 * dataLabelW / 7 + 7, combCircY + MARAOffsetFromMARB - 11), DOWN},
          QVector<QLine>()
              // Horizontal line segment between MAR{A,B} and ck_mar
              << QLine(combCircX + 5 * dataLabelW / 7 + 10, combCircY + MARAOffsetFromMARB - dataLabelH, ctrlInputX - 7,
                       combCircY + MARAOffsetFromMARB - dataLabelH)
              // The vertical line intersecting MAR,MARB should be roughly 5/7 of the way down the circuits.
              << QLine(combCircX + 5 * dataLabelW / 7 + 10, combCircY + dataLabelH + 3,
                       combCircX + 5 * dataLabelW / 7 + 10, combCircY + MARAOffsetFromMARB - 3));
// MARMux output busses
const QPolygon bux_marmux_to_mara = QPolygon(
    QVector<QPoint>() << QPoint(poly_marmux.x(),
                                reg_byte_mara.y() + reg_byte_mara.height() / 2 - 5) // Foot Top Right point
                      << QPoint(poly_marmux.x(),
                                reg_byte_mara.y() + reg_byte_mara.height() / 2 + 5) // Foot Bottom Right point
                      << QPoint(reg_byte_mara.right() + arrowHDepth - 5,
                                reg_byte_mara.y() + reg_byte_mara.height() / 2 + 5) // Arrow Bottom Inner point
                      << QPoint(reg_byte_mara.right() + arrowHDepth - 5,
                                reg_byte_mara.y() + reg_byte_mara.height() / 2 + 10) // Arrow Bottom Outer point
                      << QPoint(reg_byte_mara.right() + arrowHOffset,
                                reg_byte_mara.y() + reg_byte_mara.height() / 2) // Arrow Middle point
                      << QPoint(reg_byte_mara.right() + arrowHDepth - 5,
                                reg_byte_mara.y() + reg_byte_mara.height() / 2 - 10) // Arrow Top Outer point
                      << QPoint(reg_byte_mara.right() + arrowHDepth - 5,
                                reg_byte_mara.y() + reg_byte_mara.height() / 2 - 5)); // Arrow Top Inner point

const QPolygon bus_marmux_to_marb = QPolygon(
    QVector<QPoint>() << QPoint(poly_marmux.x(),
                                reg_byte_marb.y() + reg_byte_marb.height() / 2 - 5) // Foot Top Right point
                      << QPoint(poly_marmux.x(),
                                reg_byte_marb.y() + reg_byte_marb.height() / 2 + 5) // Bottom Right point
                      << QPoint(reg_byte_marb.right() + arrowHDepth - 5,
                                reg_byte_marb.y() + reg_byte_marb.height() / 2 + 5) // Arrow Bottom Inner point
                      << QPoint(reg_byte_marb.right() + arrowHDepth - 5,
                                reg_byte_marb.y() + reg_byte_marb.height() / 2 + 10) // Arrow Bottom Outer point
                      << QPoint(reg_byte_marb.right() + arrowHOffset,
                                reg_byte_marb.y() + reg_byte_marb.height() / 2) // Arrow Middle point
                      << QPoint(reg_byte_marb.right() + arrowHDepth - 5,
                                reg_byte_marb.y() + reg_byte_marb.height() / 2 - 10) // Arrow Top Outer point
                      << QPoint(reg_byte_marb.right() + arrowHDepth - 5,
                                reg_byte_marb.y() + reg_byte_marb.height() / 2 - 5)); // Arrow Top Inner point

// MDROdd, ck_mdro and its control
const QRect reg_byte_mdro = QRect(combCircX, combCircY + MDREOffsetFromCombY, dataLabelW, dataLabelH);
const QRect ck_mdroCheckbox = QRect(ctrlInputX, reg_byte_mdro.y() - 25, checkW + 10, checkH);
const Arrow ck_mdro = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(reg_byte_mdro.x() + reg_byte_mdro.width() / 2 - 3, reg_byte_mdro.y() - 12),
                                      DOWN},
    QVector<QLine>()
        // Horizontal line from MDRO checkbox to center of MDRO
        << QLine(reg_byte_mdro.x() + reg_byte_mdro.width() / 2, ck_mdroCheckbox.y() + ck_mdroCheckbox.height() / 2,
                 ctrlInputX - 7, ck_mdroCheckbox.y() + ck_mdroCheckbox.height() / 2)
        // Vertical line between arrowhead and horizontal line from checkbox
        << QLine(reg_byte_mdro.x() + reg_byte_mdro.width() / 2, ck_mdroCheckbox.y() + ck_mdroCheckbox.height() / 2,
                 reg_byte_mdro.x() + reg_byte_mdro.width() / 2, reg_byte_mdro.y() - 12));

// MDROMux and its control
const QRect mux_mdro = QRect(combCircX, reg_byte_mdro.bottom() + MDRRegOffsetFromMDRMux, dataLabelW, dataLabelH);
const QRect MDROMuxTristateLabel = QRect(ctrlInputX, mux_mdro.y(), labelTriW, labelTriH);
const QRect MDROMuxLabel = QRect(ctrlLabelX, MDROMuxTristateLabel.y(), labelW + 20, labelH);
const Arrow sel_mux_mdro =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(mux_mdro.right() + 5,
                                                   MDROMuxTristateLabel.y() + MDROMuxTristateLabel.height() / 2 - 3)},
          QVector<QLine>()
              // Horizontal line between MDROMux and its tristate label
              << QLine(ctrlInputX - 7, MDROMuxTristateLabel.y() + MDROMuxTristateLabel.height() / 2,
                       mux_mdro.right() + 5, MDROMuxTristateLabel.y() + MDROMuxTristateLabel.height() / 2));

// MDREven, ck_mdre and its control
const QRect reg_byte_mdre = QRect(combCircX, mux_mdro.bottom() + MDRORegOffsetFromMDREMux, dataLabelW, dataLabelH);
const QRect ck_mdreCheckbox = QRect(ctrlInputX, reg_byte_mdre.y() - 40, checkW + 10, checkH);
const Arrow ck_mdre = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(reg_byte_mdre.x() + reg_byte_mdre.width() / 2 - 3, reg_byte_mdre.y() - 12),
                                      DOWN},
    QVector<QLine>()
        // Horizontal line from ck_mdre to midpoint of MDREven
        << QLine(ctrlInputX - 7, ck_mdreCheckbox.y() + ck_mdreCheckbox.height() / 2,
                 reg_byte_mdre.x() + reg_byte_mdre.width() / 2, ck_mdreCheckbox.y() + ck_mdreCheckbox.height() / 2)
        // Vertical line connecting arrowhead and horizontal line segment
        << QLine(reg_byte_mdre.x() + reg_byte_mdre.width() / 2, ck_mdreCheckbox.y() + ck_mdreCheckbox.height() / 2,
                 reg_byte_mdre.x() + reg_byte_mdre.width() / 2, reg_byte_mdre.y() - 12));

// MDREMux and its control
const QRect mux_mdre = QRect(combCircX, reg_byte_mdre.bottom() + MDRRegOffsetFromMDRMux, dataLabelW, dataLabelH);
const QRect MDREMuxTristateLabel = QRect(ctrlInputX, mux_mdre.y() - 25, labelTriW, labelTriH);
const QRect MDREMuxLabel = QRect(ctrlLabelX, MDREMuxTristateLabel.y(), labelW + 20, labelH);
const Arrow sel_mux_mdre =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(mux_mdre.right() + 5, mux_mdre.y() + mux_mdre.height() / 2 - 3)},
          QVector<QLine>()
              // Horizontal leg extending from ck_mdro
              << QLine(ctrlInputX - 7, MDREMuxTristateLabel.y() + MDREMuxTristateLabel.height() / 2,
                       mux_mdre.right() + 25, MDREMuxTristateLabel.y() + MDREMuxTristateLabel.height() / 2)
              // Vertical line segment
              << QLine(mux_mdre.right() + 25, MDREMuxTristateLabel.y() + MDREMuxTristateLabel.height() / 2,
                       mux_mdre.right() + 25, mux_mdre.y() + mux_mdre.height() / 2)
              // Horizonal line segment connecting arrowhead and vertical line
              << QLine(mux_mdre.right() + 25, mux_mdre.y() + mux_mdre.height() / 2, mux_mdre.right() + 5,
                       mux_mdre.y() + mux_mdre.height() / 2));
// EOMux and its control
const QRect mux_eo =
    QRect(poly_marmux.x() + poly_marmux.width() / 2 - dataLabelW / 2, // Center EOMux horizontally on MARMux
          mux_mdre.y() + EOMuxOffsetFromMDREMux, dataLabelW, dataLabelH);
const QRect EOMuxTristateLabel = QRect(ctrlInputX, mux_eo.y(), labelTriW, labelTriH);
const QRect EOMuxLabel = QRect(ctrlLabelX, EOMuxTristateLabel.y(), labelW, labelH);
const Arrow sel_muxeo = Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(mux_eo.right() + 4, EOMuxTristateLabel.y() + 6)},
                              QVector<QLine>() << QLine(mux_eo.right() + 5, mux_eo.y() + mux_eo.height() / 2,
                                                        ctrlInputX - 7, mux_eo.y() + mux_eo.height() / 2));

// ALU and its control
const QRect ALULineEdit = QRect(ctrlInputX, BottomOfAlu + ALULabelYOffsetFromALU, 26, lineEditH);
const QRect ALULabel = QRect(ctrlLabelX, BottomOfAlu + ALULabelYOffsetFromALU, 31, labelH);
const QRect ALUFunctionLabel = OneByteShapes::ALUFunctionLabel.translated(controlOffsetX, aluOffsetY);

// CSMux and its control
const QRect CSMuxLabel = QRect(ctrlLabelX, BottomOfAlu + 5, labelW, labelH);
const QRect mux_cs = QRect(statusBitsX + 19 - 69, BottomOfAlu + 5, dataLabelW, dataLabelH);
const QRect CSMuxTristateLabel = QRect(ctrlInputX, BottomOfAlu + 5, 25, 21);

// CMux and its control
const QRect mux_c = OneByteShapes::mux_c.translated(controlOffsetX, aluOffsetY);
const QRect cMuxTristateLabel = QRect(ctrlInputX, ALULineEdit.y() - labelTriH - 4, labelTriW, labelTriH);
const QRect cMuxLabel = QRect(ctrlLabelX, cMuxTristateLabel.y(), labelW, labelH);
const Arrow sel_muxc =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(mux_c.left() + 7, mux_c.top() - 12), DOWN},
          QVector<QLine>() << QLine(mux_cs.left() + 20, cMuxTristateLabel.y() + cMuxTristateLabel.height() / 2,
                                    ctrlInputX - 7, cMuxTristateLabel.y() + cMuxTristateLabel.height() / 2)
                           << QLine(mux_c.right() - 5, cMuxTristateLabel.y() + cMuxTristateLabel.height() / 2,
                                    mux_c.left() + 10, cMuxTristateLabel.y() + cMuxTristateLabel.height() / 2)
                           << QLine(mux_c.left() + 10, cMuxTristateLabel.y() + cMuxTristateLabel.height() / 2,
                                    mux_c.left() + 10, mux_c.top() - 12));

// Status bit S, SCk and its control
const QRect SCkCheckBox = QRect(ctrlInputX, BottomOfAlu + SCKYOffsetFromALU, checkW, checkH);
const QRect reg_bit_s = QRect(statusBitsX, BottomOfAlu + SCKYOffsetFromALU, 19, dataLabelH);

// Status bit C, CCk and its control
const QRect CCkCheckBox = QRect(ctrlInputX, BottomOfAlu + CCkYOffsetFromALU, checkW, checkH);
const QRect reg_bit_c = QRect(statusBitsX, BottomOfAlu + CCkYOffsetFromALU - 1, 19, dataLabelH);

// Status bit V, VCk and its control
const QRect VCkCheckBox = QRect(ctrlInputX, BottomOfAlu + VCkYOffsetFromALU, checkW, checkH);
const QRect reg_bit_v = QRect(statusBitsX, BottomOfAlu + VCkYOffsetFromALU, 19, dataLabelH);

// AndZ and its control
const QRect AndZLabel = QRect(ctrlLabelX, BottomOfAlu + ANDZYOffsetFromALU, 45, 20);
const QRect AndZTristateLabel = QRect(ctrlInputX, BottomOfAlu + ANDZYOffsetFromALU, labelTriW, labelTriH);
const QRect mux_andz = QRect(416 + controlOffsetX, BottomOfAlu + ANDZYOffsetFromALU + 27, 41, 21);

// Status bit Z, ZCk and its control
const QRect ZCkCheckBox = QRect(ctrlInputX, BottomOfAlu + ZCkYOffsetFromALU, 60, 20);
const QRect reg_bit_z = QRect(statusBitsX, BottomOfAlu + ZCkYOffsetFromALU, 19, dataLabelH);

// Status bit N, NCk and its control
const QRect NCkCheckBox = QRect(ctrlInputX, BottomOfAlu + NCkYOffsetFromALU, checkW, checkH);
const QRect reg_bit_n = QRect(statusBitsX, BottomOfAlu + NCkYOffsetFromALU, 19, dataLabelH);

// MemWrite and its control
const QRect MemWriteLabel = QRect(ctrlLabelX, BottomOfAlu + MemWriteYOffsetFromALU, check2W, check2H);
const QRect MemWriteTristateLabel = QRect(ctrlInputX, BottomOfAlu + MemWriteYOffsetFromALU, labelTriW, labelTriH);

// MemRead and its control
const QRect MemReadLabel = QRect(ctrlLabelX, BottomOfAlu + MemReadYOffsetFromALU, check2W, check2H);
const QRect MemReadTristateLabel = QRect(ctrlInputX, BottomOfAlu + MemReadYOffsetFromALU, labelTriW, labelTriH);

const auto mar_base_x = reg_byte_mara.x() + reg_byte_mara.width() / 2;
const auto mar_base_y = (reg_byte_marb.bottom() + reg_byte_mara.y()) / 2;
const QPolygon bus_mar_to_addr =
    QPolygon(QVector<QPoint>()
             // Top Foot
             << QPoint(mar_base_x - 5, reg_byte_marb.bottom() + 1) // Foot Top Left point
             << QPoint(mar_base_x - 5,
                       mar_base_y - 10) // Pivot between foot right corner and arrow inner point.
             // arrow:
             << QPoint(bus_addr.right() + arrowHDepth,
                       mar_base_y - 10) // Arrow Top Inner Point
             << QPoint(bus_addr.right() + arrowHDepth,
                       mar_base_y - 15) // Arrow Top Outer Point
             << QPoint(bus_addr.right() + arrowHOffset,
                       mar_base_y + 1) // Arrow Middle Point
             << QPoint(bus_addr.right() + arrowHDepth,
                       mar_base_y + 16) // Arrow Bottom Outer Point
             << QPoint(bus_addr.right() + arrowHDepth,
                       mar_base_y + 10) // Arrow Bottom Inner Point
             // Bottom Foot
             << QPoint(mar_base_x - 5,
                       mar_base_y + 10)                   // Pivot between bottom right corner and bottom inner point
             << QPoint(mar_base_x - 5, reg_byte_mara.y()) // Foot Bottom Left point
             << QPoint(mar_base_x + 5, reg_byte_mara.y()) // Foot Bottom Right point
             // Black Line
             << QPoint(mar_base_x + 5,
                       mar_base_y) // Black line right point
             << QPoint(bus_addr.right() + arrowHDepth,
                       mar_base_y) // Black line left point
             << QPoint(mar_base_x + 5,
                       mar_base_y) // Black line right point
             // Top Foot
             << QPoint(mar_base_x + 5, reg_byte_marb.bottom() + 1)); // Foot Top Right point

const QPolygon bus_nzvc_to_cmux = OneByteShapes::bus_nzvc_to_cmux.translated(controlOffsetX, aluOffsetY);

// AMux, its controls, selection lines, and output.
const QRect mux_a =
    QRect(((controlOffsetX + OneByteShapes::ALUUpperLeftLine_LeftPoint) +
           (controlOffsetX + OneByteShapes::ALUUpperLeftLine_RightPoint)) /
                  2 -
              dataLabelW / 2,
          // Center AMUX's x on the midpoint of the poly_alugon, which has been shifted by controlOffsetX pixels.
          poly_alu.boundingRect().y() - AMuxYOffsetFrompoly_alu, dataLabelW,
          dataLabelH); // Place AMuxYOffsetFrompoly_alu pixels distance between AMux and the ALU

const QRect aMuxTristateLabel = QRect(ctrlInputX, mux_a.y(), labelTriW, 21);
const QRect aMuxLabel = QRect(ctrlLabelX, aMuxTristateLabel.y(), labelW, labelH);
const Arrow sel_muxa =
    Arrow(QVector<Arrowhead>()
              // Place the arrowhead slightly off-centered from AMux, otherwise it is visually odd.
              << Arrowhead{QPoint(mux_a.x() + mux_a.width() + 3, mux_a.y() + mux_a.height() / 2 - 2)},
          // Draw a line from the aMuxTristateLabel to AMux, and center the line vertically between the two.
          // Add one to the calculated y coordinates, otherwise the line and arrow don't appear to be centered.
          QVector<QLine>() << QLine(ctrlInputX - 7, mux_a.y() + mux_a.height() / 2 + 1,
                                    // Add 5 to the x coordinate, otherwise the line extends past the arrow.
                                    mux_a.x() + mux_a.width() + 5, mux_a.y() + mux_a.height() / 2 + 1));

// EOMux bus definition is split from EOMux, because it depends on AMUX, which depends on the ALU
const QPolygon bus_eomux_to_amux =
    QPolygon(QVector<QPoint>()
             // Foot
             << QPoint(mux_eo.x() + mux_eo.width() / 2 - 5,
                       mux_eo.bottom() + 1) // Foot Top Left point
             << QPoint(mux_eo.x() + mux_eo.width() / 2 + 5,
                       mux_eo.bottom() + 1) // Foot Top Right point
             << QPoint(mux_eo.x() + mux_eo.width() / 2 + 5,
                       mux_eo.bottom() + 10) // Pivot between upper right vertical leg and upper horizontal leg
             << QPoint(mux_a.x() + 15,
                       mux_eo.bottom() + 10) // Pivot between upper horizontal leg, and the right vertical leg
             // Arrow
             << QPoint(mux_a.x() + 15, mux_a.y() - (arrowHDepth - 5)) // Arrow Right Inner point
             << QPoint(mux_a.x() + 20, mux_a.y() - (arrowHDepth - 5)) // Arrow Right Outer point
             << QPoint(mux_a.x() + 10, mux_a.y() - arrowHOffset)      // Arrow Middle  point
             << QPoint(mux_a.x(), mux_a.y() - (arrowHDepth - 5))      // Arrow Left Outer point
             << QPoint(mux_a.x() + 5, mux_a.y() - (arrowHDepth - 5))  // Arrow Left Inner point
             // Remainder of Foot
             << QPoint(mux_a.x() + 5,
                       mux_eo.bottom() + 20) // Pivot between arrow left side and lower horizontal leg
             << QPoint(mux_eo.x() + mux_eo.width() / 2 - 5,
                       mux_eo.bottom() + 20) // Pivot between lower horizontal leg and start
    );

const QPolygon bus_amux_to_alu =
    QPolygon(QVector<QPoint>() << QPoint(mux_a.x() + mux_a.width() / 2 - 5,
                                         mux_a.y() + mux_a.height()) // Foot Upper Left point
                               << QPoint(mux_a.x() + mux_a.width() / 2 + 5,
                                         mux_a.y() + mux_a.height()) // Upper Right point
                               << QPoint(mux_a.x() + mux_a.width() / 2 + 5,
                                         poly_alu.boundingRect().y() - (arrowHDepth - 5)) // Arrow Right Inner point
                               << QPoint(mux_a.x() + mux_a.width() / 2 + 10,
                                         poly_alu.boundingRect().y() - (arrowHDepth - 5)) // Arrow Right Outer point
                               << QPoint(mux_a.x() + mux_a.width() / 2,
                                         poly_alu.boundingRect().y() - arrowHOffset) // Arrow Middle point
                               << QPoint(mux_a.x() + mux_a.width() / 2 - 10,
                                         poly_alu.boundingRect().y() - (arrowHDepth - 5)) // Arrow Left Outer point
                               << QPoint(mux_a.x() + mux_a.width() / 2 - 5,
                                         poly_alu.boundingRect().y() - (arrowHDepth - 5)) // Arrow Left Inner point
    );

const QPolygon bus_a =
    QPolygon(QVector<QPoint>()
             << QPoint(mux_a.right() - 15, poly_regbank.bottom() + 1)     // Top left corner of register foot
             << QPoint(mux_a.right() - 5, poly_regbank.bottom() + 1)      // Top right corner of register foot;
             << QPoint(mux_a.right() - 5, mux_a.y() - (arrowHDepth - 5))  // AMux Arrow Right Inner point
             << QPoint(mux_a.right() - 0, mux_a.y() - (arrowHDepth - 5))  // AMux Arrow Right Outer point
             << QPoint(mux_a.right() - 10, mux_a.y() - arrowHOffset)      // AMux Arrow Middle point
             << QPoint(mux_a.right() - 20, mux_a.y() - (arrowHDepth - 5)) // AMux Arrow Left Outer point
             << QPoint(mux_a.right() - 15, mux_a.y() - (arrowHDepth - 5)) // AMux Arrow Left Inner point
             << QPoint(mux_a.right() - 15,
                       poly_marmux.bottom() - 5) // Pivot between AMUX arrow left point and MARMux Inner Bottom Edge
             << QPoint(poly_marmux.right() + (arrowHDepth - 5),
                       poly_marmux.bottom() - 5) // MARMux Arrow Bottom Inner point
             << QPoint(poly_marmux.right() + (arrowHDepth - 5),
                       poly_marmux.bottom() - 0) // MARMux Arrow Bottom Outer point
             << QPoint(poly_marmux.right() + (arrowHOffset), poly_marmux.bottom() - 10) // MARMux Arrow Middle point
             << QPoint(poly_marmux.right() + (arrowHDepth - 5),
                       poly_marmux.bottom() - 20) // MARMux Arrow Top Outer point
             << QPoint(poly_marmux.right() + (arrowHDepth - 5),
                       poly_marmux.bottom() - 15) // MARMux Arrow Top Inner point
             << QPoint(mux_a.right() - 15,
                       poly_marmux.bottom() - 15) // Pivot between MARMux arrow top and register top left foot
    );
const QPolygon bus_b = QPolygon(
    QVector<QPoint>()
    << QPoint(ALUUpperRightLineMidpoint - 5, poly_regbank.bottom() + 1) // Top left corner of register foot
    << QPoint(ALUUpperRightLineMidpoint + 5, poly_regbank.bottom() + 1) // Top right corner of register foot
    << QPoint(ALUUpperRightLineMidpoint + 5, poly_marmux.y() + 5)       // Pivot between register foor and right output
    //<< QPoint(bus_bRightArrowTipX-(arrowHDepth-5), poly_marmux.y()+5) // Right Out Arrow Top Inner point
    //<< QPoint(bus_bRightArrowTipX-(arrowHDepth-5),poly_marmux.y()+0) // Right Out Arrow Top Outer point
    //<< QPoint(bus_bRightArrowTipX-(arrowHOffset),poly_marmux.y()+10) // Right Out Arrow Middle point
    //<< QPoint(bus_bRightArrowTipX-(arrowHDepth-5),poly_marmux.y()+20) // Right Out Arrow Botton Outer point
    //<< QPoint(bus_bRightArrowTipX-(arrowHDepth-5),poly_marmux.y()+15) // Right Out Arrow Bottom Inner point
    //<< QPoint(ALUUpperRightLineMidpoint+5,poly_marmux.y()+15) // Pivot between right out arrow and alu arrow
    << QPoint(ALUUpperRightLineMidpoint + 5,
              poly_alu.boundingRect().y() - (arrowHDepth - 5)) // ALU Arrow Right Inner point
    << QPoint(ALUUpperRightLineMidpoint + 10,
              poly_alu.boundingRect().y() - (arrowHDepth - 5)) // ALU Arrow Right Outer point
    << QPoint(ALUUpperRightLineMidpoint + 0, poly_alu.boundingRect().y() - (arrowHOffset)) // ALU Arrow Middle point
    << QPoint(ALUUpperRightLineMidpoint - 10,
              poly_alu.boundingRect().y() - (arrowHDepth - 5)) // ALU Arrow Left Outer point
    << QPoint(ALUUpperRightLineMidpoint - 5,
              poly_alu.boundingRect().y() - (arrowHDepth - 5))     // ALU Arrow Left Inner point
    << QPoint(ALUUpperRightLineMidpoint - 5, poly_marmux.y() + 15) // Pivot between ALU arrow and MARMux Arrow
    << QPoint(poly_marmux.right() + (arrowHDepth - 5),
              poly_marmux.y() + 15) // MARMux Arrow Bottom Inner point
    << QPoint(poly_marmux.right() + (arrowHDepth - 5),
              poly_marmux.y() + 20)                                         // MARMux Arrow Bottom Outer point
    << QPoint(poly_marmux.right() + (arrowHOffset), poly_marmux.y() + 10)   // MARMux Arrow Middle point
    << QPoint(poly_marmux.right() + (arrowHDepth - 5), poly_marmux.y() + 0) // MARMux Arrow Top Outer point
    << QPoint(poly_marmux.right() + (arrowHDepth - 5), poly_marmux.y() + 5) // MARMux Arrow Top Inner point
    << QPoint(ALUUpperRightLineMidpoint - 5, poly_marmux.y() + 5)           // Pivot between MARMux and register foot
);
const QPolygon bus_c = QPolygon(
    QVector<QPoint>()
    << QPoint(mux_c.x() + mux_c.width() / 2 + 5, mux_c.y()) // CMux Right foot
    << QPoint(mux_c.x() + mux_c.width() / 2 - 5, mux_c.y()) // CMux Left foot
    // Branch off to MDREven
    << QPoint(mux_c.x() + mux_c.width() / 2 - 5,
              mux_mdre.bottom() + (arrowHDepth - 5) +
                  bus_cToMDREMuxLength) // Pivot between CMux foot and MDRE lower Leg
    << QPoint(mux_mdre.right() - 15,
              mux_mdre.bottom() + (arrowHDepth - 5) +
                  bus_cToMDREMuxLength) // Pivot between MDRE lower leg and MDRE left arrow
    << QPoint(mux_mdre.right() - 15,
              mux_mdre.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Left Inner Point
    << QPoint(mux_mdre.right() - 20,
              mux_mdre.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Left Outer Point
    << QPoint(mux_mdre.right() - 10,
              mux_mdre.bottom() + (arrowHOffset)) // MDREMux Arrow Middle Point
    << QPoint(mux_mdre.right() - 0,
              mux_mdre.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Right Outer Point
    << QPoint(mux_mdre.right() - 5,
              mux_mdre.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Right Inner Point
    << QPoint(mux_mdre.right() - 5, mux_mdre.bottom() + (arrowHDepth - 5) + bus_cToMDREMuxLength -
                                        10) // Pivot between MDRE right arrow and MDRE upper leg
    << QPoint(mux_c.x() + mux_c.width() / 2 - 5, mux_mdre.bottom() + (arrowHDepth - 5) + bus_cToMDREMuxLength -
                                                     10) // Pivot between MDRE upper leg and leg upwards
    // Branch off to MDROdd
    << QPoint(mux_c.x() + mux_c.width() / 2 - 5,
              mux_mdro.bottom() + (arrowHDepth - 5) + 20 + 5) // Pivot between CMux foot and MDRE lower Leg
    << QPoint(mux_mdro.right() - 15,
              mux_mdro.bottom() + (arrowHDepth - 5) + 20 + 5) // Pivot between MDRE lower leg and MDRE left arrow
    << QPoint(mux_mdro.right() - 15,
              mux_mdro.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Left Inner Point
    << QPoint(mux_mdro.right() - 20,
              mux_mdro.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Left Outer Point
    << QPoint(mux_mdro.right() - 10,
              mux_mdro.bottom() + (arrowHOffset)) // MDREMux Arrow Middle Point
    << QPoint(mux_mdro.right() - 0,
              mux_mdro.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Right Outer Point
    << QPoint(mux_mdro.right() - 5,
              mux_mdro.bottom() + (arrowHDepth - 5)) // MDREMux Arrow Right Inner Point
    << QPoint(mux_mdro.right() - 5,
              mux_mdro.bottom() + (arrowHDepth - 5) + 10 + 5) // Pivot between MDRE right arrow and MDRE upper leg
    << QPoint(mux_c.x() + mux_c.width() / 2 - 5,
              mux_mdro.bottom() + (arrowHDepth - 5) + 10 + 5) // Pivot between MDRE upper leg and leg upwards
    // Resume path to register bank's arrow
    << QPoint(mux_c.x() + mux_c.width() / 2 - 5,
              poly_regbank.bottom() + (arrowHDepth - 5)) // Register Arrow Left Inner Point
    << QPoint(mux_c.x() + mux_c.width() / 2 - 10,
              poly_regbank.bottom() + (arrowHDepth - 5)) // Register Arrow Left Outer Point
    << QPoint(mux_c.x() + mux_c.width() / 2 + 0,
              poly_regbank.bottom() + (arrowHOffset)) // Register Arrow Middle Point
    << QPoint(mux_c.x() + mux_c.width() / 2 + 10,
              poly_regbank.bottom() + (arrowHDepth - 5)) // Register Arrow Right Outer Point
    << QPoint(mux_c.x() + mux_c.width() / 2 + 5,
              poly_regbank.bottom() + (arrowHDepth - 5)) // Register Arrow Right Inner Point
);
const QPolygon bus_addr_to_ddr = OneByteShapes::bus_addr_to_ddr;
// const QPolygon bus_data_to_mdrmux;
const QPolygon bus_data_to_mdromux =
    QPolygon(QVector<QPoint>()
             // Foot:
             << QPoint(mux_mdro.x() + 15, mux_mdro.bottom() + (arrowHDepth - 5) + 18 +
                                              5) // Point between vertical right leg and lower horizontal leg
             << QPoint(80 + 2, mux_mdro.bottom() + (arrowHDepth - 5) + 18 + 5) // bus_data Foot Bottom point
             << QPoint(80 + 2, mux_mdro.bottom() + (arrowHDepth - 5) + 8 + 5)  // bus_data Foot Top point
             << QPoint(mux_mdro.x() + 5, mux_mdro.bottom() + (arrowHDepth - 5) + 8 +
                                             5) // Point between vertical left leg and upper horizontal leg
             // Arrowhead:
             << QPoint(mux_mdro.x() + 5, mux_mdro.bottom() + arrowHDepth - 5)    // Arrow Left Inner point
             << QPoint(mux_mdro.x() + 0, mux_mdro.bottom() + arrowHDepth - 5)    // Arrow Left Outer point
             << QPoint(mux_mdro.x() + 10, mux_mdro.bottom() + arrowHOffset)      // Arrow Middle point
             << QPoint(mux_mdro.x() + 20, mux_mdro.bottom() + arrowHDepth - 5)   // Arrow Right Outer point
             << QPoint(mux_mdro.x() + 15, mux_mdro.bottom() + arrowHDepth - 5)); // Arrow Right Inner point

const QPolygon bus_data_to_mdremux =
    QPolygon(QVector<QPoint>()
             // Foot:
             << QPoint(mux_mdre.x() + 15, mux_mdre.bottom() + (arrowHDepth - 5) +
                                              18) // Pivot between vertical right leg and lower horizontal leg
             << QPoint(80 + 2, mux_mdre.bottom() + (arrowHDepth - 5) + 18) // bus_data Foot Bottom point
             << QPoint(80 + 2, mux_mdre.bottom() + (arrowHDepth - 5) + 8)  // bus_data Foot Top point
             << QPoint(mux_mdre.x() + 5, mux_mdre.bottom() + (arrowHDepth - 5) +
                                             8) // Pivot between vertical left leg and upper horizontal leg
             // Arrowhead:
             << QPoint(mux_mdre.x() + 5, mux_mdre.bottom() + (arrowHDepth - 5))  // Arrow Left Inner point
             << QPoint(mux_mdre.x() + 0, mux_mdre.bottom() + (arrowHDepth - 5))  // Arrow Left Outer point
             << QPoint(mux_mdre.x() + 10, mux_mdre.bottom() + arrowHOffset)      // Arrow Middle point
             << QPoint(mux_mdre.x() + 20, mux_mdre.bottom() + arrowHDepth - 5)   // Arrow Right Outer point
             << QPoint(mux_mdre.x() + 15, mux_mdre.bottom() + arrowHDepth - 5)); // Arrow Right Inner point
const QPolygon bus_mdro_to_data = QPolygon(
    QVector<QPoint>()
    << QPoint(reg_byte_mdro.x(), reg_byte_mdro.y() + reg_byte_mdro.height() / 2 - 5) // Top Right Corner
    << QPoint(bus_data.x() + bus_data.width() + 13,
              reg_byte_mdro.y() + reg_byte_mdro.height() / 2 - 5) // Arrow Inner upper point
    << QPoint(bus_data.x() + bus_data.width() + 13,
              reg_byte_mdro.y() + reg_byte_mdro.height() / 2 - 10) // Arrow Outer Upper point
    << QPoint(bus_data.x() + bus_data.width() + 3, reg_byte_mdro.y() + reg_byte_mdro.height() / 2) // Arrow middle
    << QPoint(bus_data.x() + bus_data.width() + 13,
              reg_byte_mdro.y() + reg_byte_mdro.height() / 2 + 10) // Arrow Outer Lower Point
    << QPoint(bus_data.x() + bus_data.width() + 13,
              reg_byte_mdro.y() + reg_byte_mdro.height() / 2 + 5)                      // Arrow Inner Lower Point
    << QPoint(reg_byte_mdro.x(), reg_byte_mdro.y() + reg_byte_mdro.height() / 2 + 5)); // Bottom Right Corner

const QPolygon bus_mdre_to_data = QPolygon(
    QVector<QPoint>() << QPoint(reg_byte_mdre.x(), reg_byte_mdre.y() + reg_byte_mdre.height() / 2 - 5) // Foot Top point
                      << QPoint(bus_data.x() + bus_data.width() + 13,
                                reg_byte_mdre.y() + reg_byte_mdre.height() / 2 - 5) // Arrow Top Inner point
                      << QPoint(bus_data.x() + bus_data.width() + 13,
                                reg_byte_mdre.y() + reg_byte_mdre.height() / 2 - 10) // Arrow Top Outer point
                      << QPoint(bus_data.x() + bus_data.width() + 3,
                                reg_byte_mdre.y() + reg_byte_mdre.height() / 2) // Arrow middle point
                      << QPoint(bus_data.x() + bus_data.width() + 13,
                                reg_byte_mdre.y() + reg_byte_mdre.height() / 2 + 10) // Arrow Botton Outer point
                      << QPoint(bus_data.x() + bus_data.width() + 13,
                                reg_byte_mdre.y() + reg_byte_mdre.height() / 2 + 5) // Arrow Bottom Inner point
                      << QPoint(reg_byte_mdre.x(),
                                reg_byte_mdre.y() + reg_byte_mdre.height() / 2 + 5)); // Foot Bottom point
const QPolygon bus_mdre_to_eomux = QPolygon(
    QVector<QPoint>()
    << QPoint(reg_byte_mdre.right() + 1, reg_byte_mdre.y() + reg_byte_mdre.height() / 2 - 5) // MDRE Foot Top point
    << QPoint(reg_byte_mdre.right() + 1, reg_byte_mdre.y() + reg_byte_mdre.height() / 2 + 5) // MDRE Foot Bottom point
    << QPoint(mux_eo.x() + 5,
              reg_byte_mdre.y() + reg_byte_mdre.height() / 2 + 5) // Pivot between MDRE bottom and EOMux bottom
    << QPoint(mux_eo.x() + 5, mux_eo.y() - (arrowHDepth - 5))     // EOMux Arrow Left Inner point
    << QPoint(mux_eo.x() + 0, mux_eo.y() - (arrowHDepth - 5))     // EOMux Arrow Left Outer point
    << QPoint(mux_eo.x() + 10, mux_eo.y() - (arrowHOffset))       // EOMux Arrow Middle point
    << QPoint(mux_eo.x() + 20, mux_eo.y() - (arrowHDepth - 5))    // EOMux Arrow Right Outer point
    << QPoint(mux_eo.x() + 15, mux_eo.y() - (arrowHDepth - 5))    // EOMux Arrow Right Inner point
    << QPoint(mux_eo.x() + 15,
              poly_marmux.bottom() + (arrowHDepth - 5)) // MARMux Arrow Right Inner point
    << QPoint(mux_eo.x() + 20,
              poly_marmux.bottom() + (arrowHDepth - 5))                 // MARMux Arrow Right Outer point
    << QPoint(mux_eo.x() + 10, poly_marmux.bottom() + (arrowHOffset))   // MARMux Arrow Middle point
    << QPoint(mux_eo.x() + 0, poly_marmux.bottom() + (arrowHDepth - 5)) // MARMux Arrow Left Outer point
    << QPoint(mux_eo.x() + 5, poly_marmux.bottom() + (arrowHDepth - 5)) // MARMux Arrow Left Inner point
    << QPoint(mux_eo.x() + 5,
              reg_byte_mdre.y() + reg_byte_mdre.height() / 2 - 5) // Pivot between MARMux bottom and MDRE top
);
const QPolygon bus_mdro_to_eomux = QPolygon(
    QVector<QPoint>()
    << QPoint(reg_byte_mdro.right() + 1, reg_byte_mdro.y() + reg_byte_mdro.height() / 2 - 5) // MDRO Foot Top point
    << QPoint(reg_byte_mdro.right() + 1, reg_byte_mdro.y() + reg_byte_mdro.height() / 2 + 5) // MDRO Foot Bottom point
    << QPoint(mux_eo.right() - 15,
              reg_byte_mdro.y() + reg_byte_mdro.height() / 2 + 5)  // Pivot between MDRE bottom and EOMux bottom
    << QPoint(mux_eo.right() - 15, mux_eo.y() - (arrowHDepth - 5)) // EOMux Arrow Left Inner Edge
    << QPoint(mux_eo.right() - 20, mux_eo.y() - (arrowHDepth - 5)) // EOMux Arrow Left Outer Edge
    << QPoint(mux_eo.right() - 10, mux_eo.y() - (arrowHOffset))    // EOMux Arrow Middle point
    << QPoint(mux_eo.right() - 0, mux_eo.y() - (arrowHDepth - 5))  // EOMux Arrow Right Outer Edge
    << QPoint(mux_eo.right() - 5, mux_eo.y() - (arrowHDepth - 5))  // EOMux Arrow Right Inner Edge
    << QPoint(mux_eo.right() - 5,
              poly_marmux.bottom() + (arrowHDepth - 5)) // MARMux Arrow Right Inner Edge
    << QPoint(mux_eo.right() + 0,
              poly_marmux.bottom() + (arrowHDepth - 5))                   // MARMux Arrow Right Outer Edge
    << QPoint(mux_eo.right() - 10, poly_marmux.bottom() + (arrowHOffset)) // MARMux Arrow Middle point
    << QPoint(mux_eo.right() - 20,
              poly_marmux.bottom() + (arrowHDepth - 5)) // MARMux Arrow Left Outer Edge
    << QPoint(mux_eo.right() - 15,
              poly_marmux.bottom() + (arrowHDepth - 5)) // MARMux Arrow Left Inner Edge
    << QPoint(mux_eo.right() - 15,
              reg_byte_mdro.y() + reg_byte_mdro.height() / 2 - 5) // Pivot between MARMux bottom and MDRE top
);
// const QPolygon bus_mdrmux_to_mdr;
const QPolygon bus_mdromux_to_mdro = QPolygon(QVector<QPoint>() << QPoint(mux_mdro.x() + mux_mdro.width() / 2 - 5,
                                                                          mux_mdro.y()) // Foot Left point
                                                                << QPoint(mux_mdro.x() + mux_mdro.width() / 2 - 5,
                                                                          mux_mdro.y() - 7) // Arrow Left Inner point
                                                                << QPoint(mux_mdro.x() + mux_mdro.width() / 2 - 10,
                                                                          mux_mdro.y() - 7) // Arrow Left Outer point
                                                                << QPoint(mux_mdro.x() + mux_mdro.width() / 2,
                                                                          mux_mdro.y() - 17) // Arrow Middle point
                                                                << QPoint(mux_mdro.x() + mux_mdro.width() / 2 + 10,
                                                                          mux_mdro.y() - 7) // Arrow Right Outer point
                                                                << QPoint(mux_mdro.x() + mux_mdro.width() / 2 + 5,
                                                                          mux_mdro.y() - 7) // Arrow Right Inner point
                                                                << QPoint(mux_mdro.x() + mux_mdro.width() / 2 + 5,
                                                                          mux_mdro.y())); // Foot Right point

const QPolygon bus_mdremux_to_mdre = QPolygon(QVector<QPoint>() << QPoint(mux_mdre.x() + mux_mdre.width() / 2 - 5,
                                                                          mux_mdre.y()) // Foot Left point
                                                                << QPoint(mux_mdre.x() + mux_mdre.width() / 2 - 5,
                                                                          mux_mdre.y() - 7) // Arrow Left Inner point
                                                                << QPoint(mux_mdre.x() + mux_mdre.width() / 2 - 10,
                                                                          mux_mdre.y() - 7) // Arrow  Outer left point
                                                                << QPoint(mux_mdre.x() + mux_mdre.width() / 2,
                                                                          mux_mdre.y() - 17) // Arrow Middle point
                                                                << QPoint(mux_mdre.x() + mux_mdre.width() / 2 + 10,
                                                                          mux_mdre.y() - 7) // Arrow Right Outer point
                                                                << QPoint(mux_mdre.x() + mux_mdre.width() / 2 + 5,
                                                                          mux_mdre.y() - 7) // Arrow Right Inner point
                                                                << QPoint(mux_mdre.x() + mux_mdre.width() / 2 + 5,
                                                                          mux_mdre.y())); // Foot Right point

const QPolygon bus_alu_to_cmux = OneByteShapes::bus_alu_to_cmux.translated(controlOffsetX, aluOffsetY);

const Arrow sel_alu =
    Arrow(QVector<Arrowhead>() << Arrowhead{QPoint(poly_alu.boundingRect().right() - 13,
                                                   poly_alu.boundingRect().bottom() - 21)},
          QVector<QLine>() << QLine(poly_alu.boundingRect().right() - 13, ALULineEdit.y() + selectYOffset - 1,
                                    ctrlInputX - 7, ALULineEdit.y() + selectYOffset - 1)
                           << QLine(ctrlInputX - 17, ALULineEdit.y() + 13, ctrlInputX - 27,
                                    ALULineEdit.y() + 3)); // diagonal line

const Arrow logic_alu_nzvc = OneByteShapes::logic_alu_nzvc.translated(controlOffsetX, aluOffsetY);

const QLine sel_muxcs = QLine(mux_cs.right() + arrowHOffset, CSMuxLabel.y() + selectYOffset + 1,
                                ctrlInputX - 7, CSMuxLabel.y() + selectYOffset + 1);
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
const QLine poly_nzvc_join = QLine(OneByteShapes::poly_nzvc_join).translated(controlOffsetX, aluOffsetY);
const Arrow logic_c_to_nzvc = OneByteShapes::logic_c_to_nzvc.translated(controlOffsetX, aluOffsetY);
const Arrow logic_c_to_csmux = OneByteShapes::logic_c_to_csmux.translated(controlOffsetX, aluOffsetY);
const Arrow logic_cin = OneByteShapes::logic_cin.translated(controlOffsetX, aluOffsetY);
const Arrow logic_s_to_csmux = OneByteShapes::logic_s_to_csmux.translated(controlOffsetX, aluOffsetY);
const Arrow logic_z_to_nzvc = OneByteShapes::logic_z_to_nzvc.translated(controlOffsetX, aluOffsetY);
const Arrow logic_v_to_nzvc = OneByteShapes::logic_v_to_nzvc.translated(controlOffsetX, aluOffsetY);
const Arrow logic_n_to_nzvc = OneByteShapes::logic_n_to_nzvc.translated(controlOffsetX, aluOffsetY);

const Arrow logic_andz_to_z = Arrow(
    QVector<Arrowhead>() << Arrowhead{QPoint(mux_andz.x() + mux_andz.width() / 2 - arrowHOffset / 2,
                                             mux_andz.top() - selectYOffset - 2),
                                      DOWN}
                         << Arrowhead{QPoint(reg_bit_z.x() - 12, mux_andz.y() + mux_andz.height() / 2 - 3), RIGHT},
    QVector<QLine>()
        // Connects arrow head to horizontal line
        << QLine(mux_andz.x() + mux_andz.width() / 2, AndZTristateLabel.y() + AndZTristateLabel.height() / 2,
                 mux_andz.x() + mux_andz.width() / 2, mux_andz.y() - arrowHOffset)
        // Horizontal line from label to arrowhead.
        << QLine(mux_andz.x() + mux_andz.width() / 2, AndZTristateLabel.y() + AndZTristateLabel.height() / 2,
                 ctrlInputX - 7, AndZTristateLabel.y() + AndZTristateLabel.height() / 2)
        // Line from ANDZ circuit to Z bit.
        << QLine(mux_andz.right(), mux_andz.y() + mux_andz.height() / 2, reg_bit_z.left() - arrowHOffset,
                 mux_andz.y() + mux_andz.height() / 2));

const QLine ck_memread = QLine(bus_data.right() + arrowHOffset, MemReadLabel.y() + selectYOffset, ctrlInputX - 7,
                                  MemReadLabel.y() + selectYOffset);
const QLine ck_memwrite = QLine(bus_data.right() + arrowHOffset, MemWriteLabel.y() + selectYOffset, ctrlInputX - 7,
                                   MemWriteLabel.y() + selectYOffset); // Doesn't draw vertical lines

} // namespace TwoByteShapes
