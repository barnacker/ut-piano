#include "selectionwindow.h"
#include "musescore.h"
#include "libmscore/score.h"
#include "libmscore/select.h"
#include "palettebox.h"
#include "scoreaccessibility.h"

namespace Ms {

static const char* labels[] = {
      QT_TRANSLATE_NOOP("selectionfilter", "All"),
      QT_TRANSLATE_NOOP("selectionfilter", "Voice 1"),
      QT_TRANSLATE_NOOP("selectionfilter", "Voice 2"),
      QT_TRANSLATE_NOOP("selectionfilter", "Voice 3"),
      QT_TRANSLATE_NOOP("selectionfilter", "Voice 4"),
      QT_TRANSLATE_NOOP("selectionfilter", "Dynamics"),
      QT_TRANSLATE_NOOP("selectionfilter", "Fingering"),
      QT_TRANSLATE_NOOP("selectionfilter", "Lyrics"),
      QT_TRANSLATE_NOOP("selectionfilter", "Chord Symbols"),
      QT_TRANSLATE_NOOP("selectionfilter", "Other Text"),
      QT_TRANSLATE_NOOP("selectionfilter", "Articulations & Ornaments"),
      QT_TRANSLATE_NOOP("selectionfilter", "Slurs"),
      QT_TRANSLATE_NOOP("selectionfilter", "Figured Bass"),
      QT_TRANSLATE_NOOP("selectionfilter", "Ottava"),
      QT_TRANSLATE_NOOP("selectionfilter", "Pedal Lines"),
      QT_TRANSLATE_NOOP("selectionfilter", "Other Lines"),
      QT_TRANSLATE_NOOP("selectionfilter", "Arpeggios"),
      QT_TRANSLATE_NOOP("selectionfilter", "Glissandos"),
      QT_TRANSLATE_NOOP("selectionfilter", "Fretboard Diagrams"),
      QT_TRANSLATE_NOOP("selectionfilter", "Breath Marks"),
      QT_TRANSLATE_NOOP("selectionfilter", "Tremolo"),
      QT_TRANSLATE_NOOP("selectionfilter", "Grace Notes")
      };

const int numLabels = sizeof(labels)/sizeof(labels[0]);

SelectionListWidget::SelectionListWidget(QWidget *parent) : QListWidget(parent)
      {
      setAccessibleName(tr("Selection filter"));
      setAccessibleDescription(tr("Use Tab and Backtab (Shift+Tab) to move through the check boxes"));
      setFrameShape(QFrame::NoFrame);
      setSelectionMode(QAbstractItemView::SingleSelection);
      setFocusPolicy(Qt::TabFocus);
      setTabKeyNavigation(true);

      for (int row = 0; row < numLabels; row++) {
            QListWidgetItem *listItem = new QListWidgetItem(qApp->translate("selectionfilter", labels[row]),this);
            listItem->setData(Qt::UserRole, row == 0 ? QVariant(-1) : QVariant(1 << (row - 1)));
            listItem->setData(Qt::AccessibleTextRole, qApp->translate("selectionfilter", labels[row]));
            listItem->setCheckState(Qt::Unchecked);
            addItem(listItem);
            }
      }
void SelectionListWidget::focusInEvent(QFocusEvent* e) {
      setCurrentRow(0);
      QListWidget::focusInEvent(e);
      }

SelectionWindow::SelectionWindow(QWidget *parent, Score* score) :
      QDockWidget(tr("Selection Filter"),parent)
      {
      setObjectName("SelectionWindow");
      setAllowedAreas(Qt::DockWidgetAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea));
      _score = score;

      _listWidget = new SelectionListWidget;
      setWidget(_listWidget);

      //?MuseScore::restoreGeometry(this);

      updateFilteredElements();
      connect(_listWidget, SIGNAL(itemChanged(QListWidgetItem*)), SLOT(changeCheckbox(QListWidgetItem*)));
      }

SelectionWindow::~SelectionWindow()
      {
      //if (isVisible()) {
      //      MuseScore::saveGeometry(this);
      //      }
      }

//---------------------------------------------------------
//   updateFilteredElements
//---------------------------------------------------------

void SelectionWindow::updateFilteredElements()
      {
      if (!_score)
            return;
      int filter = _score->selectionFilter().filtered();
      bool all = true;
      bool none = true;
      _listWidget->blockSignals(true);
      for(int row = 1; row < _listWidget->count(); row++) {
            QListWidgetItem *item = _listWidget->item(row);
            if (filter & 1 << (row - 1)) {
                  if (item->checkState() != Qt::Checked)
                        item->setCheckState(Qt::Checked);
                  none = false;
                  }
            else {
                  if (item->checkState() != Qt::Unchecked)
                        item->setCheckState(Qt::Unchecked);
                  all = false;
                  }
            }
      QListWidgetItem *item = _listWidget->item(0);
      Qt::CheckState state = all ? Qt::Checked : (none ? Qt::Unchecked : Qt::PartiallyChecked);
      if (item->checkState() != state)
            item->setCheckState(state);
      _listWidget->blockSignals(false);
      }

//---------------------------------------------------------
//   changeCheckbox
//---------------------------------------------------------

void SelectionWindow::changeCheckbox(QListWidgetItem* item)
      {
      int type = item->data(Qt::UserRole).toInt();
      bool set = false;
      item->checkState() == Qt::Checked ? set = true : set = false;
      if (type > 0) {
            _score->selectionFilter().setFiltered(static_cast<SelectionFilterType>(type), set);
            }
      else {
            for (int row = 1; row < numLabels; row++)
                  _score->selectionFilter().setFiltered(static_cast<SelectionFilterType>(1 << (row - 1)), set);
            }
      if (_score->selection().isRange())
            _score->selection().updateSelectedElements();
      updateFilteredElements();
      _score->setUpdateAll();
      _score->end();
      ScoreAccessibility::instance()->updateAccessibilityInfo();
      }

//---------------------------------------------------------
//   showSelectionWindow
//---------------------------------------------------------

void MuseScore::showSelectionWindow(bool val)
      {
      QAction* a = getAction("toggle-selection-window");
      if (selectionWindow == 0) {
            selectionWindow = new SelectionWindow(this,this->currentScore());
            connect(selectionWindow, SIGNAL(closed(bool)), a, SLOT(setChecked(bool)));
            addDockWidget(Qt::LeftDockWidgetArea,selectionWindow);
            if (paletteBox && paletteBox->isVisible()) {
                  tabifyDockWidget(paletteBox, selectionWindow);
                  }
            }
      selectionWindow->setVisible(val);
      if (val) {
            selectionWindow->raise();
            }
      }
void SelectionWindow::closeEvent(QCloseEvent* ev)
      {
      emit closed(false);
      QWidget::closeEvent(ev);
      }

void SelectionWindow::hideEvent(QHideEvent* ev)
      {
      //MuseScore::saveGeometry(this);
      QWidget::hideEvent(ev);
      }

void SelectionWindow::setScore(Score* score)
      {
      _score = score;
      updateFilteredElements();
      }

QSize SelectionWindow::sizeHint() const
      {
      return QSize(170 * guiScaling, 170 * guiScaling);
      }

}
