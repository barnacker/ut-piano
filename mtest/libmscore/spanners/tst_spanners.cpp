//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2015 Werner Schweer & others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/chord.h"
#include "libmscore/excerpt.h"
#include "libmscore/glissando.h"
#include "libmscore/layoutbreak.h"
#include "libmscore/lyrics.h"
#include "libmscore/measure.h"
#include "libmscore/part.h"
#include "libmscore/staff.h"
#include "libmscore/score.h"
#include "libmscore/system.h"
#include "libmscore/undo.h"

#define DIR QString("libmscore/spanners/")

using namespace Ms;

//---------------------------------------------------------
//   TestSpanners
//---------------------------------------------------------

class TestSpanners : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase();
      void spanners01();            // adding glissandos in several contexts
      void spanners02();            // loading back existing cross-staff glissando from lower to higher staff
      void spanners03();            // adding glissandos from/to grace notes
      void spanners04();            // linking a staff to a staff containing a glissando
      void spanners05();            // creating part from an existing staff containing a glissando
      void spanners06();            // add a glissando to a staff with a linked staff
      void spanners07();            // add a glissando to a staff with an excerpt attached
      void spanners08();            // delete a lyrics with separator and undo
      void spanners09();            // remove a measure containing the end point of a LyricsLine and undo
      void spanners10();            // remove a measure containing the start point of a LyricsLine and undo
      void spanners11();            // remove a measure entirely containing a LyricsLine and undo
      void spanners12();            // remove a measure containing the middle portion of a LyricsLine and undo
      void spanners13();            // drop a line break at the middle of a LyricsLine and check LyricsLineSegments
      void spanners14();            // creating part from an existing grand staff containing a cross staff glissando
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestSpanners::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
///  spanners01
///   Adds glissandi in several contexts.
//---------------------------------------------------------

void TestSpanners::spanners01()
      {
      DropData    dropData;
      Glissando*  gliss;

      Score* score = readScore(DIR + "glissando01.mscx");
      QVERIFY(score);
      score->doLayout();

      // SIMPLE CASE: GLISSANDO FROM A NOTE TO THE FOLLOWING
      // go to top note of first chord
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(Segment::Type::ChordRest, 0);
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score); // create a new element each time, as drop() will eventually delete it
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM TOP STAFF TO BOTTOM STAFF
      // go to top note of first chord of next measure
      msr   = msr->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(0));   // voice 0 of staff 0
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM BOTTOM STAFF TO TOP STAFF
      // go to bottom note of first chord of next measure
      msr   = msr->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(4));   // voice 0 of staff 1
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO OVER INTERVENING NOTES IN ANOTHER VOICE
      // go to top note of first chord of next measure
      msr   = msr->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(0));   // voice 0 of staff 0
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO OVER INTERVENING NOTES IN ANOTHER STAFF
      // go to top note of first chord of next measure
      msr   = msr->nextMeasure()->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(0));   // voice 0 of staff 0
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando01.mscx", DIR + "glissando01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners02
///   Check loading of score with a glissando from a lower to a higher staff:
//    A score with:
//          grand staff,
//          glissando from a bass staff note to a treble staff note
//    is loaded and laid out and saved: should be round-trip safe.
//---------------------------------------------------------

