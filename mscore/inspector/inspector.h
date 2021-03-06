//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#ifndef __INSPECTOR_H__
#define __INSPECTOR_H__

#include "inspectorBase.h"
#include "ui_inspector_element.h"
#include "ui_inspector_bend.h"
#include "ui_inspector_break.h"
#include "ui_inspector_vbox.h"
#include "ui_inspector_tbox.h"
#include "ui_inspector_hbox.h"
#include "ui_inspector_articulation.h"
#include "ui_inspector_spacer.h"
#include "ui_inspector_segment.h"
#include "ui_inspector_note.h"
#include "ui_inspector_chord.h"
#include "ui_inspector_rest.h"
#include "ui_inspector_clef.h"
#include "ui_inspector_timesig.h"
#include "ui_inspector_keysig.h"
#include "ui_inspector_volta.h"
#include "ui_inspector_barline.h"
#include "ui_inspector_tuplet.h"
#include "ui_inspector_accidental.h"
#include "ui_inspector_tempotext.h"
#include "ui_inspector_dynamic.h"
#include "ui_inspector_slur.h"
#include "ui_inspector_empty.h"
#include "ui_inspector_text.h"
#include "ui_inspector_fret.h"
#include "ui_inspector_tremolo.h"
#include "ui_inspector_caesura.h"

namespace Ms {

class Element;
class Note;
class Inspector;
class Segment;
class Chord;

//---------------------------------------------------------
//   InspectorElement
//---------------------------------------------------------

class UiInspectorElement: public Ui::InspectorElement {
   public:
      void setupUi(QWidget *InspectorElement);
      };

class InspectorElement : public InspectorBase {
      Q_OBJECT
      UiInspectorElement b;

   public:
      InspectorElement(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorBreak
//---------------------------------------------------------

class InspectorBreak : public InspectorBase {
      Q_OBJECT
      Ui::InspectorBreak b;

   public:
      InspectorBreak(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorVBox
//---------------------------------------------------------

class InspectorVBox : public InspectorBase {
      Q_OBJECT
      Ui::InspectorVBox vb;

   public:
      InspectorVBox(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorTBox
//---------------------------------------------------------

class InspectorTBox : public InspectorBase {
      Q_OBJECT
      Ui::InspectorTBox tb;

   public:
      InspectorTBox(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorHBox
//---------------------------------------------------------

class InspectorHBox : public InspectorBase {
      Q_OBJECT
      Ui::InspectorHBox hb;

   public:
      InspectorHBox(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorArticulation
//---------------------------------------------------------

class InspectorArticulation : public InspectorBase {
      Q_OBJECT
      UiInspectorElement e;
      Ui::InspectorArticulation ar;

   public:
      InspectorArticulation(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorSpacer
//---------------------------------------------------------

class InspectorSpacer : public InspectorBase {
      Q_OBJECT
      Ui::InspectorSpacer sp;

   public:
      InspectorSpacer(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorRest
//---------------------------------------------------------

class InspectorRest : public InspectorBase {
      Q_OBJECT

      UiInspectorElement   e;
      Ui::InspectorSegment s;
      Ui::InspectorRest    r;

      QToolButton* tuplet;

   private slots:
      void tupletClicked();

   public:
      InspectorRest(QWidget* parent);
      virtual void setElement() override;
      };

//---------------------------------------------------------
//   InspectorClef
//---------------------------------------------------------

class Clef;

class InspectorClef : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorSegment s;
      Ui::InspectorClef    c;
      Clef* otherClef;        // the courtesy clef for a main clef or viceversa
                              // used to keep in sync ShowCourtesy setting of both clefs
   protected slots:
      virtual void valueChanged(int idx) override;

   public:
      InspectorClef(QWidget* parent);
      virtual void setElement() override;
      };

//---------------------------------------------------------
//   InspectorTimeSig
//---------------------------------------------------------

class InspectorTimeSig : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorSegment s;
      Ui::InspectorTimeSig t;

   public:
      InspectorTimeSig(QWidget* parent);
      virtual void setElement() override;
      };

//---------------------------------------------------------
//   InspectorKeySig
//---------------------------------------------------------

class InspectorKeySig : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorSegment s;
      Ui::InspectorKeySig k;

   public:
      InspectorKeySig(QWidget* parent);
      virtual void setElement() override;
      };

//---------------------------------------------------------
//   InspectorTuplet
//---------------------------------------------------------

class InspectorTuplet : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorTuplet t;

   public:
      InspectorTuplet(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorAccidental
//---------------------------------------------------------

class InspectorAccidental : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorAccidental a;

   public:
      InspectorAccidental(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorBend
//---------------------------------------------------------

class InspectorBend : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorBend g;

   private slots:
      void propertiesClicked();

   public:
      InspectorBend(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorTremoloBar
//---------------------------------------------------------

class InspectorTremoloBar : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorTremoloBar g;

   private slots:
      void propertiesClicked();

   public:
      InspectorTremoloBar(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorTempoText
//---------------------------------------------------------

class InspectorTempoText : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorText t;
      Ui::InspectorTempoText tt;

   public:
      InspectorTempoText(QWidget* parent);
      virtual void setElement() override;
      virtual void postInit() override;
      };

//---------------------------------------------------------
//   InspectorDynamic
//---------------------------------------------------------

class InspectorDynamic : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorText t;
      Ui::InspectorDynamic d;

   public:
      InspectorDynamic(QWidget* parent);
      virtual void setElement() override;
      };

//---------------------------------------------------------
//   InspectorBarLine
//---------------------------------------------------------

#define BARLINE_BUILTIN_SPANS 5

class InspectorBarLine : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorBarLine b;

      static QString builtinSpanNames[BARLINE_BUILTIN_SPANS];

      void  blockSpanDataSignals(bool val);

   private slots:
      void spanTypeChanged(int idx);
      void resetSpanType();
      void manageSpanData();

   public:
      InspectorBarLine(QWidget* parent);
      virtual void setElement() override;
      };

//---------------------------------------------------------
//   Inspector
//---------------------------------------------------------

class Inspector : public QDockWidget {
      Q_OBJECT

      QScrollArea* sa;
      InspectorBase* ie;
      QList<Element*> _el;
      Element* _element;      // currently displayed element
      bool _inspectorEdit;    // set to true when an edit originates from
                              // within the inspector itself

   public slots:
      void reset();

   public:
      Inspector(QWidget* parent = 0);
      void setElement(Element*);
      void setElements(const QList<Element*>&);
      Element* element() const            { return _element;       }
      const QList<Element*>& el() const   { return _el;            }
      void setInspectorEdit(bool val)     { _inspectorEdit = val;  }
      };

//---------------------------------------------------------
//   InspectorSlur
//---------------------------------------------------------

class InspectorSlur : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorSlur s;

   public:
      InspectorSlur(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorCaesura
//---------------------------------------------------------

class InspectorCaesura : public InspectorBase {
      Q_OBJECT

      UiInspectorElement e;
      Ui::InspectorCaesura c;

   public:
      InspectorCaesura(QWidget* parent);
      };

//---------------------------------------------------------
//   InspectorEmpty
//---------------------------------------------------------

class InspectorEmpty : public InspectorBase {
      Q_OBJECT

      Ui::InspectorEmpty e;

   public:
      InspectorEmpty(QWidget* parent);
      virtual QSize sizeHint() const override;
      };

} // namespace Ms
#endif

