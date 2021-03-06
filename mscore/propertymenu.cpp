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

#include "scoreview.h"
#include "musescore.h"
#include "libmscore/undo.h"

#include "libmscore/score.h"
#include "libmscore/element.h"
#include "seq.h"
#include "libmscore/mscore.h"

#include "articulationprop.h"
#include "bendproperties.h"
#include "voltaproperties.h"
#include "lineproperties.h"
#include "tremolobarprop.h"
#include "timesigproperties.h"
#include "textstyle.h"
#include "textproperties.h"
#include "sectionbreakprop.h"
#include "stafftextproperties.h"
#include "glissandoproperties.h"
#include "fretproperties.h"
#include "selinstrument.h"
#include "pianoroll.h"
#include "editstyle.h"
#include "editstaff.h"
#include "measureproperties.h"

#include "libmscore/staff.h"
#include "libmscore/segment.h"
#include "libmscore/bend.h"
#include "libmscore/box.h"
#include "libmscore/text.h"
#include "libmscore/articulation.h"
#include "libmscore/volta.h"
#include "libmscore/tremolobar.h"
#include "libmscore/timesig.h"
#include "libmscore/accidental.h"
#include "libmscore/clef.h"
#include "libmscore/dynamic.h"
#include "libmscore/tempotext.h"
#include "libmscore/keysig.h"
#include "libmscore/stafftext.h"
#include "libmscore/staffstate.h"
#include "libmscore/note.h"
#include "libmscore/layoutbreak.h"
#include "libmscore/image.h"
#include "libmscore/hairpin.h"
#include "libmscore/chord.h"
#include "libmscore/rest.h"
#include "libmscore/harmony.h"
#include "libmscore/glissando.h"
#include "libmscore/fret.h"
#include "libmscore/instrchange.h"
#include "libmscore/slur.h"
#include "libmscore/jump.h"
#include "libmscore/marker.h"
#include "libmscore/measure.h"