void TestSpanners::spanners02()
      {
      Score* score = readScore(DIR + "glissando-crossstaff01.mscx");
      QVERIFY(score);
      score->doLayout();

      QVERIFY(saveCompareScore(score, "glissando-crsossstaff01.mscx", DIR + "glissando-crossstaff01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners03
///   Loads a score with before- and after-grace notes and adds several glissandi from/to them.
//---------------------------------------------------------

void TestSpanners::spanners03()
      {
      DropData    dropData;
      Glissando*  gliss;

      Score* score = readScore(DIR + "glissando-graces01.mscx");
      QVERIFY(score);
      score->doLayout();

      // GLISSANDO FROM MAIN NOTE TO AFTER-GRACE
      // go to top note of first chord
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(Segment::Type::ChordRest, 0);
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score); // create a new element each time, as drop() will eventually delete it
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM AFTER-GRACE TO BEFORE-GRACE OF NEXT CHORD
      // go to last after-grace of chord and drop a glissando on it
      Ms::Chord*      grace = chord->graceNotesAfter().last();
      QVERIFY(grace && grace->type() == Element::Type::CHORD);
      note              = grace->upNote();
      QVERIFY(note);
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM MAIN NOTE TO BEFORE-GRACE OF NEXT CHORD
      // go to next chord
      seg               = seg->nextCR(0);
      QVERIFY(seg);
      chord             = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      note              = chord->upNote();
      QVERIFY(note);
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM BEFORE-GRACE TO MAIN NOTE
      // go to next chord
      seg               = seg->nextCR(0);
      QVERIFY(seg);
      chord             = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      // go to its last before-grace note
      grace             = chord->graceNotesBefore().last();
      QVERIFY(grace && grace->type() == Element::Type::CHORD);
      note              = grace->upNote();
      QVERIFY(note);
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando-graces01.mscx", DIR + "glissando-graces01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners04
///   Linking a staff to an existing staff containing a glissando
//---------------------------------------------------------

void TestSpanners::spanners04()
      {

      Score* score = readScore(DIR + "glissando-cloning01.mscx");
      QVERIFY(score);
      score->doLayout();

      // add a linked staff to the existing staff
      // (copied and adapted from void MuseScore::editInstrList() in mscore/instrdialog.cpp)
      Staff* oldStaff   = score->staff(0);
      Staff* newStaff   = new Staff(score);
      newStaff->setPart(oldStaff->part());
      newStaff->initFromStaffType(oldStaff->staffType());
      newStaff->setDefaultClefType(ClefTypeList(ClefType::G));

      KeySigEvent ke;
      ke.setKey(Key::C);
      newStaff->setKey(0, ke);

      score->undoInsertStaff(newStaff, 1, false);
      cloneStaff(oldStaff, newStaff);

      QVERIFY(saveCompareScore(score, "glissando-cloning01.mscx", DIR + "glissando-cloning01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners05
///   Creating part from an existing staff containing a glissando
//---------------------------------------------------------

void TestSpanners::spanners05()
      {

      Score* score = readScore(DIR + "glissando-cloning02.mscx");
      QVERIFY(score);
      score->doLayout();

      // create parts
      // (copied and adapted from void TestParts::createParts() in mtest/libmscore/parts/tst_parts.cpp)
      QList<Part*> parts;
      parts.append(score->parts().at(0));
      Score* nscore = new Score(score);

      Excerpt ex(score);
      ex.setPartScore(nscore);
      ex.setTitle(parts.front()->longName());
      ex.setParts(parts);
      ::createExcerpt(&ex);
      QVERIFY(nscore);

      nscore->setName(parts.front()->partName());
      score->undo(new AddExcerpt(nscore));

      QVERIFY(saveCompareScore(score, "glissando-cloning02.mscx", DIR + "glissando-cloning02-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners06
///   Drop a glissando on a staff with a linked staff
//---------------------------------------------------------

void TestSpanners::spanners06()
      {
      DropData    dropData;
      Glissando*  gliss;

      Score* score = readScore(DIR + "glissando-cloning03.mscx");
      QVERIFY(score);
      score->doLayout();

      // DROP A GLISSANDO ON FIRST NOTE
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(Segment::Type::ChordRest, 0);
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando-cloning03.mscx", DIR + "glissando-cloning03-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners07
///   Drop a glissando on a staff with an excerpt
//---------------------------------------------------------

void TestSpanners::spanners07()
      {
      DropData    dropData;
      Glissando*  gliss;

      Score* score = readScore(DIR + "glissando-cloning04.mscx");
      QVERIFY(score);
      score->doLayout();

      // DROP A GLISSANDO ON FIRST NOTE
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(Segment::Type::ChordRest, 0);
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.element  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando-cloning04.mscx", DIR + "glissando-cloning04-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners08
///   Delete a lyrics with separator and undo
//---------------------------------------------------------

void TestSpanners::spanners08()
      {
      Score* score = readScore(DIR + "lyricsline01.mscx");
      QVERIFY(score);
      score->doLayout();

      // verify initial LyricsLine setup
      System* sys = score->systems()->at(0);
      QVERIFY(sys->spannerSegments().size() == 1);
      QVERIFY(score->unmanagedSpanners().size() == 1);

      // DELETE LYRICS
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(Segment::Type::ChordRest, 0);
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == Element::Type::CHORD);
      QVERIFY(chord->lyricsList().size() > 0);
      Lyrics*     lyr   = chord->lyrics(0);
      score->startCmd();
      score->undoRemoveElement(lyr);
      score->endCmd();

      // verify setup after deletion
      QVERIFY(sys->spannerSegments().size() == 0);
      QVERIFY(score->unmanagedSpanners().size() == 0);

      // save and verify score after deletion
      QVERIFY(saveCompareScore(score, "lyricsline01.mscx", DIR + "lyricsline01-ref.mscx"));

      // UNDO
      score->undo()->undo();
      score->doLayout();

      // verify setup after undo
      QVERIFY(sys->spannerSegments().size() == 1);
      QVERIFY(score->unmanagedSpanners().size() == 1);

      // save and verify score after undo
      QVERIFY(saveCompareScore(score, "lyricsline01.mscx", DIR + "lyricsline01.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners09
///   Remove a measure containing the end point of a LyricsLine and undo
//
//  +---spanner---+
//         +---remove----+
//
//---------------------------------------------------------

void TestSpanners::spanners09()
      {
      Score* score = readScore(DIR + "lyricsline02.mscx");
      QVERIFY(score);
      score->doLayout();

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdDeleteSelectedMeasures();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline02.mscx", DIR + "lyricsline02-ref.mscx"));

      // UNDO AND VERIFY
      score->undo()->undo();
      QVERIFY(saveCompareScore(score, "lyricsline02.mscx", DIR + "lyricsline02.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners10
///   Remove a measure containing the start point of a LyricsLine and undo
//
//         +---spanner---+
//  +---remove----+
//
//---------------------------------------------------------

void TestSpanners::spanners10()
      {
      Score* score = readScore(DIR + "lyricsline03.mscx");
      QVERIFY(score);
      score->doLayout();

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdDeleteSelectedMeasures();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline03.mscx", DIR + "lyricsline03-ref.mscx"));

      // UNDO AND VERIFY
      score->undo()->undo();
      QVERIFY(saveCompareScore(score, "lyricsline03.mscx", DIR + "lyricsline03.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners11
///   Remove a measure entirely containing a LyricsLine and undo
//
//         +---spanner---+
//  +-----------remove------------+
//
//---------------------------------------------------------

void TestSpanners::spanners11()
      {
      Score* score = readScore(DIR + "lyricsline04.mscx");
      QVERIFY(score);
      score->doLayout();

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdDeleteSelectedMeasures();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline04.mscx", DIR + "lyricsline04-ref.mscx"));

      // UNDO AND VERIFY
      score->undo()->undo();
      QVERIFY(saveCompareScore(score, "lyricsline04.mscx", DIR + "lyricsline04.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners12
///   Remove a measure containing the middle portion of a LyricsLine and undo
//
//  +-----------spanner-----------+
//          +---remove----+
//
//---------------------------------------------------------

void TestSpanners::spanners12()
      {
      Score* score = readScore(DIR + "lyricsline05.mscx");
      QVERIFY(score);
      score->doLayout();

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdDeleteSelectedMeasures();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline05.mscx", DIR + "lyricsline05-ref.mscx"));

      // UNDO AND VERIFY
      score->undo()->undo();
      QVERIFY(saveCompareScore(score, "lyricsline05.mscx", DIR + "lyricsline05.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners13
///   Drop a line break at a bar line inthe middle of a LyricsLine and check LyricsLineSegments are correct
//
//---------------------------------------------------------

void TestSpanners::spanners13()
      {
      DropData          dropData;
      LayoutBreak*      brk;

      Score* score = readScore(DIR + "lyricsline06.mscx");
      QVERIFY(score);
      score->doLayout();

      // DROP A BREAK AT FIRST MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      brk               = new LayoutBreak(score);
      brk->setLayoutBreakType(LayoutBreak::Type::LINE);
      dropData.pos      = msr->pagePos();
      dropData.element  = brk;
      score->startCmd();
      msr->drop(dropData);
      score->endCmd();
      // VERIFY SEGMENTS IN SYSTEMS AND THEN SCORE
      for (System* sys : *score->systems())
            QVERIFY(sys->spannerSegments().size() == 1);
      QVERIFY(saveCompareScore(score, "lyricsline06.mscx", DIR + "lyricsline06-ref.mscx"));

      // UNDO AND VERIFY
      score->undo()->undo();
      score->doLayout();      // systems need to be re-computed
      QVERIFY(saveCompareScore(score, "lyricsline06.mscx", DIR + "lyricsline06.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners14
///   creating part from an existing grand staff containing a cross staff glissando
//---------------------------------------------------------

void TestSpanners::spanners14()
      {

      Score* score = readScore(DIR + "glissando-cloning05.mscx");
      QVERIFY(score);
      score->doLayout();

      // create parts
      // (copied and adapted from void TestParts::createParts() in mtest/libmscore/parts/tst_parts.cpp)
      QList<Part*> parts;
      parts.append(score->parts().at(0));
      Score* nscore = new Score(score);

      Excerpt ex(score);
      ex.setPartScore(nscore);
      ex.setTitle(parts.front()->longName());
      ex.setParts(parts);
      ::createExcerpt(&ex);
      QVERIFY(nscore);

      nscore->setName(parts.front()->partName());
      score->undo(new AddExcerpt(nscore));

      QVERIFY(saveCompareScore(score, "glissando-cloning05.mscx", DIR + "glissando-cloning05-ref.mscx"));
      delete score;
      }


QTEST_MAIN(TestSpanners)
#include "tst_spanners.moc"

