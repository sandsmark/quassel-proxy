/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef CHATSCENE_H_
#define CHATSCENE_H_

#include <QAbstractItemModel>
#include <QClipboard>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QSet>
#include <QTimer>

#include "chatlinemodel.h"
#include "messagefilter.h"

class AbstractUiMsg;
class ChatItem;
class ChatLine;
class ChatView;
class ColumnHandleItem;
class WebPreviewItem;

class QGraphicsSceneMouseEvent;

class ChatScene : public QGraphicsScene {
  Q_OBJECT

public:
  enum CutoffMode {
    CutoffLeft,
    CutoffRight
  };

  enum ItemType {
    ChatLineType = QGraphicsItem::UserType + 1,
    ChatItemType,
    TimestampChatItemType,
    SenderChatItemType,
    ContentsChatItemType,
    SearchHighlightType,
    WebPreviewType,
    ColumnHandleType
  };

  enum ClickMode {
    NoClick,
    DragStartClick,
    SingleClick,
    DoubleClick,
    TripleClick
  };

  ChatScene(QAbstractItemModel *model, const QString &idString, qreal width, ChatView *parent);
  virtual ~ChatScene();

  inline QAbstractItemModel *model() const { return _model; }
  inline MessageFilter *filter() const { return qobject_cast<MessageFilter*>(_model); }
  inline QString idString() const { return _idString; }

  int rowByScenePos(qreal y) const;
  inline int rowByScenePos(const QPointF &pos) const { return rowByScenePos(pos.y()); }
  ChatLineModel::ColumnType columnByScenePos(qreal x) const ;
  inline ChatLineModel::ColumnType columnByScenePos(const QPointF &pos) const { return columnByScenePos(pos.x()); }

  ChatView *chatView() const;
  ChatItem *chatItemAt(const QPointF &pos) const;
  inline ChatLine *chatLine(int row) { return (row < _lines.count()) ? _lines[row] : 0; }

  inline bool isSingleBufferScene() const { return _singleBufferId.isValid(); }
  inline BufferId singleBufferId() const { return _singleBufferId; }
  bool containsBuffer(const BufferId &id) const;

  ColumnHandleItem *firstColumnHandle() const;
  ColumnHandleItem *secondColumnHandle() const;

  inline CutoffMode senderCutoffMode() const { return _cutoffMode; }
  inline void setSenderCutoffMode(CutoffMode mode) { _cutoffMode = mode; }

  QString selection() const;
  bool hasSelection() const;
  bool hasGlobalSelection() const;
  bool isPosOverSelection(const QPointF &) const;
  bool isGloballySelecting() const;
  void initiateDrag(QWidget *source);

  bool isScrollingAllowed() const;

  virtual bool event(QEvent *e);

 public slots:
  void updateForViewport(qreal width, qreal height);
  void setWidth(qreal width);
  void layout(int start, int end, qreal width);

  // these are used by the chatitems to notify the scene and manage selections
  void setSelectingItem(ChatItem *item);
  ChatItem *selectingItem() const { return _selectingItem; }
  void startGlobalSelection(ChatItem *item, const QPointF &itemPos);
  void clearGlobalSelection();
  void clearSelection();
  void selectionToClipboard(QClipboard::Mode = QClipboard::Clipboard);
  void stringToClipboard(const QString &str, QClipboard::Mode = QClipboard::Clipboard);

  void requestBacklog();

#ifdef HAVE_WEBKIT
  void loadWebPreview(ChatItem *parentItem, const QString &url, const QRectF &urlRect);
  void clearWebPreview(ChatItem *parentItem = 0);
#endif

signals:
  void lastLineChanged(QGraphicsItem *item, qreal offset);
  void layoutChanged(); // indicates changes to the scenerect due to resizing of the contentsitems
  void mouseMoveWhileSelecting(const QPointF &scenePos);

protected:
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void handleClick(Qt::MouseButton button, const QPointF &scenePos);

protected slots:
  void rowsInserted(const QModelIndex &, int, int);
  void rowsAboutToBeRemoved(const QModelIndex &, int, int);
  void dataChanged(const QModelIndex &, const QModelIndex &);

private slots:
  void firstHandlePositionChanged(qreal xpos);
  void secondHandlePositionChanged(qreal xpos);
#ifdef HAVE_WEBKIT
  void webPreviewNextStep();
#endif
  void showWebPreviewChanged();

  void clickTimeout();

private:
  void setHandleXLimits();
  void updateSelection(const QPointF &pos);

  ChatView *_chatView;
  QString _idString;
  QAbstractItemModel *_model;
  QList<ChatLine *> _lines;
  BufferId _singleBufferId;

  // calls to QChatScene::sceneRect() are very expensive. As we manage the scenerect ourselves
  // we store the size in a member variable.
  QRectF _sceneRect;
  int _firstLineRow; // the first row to display (aka: not a daychange msg)
  void updateSceneRect(qreal width);
  inline void updateSceneRect() { updateSceneRect(_sceneRect.width()); }
  void updateSceneRect(const QRectF &rect);
  qreal _viewportHeight;

  ColumnHandleItem *_firstColHandle, *_secondColHandle;
  qreal _firstColHandlePos, _secondColHandlePos;
  CutoffMode _cutoffMode;

  ChatItem *_selectingItem;
  int _selectionStartCol, _selectionMinCol;
  int _selectionStart;
  int _selectionEnd;
  int _firstSelectionRow;
  bool _isSelecting;

  QTimer _clickTimer;
  ClickMode _clickMode;
  QPointF _clickPos;
  bool _clickHandled;
  bool _leftButtonPressed;

  bool _showWebPreview;

#ifdef HAVE_WEBKIT
  struct WebPreview {
    enum PreviewState {
      NoPreview,
      NewPreview,
      DelayPreview,
      ShowPreview,
      HidePreview
    };
    ChatItem *parentItem;
    QGraphicsItem *previewItem;
    QString url;
    QRectF urlRect;
    PreviewState previewState;
    QTimer timer;
    WebPreview() : parentItem(0), previewItem(0), previewState(NoPreview) {}
  };
  WebPreview webPreview;
#endif // HAVE_WEBKIT
};

#endif