namespace Ms {

//---------------------------------------------------------
//   genPropertyMenu1
//---------------------------------------------------------

void ScoreView::genPropertyMenu1(Element* e, QMenu* popup)
      {
      if ((!e->generated() || e->type() == Element::Type::BAR_LINE) && enableExperimental){
            if (e->flag(ElementFlag::HAS_TAG)) {
                  popup->addSeparator();

                  QMenu* menuLayer = new QMenu(tr("Layer"));
                  for (int i = 0; i < MAX_TAGS; ++i) {
                        QString tagName = score()->layerTags()[i];
                        if (!tagName.isEmpty()) {
                              QAction* a = menuLayer->addAction(tagName);
                              a->setData(QString("layer-%1").arg(i));
                              a->setCheckable(true);
                              a->setChecked(e->tag() & (1 << i));
                              }
                        }
                  popup->addMenu(menuLayer);
                  }
            }
      }

//---------------------------------------------------------
//   genPropertyMenuText
//---------------------------------------------------------

void ScoreView::genPropertyMenuText(Element* e, QMenu* popup)
      {
      if (e->flag(ElementFlag::HAS_TAG) && enableExperimental) {
            popup->addSeparator();

            QMenu* menuLayer = new QMenu(tr("Layer"));
            for (int i = 0; i < MAX_TAGS; ++i) {
                  QString tagName = score()->layerTags()[i];
                  if (!tagName.isEmpty()) {
                        QAction* a = menuLayer->addAction(tagName);
                        a->setData(QString("layer-%1").arg(i));
                        a->setCheckable(true);
                        a->setChecked(e->tag() & (1 << i));
                        }
                  }
            popup->addMenu(menuLayer);
            }
      popup->addAction(tr("Text Style..."))->setData("text-style");
      popup->addAction(tr("Text Properties..."))->setData("text-props");
      }

//---------------------------------------------------------
//   createElementPropertyMenu
//---------------------------------------------------------

void ScoreView::createElementPropertyMenu(Element* e, QMenu* popup)
      {
      if (e->type() == Element::Type::BAR_LINE) {
            genPropertyMenu1(e, popup);
            }
      else if (e->type() == Element::Type::ARTICULATION) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Articulation Properties..."))->setData("a-props");
            }
      else if (e->type() == Element::Type::BEAM) {
            popup->addAction(getAction("flip"));
            }
      else if (e->type() == Element::Type::STEM) {
            popup->addAction(getAction("flip"));
            }
      else if (e->type() == Element::Type::HOOK) {
            popup->addAction(getAction("flip"));
            }
      else if (e->type() == Element::Type::BEND) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Bend Properties..."))->setData("b-props");
            }
      else if (e->type() == Element::Type::TREMOLOBAR) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Tremolo Bar Properties..."))->setData("tr-props");
            }
      else if (e->type() == Element::Type::HBOX) {
            QMenu* textMenu = popup->addMenu(tr("Add"));
            // borrow translation info from global actions
            // but create new actions with local handler
            textMenu->addAction(getAction("frame-text")->text())->setData("frame-text");
            textMenu->addAction(getAction("picture")->text())->setData("picture");
            }
      else if (e->type() == Element::Type::VBOX) {
            QMenu* textMenu = popup->addMenu(tr("Add"));
            // borrow translation info from global actions
            // but create new actions with local handler
            textMenu->addAction(getAction("frame-text")->text())->setData("frame-text");
            textMenu->addAction(getAction("title-text")->text())->setData("title-text");
            textMenu->addAction(getAction("subtitle-text")->text())->setData("subtitle-text");
            textMenu->addAction(getAction("composer-text")->text())->setData("composer-text");
            textMenu->addAction(getAction("poet-text")->text())->setData("poet-text");
            textMenu->addAction(getAction("part-text")->text())->setData("part-text");
            textMenu->addAction(getAction("insert-hbox")->text())->setData("insert-hbox");
            textMenu->addAction(getAction("picture")->text())->setData("picture");
            }
      else if (e->type() == Element::Type::VOLTA_SEGMENT) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Line Properties..."))->setData("l-props");
            popup->addAction(tr("Volta Properties..."))->setData("v-props");
            }
      else if (e->type() == Element::Type::TIMESIG) {
            genPropertyMenu1(e, popup);
            TimeSig* ts = static_cast<TimeSig*>(e);
            int _track = ts->track();
            // if the time sig. is not generated (= not courtesy) and is in track 0
            // add the specific menu item
            QAction* a;
            if (!ts->generated() && !_track && ts->measure() != score()->firstMeasure()) {
                  a = popup->addAction(ts->showCourtesySig()
                     ? tr("Hide Courtesy Time Signature")
                     : tr("Show Courtesy Time Signature") );
                  a->setData("ts-courtesy");
                  }
            if (!ts->generated()) {
                  popup->addSeparator();
                  popup->addAction(tr("Time Signature Properties..."))->setData("ts-props");
                  }
            }
      else if (e->type() == Element::Type::CLEF) {
            genPropertyMenu1(e, popup);
            Clef* clef = static_cast<Clef*>(e);
            // if the clef is not generated (= not courtesy) add the specific menu item
            if (!e->generated() && clef->measure() != score()->firstMeasure()) {
                  QAction* a = popup->addAction(static_cast<Clef*>(e)->showCourtesy()
                     ? tr("Hide Courtesy Clef")
                     : tr("Show Courtesy Clef") );
                        a->setData("clef-courtesy");
                  }
            }
      else if (e->type() == Element::Type::DYNAMIC) {
            popup->addAction(tr("Text Style..."))->setData("text-style");
            popup->addAction(tr("Text Properties..."))->setData("text-props");
            }
      else if (e->type() == Element::Type::TEXTLINE_SEGMENT
                  || e->type() == Element::Type::OTTAVA_SEGMENT
                  || e->type() == Element::Type::PEDAL_SEGMENT
                  || (e->type() == Element::Type::HAIRPIN_SEGMENT
                      && static_cast<HairpinSegment*>(e)->hairpin()->useTextLine())) {
            popup->addAction(tr("Line Properties..."))->setData("l-props");
            }
      else if (e->type() == Element::Type::STAFF_TEXT) {
            genPropertyMenuText(e, popup);
            Text* t = static_cast<Text*>(e);
            if (t->systemFlag())
                  popup->addAction(tr("System Text Properties..."))->setData("st-props");
            else
                  popup->addAction(tr("Staff Text Properties..."))->setData("st-props");
            }
      else if (e->type() == Element::Type::TEXT
               || e->type() == Element::Type::REHEARSAL_MARK
               || e->type() == Element::Type::MARKER
               || e->type() == Element::Type::JUMP
               || e->type() == Element::Type::FINGERING
               || e->type() == Element::Type::LYRICS
               || e->type() == Element::Type::FIGURED_BASS) {
            genPropertyMenuText(e, popup);
            }
      else if (e->type() == Element::Type::HARMONY) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Text Style..."))->setData("text-style");
            }
      else if (e->type() == Element::Type::TEMPO_TEXT) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Text Style..."))->setData("text-style");
            popup->addAction(tr("Text Properties..."))->setData("text-props");
            }
      else if (e->type() == Element::Type::KEYSIG) {
            genPropertyMenu1(e, popup);
            KeySig* ks = static_cast<KeySig*>(e);
            if (!e->generated() && ks->measure() != score()->firstMeasure()) {
                  QAction* a = popup->addAction(ks->showCourtesy()
                     ? tr("Hide Courtesy Key Signature")
                     : tr("Show Courtesy Key Signature") );
                  a->setData("key-courtesy");
                  }
            }
      else if (e->type() == Element::Type::STAFF_STATE && static_cast<StaffState*>(e)->staffStateType() == StaffStateType::INSTRUMENT) {
            popup->addAction(tr("Text Style..."))->setData("text-style");
            popup->addAction(tr("Text Properties..."))->setData("text-props");
            popup->addAction(tr("Change Instrument Properties..."))->setData("ss-props");
            }
      else if (e->type() == Element::Type::SLUR_SEGMENT) {
            genPropertyMenu1(e, popup);
            //popup->addAction(tr("Edit Mode"))->setData("edit");
            }
      else if (e->type() == Element::Type::REST) {
            QAction* b = popup->actions()[0];
            QAction* a = popup->insertSeparator(b);
            a->setText(tr("Staff"));
            a = new QAction(tr("Staff Properties..."), 0);
            a->setData("staff-props");
            popup->insertAction(b, a);

            a = popup->insertSeparator(b);
            a->setText(tr("Measure"));
            a = new QAction(tr("Measure Properties..."), 0);
            a->setData("measure-props");
            // disable property changes for multi measure rests
            a->setEnabled(!static_cast<Rest*>(e)->segment()->measure()->isMMRest());

            popup->insertAction(b, a);

            genPropertyMenu1(e, popup);
            }
      else if (e->type() == Element::Type::NOTE) {
            QAction* b = popup->actions()[0];
            QAction* a = popup->insertSeparator(b);
            a->setText(tr("Staff"));
            a = new QAction(tr("Staff Properties..."), 0);
            a->setData("staff-props");
            popup->insertAction(b, a);

            a = popup->insertSeparator(b);
            a->setText(tr("Measure"));
            a = new QAction(tr("Measure Properties..."), 0);
            a->setData("measure-props");
            // disable property changes for multi measure rests
            a->setEnabled(!static_cast<Note*>(e)->chord()->segment()->measure()->isMMRest());

            popup->insertAction(b, a);

            genPropertyMenu1(e, popup);
            popup->addSeparator();

            popup->addAction(tr("Style..."))->setData("style");
            if (enableExperimental)
                  popup->addAction(tr("Chord Articulation..."))->setData("articulation");
            }
      else if (e->type() == Element::Type::LAYOUT_BREAK && static_cast<LayoutBreak*>(e)->layoutBreakType() == LayoutBreak::Type::SECTION) {
            popup->addAction(tr("Section Break Properties..."))->setData("break-props");
            }
      else if (e->type() == Element::Type::INSTRUMENT_CHANGE) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Text Style..."))->setData("text-style");
            popup->addAction(tr("Text Properties..."))->setData("text-props");
            popup->addAction(tr("Change Instrument..."))->setData("ch-instr");
            }
      else if (e->type() == Element::Type::FRET_DIAGRAM) {
            popup->addAction(tr("Fretboard Diagram Properties..."))->setData("fret-props");
            }
      else if (e->type() == Element::Type::GLISSANDO) {
            genPropertyMenu1(e, popup);
            popup->addAction(tr("Glissando Properties..."))->setData("gliss-props");
            }
      else if (e->type() == Element::Type::INSTRUMENT_NAME) {
            popup->addAction(tr("Text Style..."))->setData("text-style");
            popup->addAction(tr("Staff Properties..."))->setData("staff-props");
            }
      else
            genPropertyMenu1(e, popup);
      }

