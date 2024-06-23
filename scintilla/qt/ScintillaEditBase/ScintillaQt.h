#pragma once
//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// @file ScintillaQt.h - Qt specific subclass of ScintillaBase

#include <QAbstractScrollArea>
#include <QAction>
#include <QClipboard>
#include <QObject>
#include <QPaintEvent>
#include <QQuickPaintedItem>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>
#include "CaseFolder.h"
#include "Editor.h"
#include "Geometry.h"
#include "Platform.h"
#include "Position.h"
#include "ScintillaBase.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ScintillaTypes.h"
#include "scintilla_globals.h"

class ScintillaEditBase;

namespace Scintilla::Internal {

class SCINTILLA_EXPORT ScintillaQt : public QObject, public ScintillaBase {
  Q_OBJECT

public:
#ifdef PLAT_QT_QML
  explicit ScintillaQt(QQuickPaintedItem *parent);
  void UpdateInfos(int winId);
  QQuickPaintedItem *GetScrollArea() { return scrollArea; }
  void selectCurrentWord();
#else
  explicit ScintillaQt(QAbstractScrollArea *parent);
#endif
  virtual ~ScintillaQt();

signals:
  void cursorPositionChanged();
  void horizontalScrolled(int value);
  void verticalScrolled(int value);
  void horizontalRangeChanged(int max, int page);
  void verticalRangeChanged(int max, int page);

  void notifyParent(Scintilla::NotificationData scn);
  void notifyChange();

  // Clients can use this hook to add additional
  // formats (e.g. rich text) to the MIME data.
  void aboutToCopy(QMimeData *data);

  void command(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);

private slots:
  void onIdle();
  void execCommand(QAction *action);
  void SelectionChanged();

private:
  void Init();
  void Finalise() override;
  bool DragThreshold(Point ptStart, Point ptNow) override;
  bool ValidCodePage(int codePage) const override;
  std::string UTF8FromEncoded(std::string_view encoded) const override;
  std::string EncodedFromUTF8(std::string_view utf8) const override;

private:
  void ScrollText(Sci::Line linesToMove) override;
  void SetVerticalScrollPos() override;
  void SetHorizontalScrollPos() override;
  bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override;
  void ReconfigureScrollBars() override;
  void CopyToModeClipboard(const SelectionText &selectedText, QClipboard::Mode clipboardMode_);
  void Copy() override;
  void CopyToClipboard(const SelectionText &selectedText) override;
  void PasteFromMode(QClipboard::Mode clipboardMode_);
  void Paste() override;
  void ClaimSelection() override;
  void NotifyChange() override;
  void NotifyFocus(bool focus) override;
  void NotifyParent(Scintilla::NotificationData scn) override;
  void NotifyURIDropped(const char *uri);
  int timers[static_cast<size_t>(TickReason::dwell) + 1]{};
  bool FineTickerRunning(TickReason reason) override;
  void FineTickerStart(TickReason reason, int millis, int tolerance) override;
  void CancelTimers();
  void FineTickerCancel(TickReason reason) override;
  bool ChangeIdle(bool on);
  bool SetIdle(bool on) override;
  void SetMouseCapture(bool on) override;
  bool HaveMouseCapture() override;
  void StartDrag() override;
  Scintilla::CharacterSet CharacterSetOfDocument() const;
  const char *CharacterSetIDOfDocument() const;
  QString StringFromDocument(const char *s) const;
  QByteArray BytesForDocument(const QString &text) const;
  std::unique_ptr<CaseFolder> CaseFolderForEncoding() override;
  std::string CaseMapString(const std::string &s, CaseMapping caseMapping) override;

  void CreateCallTipWindow(PRectangle rc) override;
  void AddToPopUp(const char *label, int cmd, bool enabled) override;
  sptr_t WndProc(Scintilla::Message iMessage, uptr_t wParam, sptr_t lParam) override;
  sptr_t DefWndProc(Scintilla::Message iMessage, uptr_t wParam, sptr_t lParam) override;

  static sptr_t DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam);
  static sptr_t DirectStatusFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam, int *pStatus);
#ifdef PLAT_QT_QML
  QPainter *GetPainter() { return currentPainter; }
#endif
protected:
  void PartialPaint(const PRectangle &rect);
  void PartialPaintQml(const PRectangle &rect, QPainter *painter);

  void DragEnter(const Point &point);
  void DragMove(const Point &point);
  void DragLeave();
  void Drop(const Point &point, const QMimeData *data, bool move);
  void DropUrls(const QMimeData *data);

  void timerEvent(QTimerEvent *event) override;

private:
#ifdef PLAT_QT_QML
  QQuickPaintedItem *scrollArea; // is a ScintillaEditBase
#else
  QAbstractScrollArea *scrollArea;
#endif

  int vMax, hMax;   // Scroll bar maximums.
  int vPage, hPage; // Scroll bar page sizes.

  bool haveMouseCapture;
  bool dragWasDropped;
  int rectangularSelectionModifier;

#ifdef PLAT_QT_QML
  QPainter *currentPainter; // temporary variable for paint() handling
#endif

  friend class ::ScintillaEditBase;
};
} // namespace Scintilla::Internal