//---------------------------------------------------------
//   elementPropertyAction
//---------------------------------------------------------

void ScoreView::elementPropertyAction(const QString& cmd, Element* e)
      {
      if (cmd == "a-props") {
            ArticulationProperties rp(static_cast<Articulation*>(e));
            rp.exec();
            }
      else if (cmd == "b-props")
            editBendProperties(static_cast<Bend*>(e));
      else if (cmd == "measure-props") {
            Measure* m = 0;
            if (e->type() == Element::Type::NOTE)
                  m = static_cast<Note*>(e)->chord()->segment()->measure();
            else if (e->type() == Element::Type::REST)
                  m = static_cast<Rest*>(e)->segment()->measure();
            if (m) {
                  MeasureProperties vp(m);
                  vp.exec();
                  }
            }
      else if (cmd == "picture") {
            mscore->addImage(score(), static_cast<HBox*>(e));
            }
      else if (cmd == "frame-text") {
            Text* t = new Text(score());
            t->setTextStyleType(TextStyleType::FRAME);
            t->setParent(e);
            score()->undoAddElement(t);
            score()->select(t, SelectType::SINGLE, 0);
            startEdit(t);
            }
      else if (cmd == "title-text") {
            Text* t = new Text(score());
            t->setTextStyleType(TextStyleType::TITLE);
            t->setParent(e);
            score()->undoAddElement(t);
            score()->select(t, SelectType::SINGLE, 0);
            startEdit(t);
            }
      else if (cmd == "subtitle-text") {
            Text* t = new Text(score());
            t->setTextStyleType(TextStyleType::SUBTITLE);
            t->setParent(e);
            score()->undoAddElement(t);
            score()->select(t, SelectType::SINGLE, 0);
            startEdit(t);
            }
      else if (cmd == "composer-text") {
            Text* t = new Text(score());
            t->setTextStyleType(TextStyleType::COMPOSER);
            t->setParent(e);
            score()->undoAddElement(t);
            score()->select(t, SelectType::SINGLE, 0);
            startEdit(t);
            }
      else if (cmd == "poet-text") {
            Text* t = new Text(score());
            t->setTextStyleType(TextStyleType::POET);
            t->setParent(e);
            score()->undoAddElement(t);
            score()->select(t, SelectType::SINGLE, 0);
            startEdit(t);
            }
      else if (cmd == "part-text") {
            Text* t = new Text(score());
            t->setTextStyleType(TextStyleType::INSTRUMENT_EXCERPT);
            t->setParent(e);
            score()->undoAddElement(t);
            score()->select(t, SelectType::SINGLE, 0);
            startEdit(t);
            }
      else if (cmd == "insert-hbox") {
            HBox* s = new HBox(score());
            double w = e->width() - s->leftMargin() * DPMM - s->rightMargin() * DPMM;
            s->setBoxWidth(Spatium(w / s->spatium()));
            s->setParent(e);
            score()->undoAddElement(s);
            score()->select(s, SelectType::SINGLE, 0);
            startEdit(s);
            }
      else if (cmd == "v-props") {
            VoltaSegment* vs = static_cast<VoltaSegment*>(e);
            VoltaProperties vp;
            vp.setText(Text::unEscape(vs->volta()->text()));
            vp.setEndings(vs->volta()->endings());
            int rv = vp.exec();
            if (rv) {
                  QString txt  = vp.getText();
                  QList<int> l = vp.getEndings();
                  if (txt != vs->volta()->text())
                        vs->volta()->undoChangeProperty(P_ID::BEGIN_TEXT, Text::tagEscape(txt));
                  if (l != vs->volta()->endings())
                        vs->volta()->undoChangeProperty(P_ID::VOLTA_ENDING, QVariant::fromValue(l));
                  }
            }
      else if (cmd == "l-props") {
            TextLineSegment* vs = static_cast<TextLineSegment*>(e);
            LineProperties lp(vs->textLine());
            lp.exec();
            }
      else if (cmd == "tr-props")
            editTremoloBarProperties(static_cast<TremoloBar*>(e));
      if (cmd == "ts-courtesy") {
            TimeSig* ts = static_cast<TimeSig*>(e);
            ts->undoChangeProperty(P_ID::SHOW_COURTESY, !ts->showCourtesySig());
            }
      else if (cmd == "ts-props") {
            TimeSig* ts = static_cast<TimeSig*>(e);
            TimeSig* r = new TimeSig(*ts);
            TimeSigProperties tsp(r);

            if (tsp.exec()) {
                  ts->undoChangeProperty(P_ID::SHOW_COURTESY,      r->showCourtesySig());
                  ts->undoChangeProperty(P_ID::NUMERATOR_STRING,   r->numeratorString());
                  ts->undoChangeProperty(P_ID::DENOMINATOR_STRING, r->denominatorString());
                  ts->undoChangeProperty(P_ID::TIMESIG_TYPE,       int(r->timeSigType()));
                  ts->undoChangeProperty(P_ID::GROUPS,        QVariant::fromValue<Groups>(r->groups()));

                  if (r->sig() != ts->sig()) {
                        score()->cmdAddTimeSig(ts->measure(), ts->staffIdx(), r, true);
                        r = 0;
                        }
                  }
            delete r;
            }
      else if (cmd == "smallNote")
            score()->undoChangeProperty(e, P_ID::SMALL, !static_cast<Note*>(e)->small());
      else if (cmd == "clef-courtesy") {
            bool show = !static_cast<Clef*>(e)->showCourtesy();
            score()->undoChangeProperty(e, P_ID::SHOW_COURTESY, show);
            }
      else if (cmd == "st-props") {
            StaffTextProperties rp(static_cast<StaffText*>(e));
            if (rp.exec()) {
                  Score* score = e->score();
                  StaffText* nt = rp.staffText()->clone();
                  nt->setScore(score);
                  score->undoChangeElement(e, nt);
                  score->updateChannel();
                  score->updateSwing();
                  score->setPlaylistDirty();
                  }
            }
      else if (cmd == "text-style") {
            Text* t = static_cast<Text*>(e);
            QString name = t->textStyle().name();
            TextStyleDialog ts(0, score());
            ts.setPage(name);
            ts.exec();
            }
      else if (cmd == "text-props") {
            Text* ot    = static_cast<Text*>(e);
            Text* nText = static_cast<Text*>(ot->clone());
            TextProperties tp(nText);
            int rv = tp.exec();
            if (rv) {
                  qDebug("text-props %d %d", int(ot->textStyleType()), int(nText->textStyleType()));
                  if (ot->textStyleType() != nText->textStyleType()) {
                        nText->restyle(ot->textStyleType());
                        ot->undoChangeProperty(P_ID::TEXT_STYLE_TYPE, int(nText->textStyleType()));
                        }
                  if (ot->textStyle() != nText->textStyle())
                        ot->undoChangeProperty(P_ID::TEXT_STYLE, QVariant::fromValue<TextStyle>(nText->textStyle()));
                  if (ot->xmlText() != nText->xmlText())
                        ot->undoChangeProperty(P_ID::TEXT, nText->xmlText());
                  }
            delete nText;
            }
      else if (cmd == "key-courtesy") {
            KeySig* ks = static_cast<KeySig*>(e);
            score()->undo(new ChangeKeySig(ks, ks->keySigEvent(), !ks->showCourtesy() /*, ks->showNaturals()*/));
            }
      else if (cmd == "ss-props") {
            StaffState* ss = static_cast<StaffState*>(e);
            SelectInstrument si(ss->instrument(), 0);
            if (si.exec()) {
                  const InstrumentTemplate* it = si.instrTemplate();
                  if (it) {
                        // TODO: undo/redo
                        ss->setInstrument(Instrument::fromTemplate(it));
                        ss->staff()->part()->setInstrument(ss->instrument(), ss->segment()->tick());
                        score()->rebuildMidiMapping();
                        seq->initInstruments();
                        score()->setLayoutAll(true);
                        }
                  else
                        qDebug("no template selected?");
                  }
            }
      else if (cmd == "articulation") {
            Note* note = static_cast<Note*>(e);
            mscore->editInPianoroll(note->staff());
            }
      else if (cmd == "style") {
            EditStyle es(e->score(), 0);
            es.setPage(EditStyle::PAGE_NOTE);
            es.exec();
            }
      else if (cmd == "break-props") {
            LayoutBreak* lb = static_cast<LayoutBreak*>(e);
            SectionBreakProperties sbp(lb, 0);
            if (sbp.exec()) {
                  if (lb->pause() != sbp.pause()
                     || lb->startWithLongNames() != sbp.startWithLongNames()
                     || lb->startWithMeasureOne() != sbp.startWithMeasureOne()) {
                        LayoutBreak* nlb = new LayoutBreak(*lb);
                        nlb->setParent(lb->parent());
                        nlb->setPause(sbp.pause());
                        nlb->setStartWithLongNames(sbp.startWithLongNames());
                        nlb->setStartWithMeasureOne(sbp.startWithMeasureOne());
                        // propagate in parts
                        score()->undoChangeProperty(lb, P_ID::PAUSE, sbp.pause());
                        score()->undoChangeElement(lb, nlb);
                        }
                  }
            }
      else if (cmd == "ch-instr") {
            InstrumentChange* ic = static_cast<InstrumentChange*>(e);
            SelectInstrument si(ic->instrument(), 0);
            if (si.exec()) {
                  const InstrumentTemplate* it = si.instrTemplate();
                  if (it) {
                        int tickStart = ic->segment()->tick();
                        Part* part = ic->staff()->part();
                        Interval oldV = part->instrument(tickStart)->transpose();
                        //Instrument* oi = ic->instrument();  //part->instrument(tickStart);
                        // change instrument in all linked scores
                        for (ScoreElement* se : ic->linkList()) {
                              InstrumentChange* lic = static_cast<InstrumentChange*>(se);
                              Instrument* instrument = new Instrument(Instrument::fromTemplate(it));
                              lic->score()->undo(new ChangeInstrument(lic, instrument));
                              }
                        // transpose for current score only
                        // this automatically propagates to linked scores
                        if (part->instrument(tickStart)->transpose() != oldV) {
                              auto i = part->instruments()->upper_bound(tickStart);    // find(), ++i
                              int tickEnd;
                              if (i == part->instruments()->end())
                                    tickEnd = -1;
                              else
                                    tickEnd = i->first;
                              ic->score()->transpositionChanged(part, oldV, tickStart, tickEnd);
                              }
                        }
                  else
                        qDebug("no template selected?");
                  }
           }
      else if (cmd == "fret-props")
            editFretDiagram(static_cast<FretDiagram*>(e));
      else if (cmd == "gliss-props") {
            GlissandoProperties vp(static_cast<Glissando*>(e));
            vp.exec();
            }
      else if (cmd == "staff-props") {
            int tick = -1;
            if (e->isChordRest())
                  tick = static_cast<ChordRest*>(e)->tick();
            else if (e->type() == Element::Type::NOTE)
                  tick = static_cast<Note*>(e)->chord()->tick();
            else if (e->type() == Element::Type::MEASURE)
                  tick = static_cast<Measure*>(e)->tick();
            EditStaff editStaff(e->staff(), tick, 0);
            connect(&editStaff, SIGNAL(instrumentChanged()), mscore, SLOT(instrumentChanged()));
            editStaff.exec();
            }
      else if (cmd.startsWith("layer-")) {
            int n = cmd.mid(6).toInt();
            uint mask = 1 << n;
            e->setTag(mask);
            }
      }

//---------------------------------------------------------
//   editFretDiagram
//---------------------------------------------------------

void ScoreView::editFretDiagram(FretDiagram* fd)
      {
      FretDiagram* nFret = const_cast<FretDiagram*>(fd->clone());
      FretDiagramProperties fp(nFret, 0);
      int rv = fp.exec();
      nFret->layout();
      if (rv) {
            for (ScoreElement* ee : fd->linkList()) {
                  Element* e = static_cast<Element*>(ee);
                  FretDiagram* f = static_cast<FretDiagram*>(nFret->clone());
                  f->setScore(e->score());
                  f->setTrack(e->track());
                  e->score()->undoChangeElement(e, f);
                  }
            }
      delete nFret;
      }

//---------------------------------------------------------
//   editBendProperties
//---------------------------------------------------------

void ScoreView::editBendProperties(Bend* bend)
      {
      BendProperties bp(bend, 0);
      if (bp.exec()) {
            for (ScoreElement* b : bend->linkList())
                  b->score()->undo(new ChangeBend(static_cast<Bend*>(b), bp.points()));
            }
      }

//---------------------------------------------------------
//   editTremoloBarProperties
//---------------------------------------------------------

void ScoreView::editTremoloBarProperties(TremoloBar* tb)
      {
      TremoloBarProperties bp(tb, 0);
      if (bp.exec()) {
            for (ScoreElement* b : tb->linkList())
                  score()->undo(new ChangeTremoloBar(static_cast<TremoloBar*>(b), bp.points()));
            }
      }
}

