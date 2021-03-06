//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>

#include "libmscore/mscore.h"
#include "libmscore/score.h"
#include "libmscore/note.h"
#include "libmscore/accidental.h"
#include "libmscore/chord.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/tremolo.h"
#include "libmscore/articulation.h"
#include "libmscore/sym.h"
#include "libmscore/key.h"
#include "libmscore/pitchspelling.h"
#include "mtest/testutils.h"

#define DIR QString("libmscore/durationtype/")

using namespace Ms;

//---------------------------------------------------------
//   TestDurationType
//---------------------------------------------------------

class TestDurationType : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase();
      void halfDuration();
      void doubleDuration();
      void decDurationDotted();
      void incDurationDotted();
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestDurationType::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
//   halfDuration
//    Simple tests for command "half-duration" (default shortcut "Q").
//    starts with Whole note and repeatedly applies cmdHalfDuration()
//---------------------------------------------------------

void TestDurationType::halfDuration()
{
      score = readScore(DIR + "empty.mscx");
      score->doLayout();
      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(0, false, Segment::Type::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_WHOLE);
      score->inputState().setNoteEntryMode(true);

      score->cmdAddPitch(42, false);
      QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(1, 1));

      // repeatedly half-duration from V_WHOLE to V_128
      for (int i = 128; i > 1; i /= 2) {
            score->cmdHalfDuration();
            QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(i / 2, 128));
            }
}

//---------------------------------------------------------
//   halfDuration
//    Simple tests for command "double-duration" (default shortcut "W").
//    Starts with 128th note and repeatedly applies cmdDoubleDuration() up to Whole note.
//---------------------------------------------------------

void TestDurationType::doubleDuration()
{
    score = readScore(DIR + "empty.mscx");
    score->doLayout();
    score->inputState().setTrack(0);
    score->inputState().setSegment(score->tick2segment(0, false, Segment::Type::ChordRest));
    score->inputState().setDuration(TDuration::DurationType::V_128TH);
    score->inputState().setNoteEntryMode(true);

    score->cmdAddPitch(42, false);
    QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(1, 128));

    // repeatedly double-duration from V_128 to V_WHOLE
    for (int i = 1; i < 128; i *= 2) {
          score->cmdDoubleDuration();
          QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(2 * i, 128));
          }
}

//---------------------------------------------------------
//   decDurationDotted
//    Simple tests for command "dec-duration-dotted" (default shortcut "Shift+Q").
//    Starts with Whole note and repeatedly applies cmdDecDurationDotted() down to 64th note.
//---------------------------------------------------------

void TestDurationType::decDurationDotted()
{
      score = readScore(DIR + "empty.mscx");
      score->doLayout();
      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(0, false, Segment::Type::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_WHOLE);
      score->inputState().setNoteEntryMode(true);

      score->cmdAddPitch(42, false);
      QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(1, 1));

      // repeatedly dec-duration-dotted from V_WHOLE to V_64
      for (int i = 128; i > 2; i /= 2) {
            score->cmdDecDurationDotted();
            QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(i + i/2, 256));

            score->cmdDecDurationDotted();
            QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(i/2, 128));
            }
}

//---------------------------------------------------------
//   incDurationDotted
//    Simple tests for command "inc-duration-dotted" (default shortcut "Shift+W").
//    Starts with 64th note and repeatedly applies cmdIncDurationDotted() up to Whole note.
//---------------------------------------------------------

void TestDurationType::incDurationDotted()
{
      score = readScore(DIR + "empty.mscx");
      score->doLayout();
      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(0, false, Segment::Type::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_64TH);
      score->inputState().setNoteEntryMode(true);

      score->cmdAddPitch(42, false);
      QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(1, 64));

      // repeatedly inc-duration-dotted from V_64 to V_WHOLE
      for (int i = 1; i < 64; i *= 2) {
            score->cmdIncDurationDotted();
            QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(3 * i, 128));

            score->cmdIncDurationDotted();
            QVERIFY(score->firstMeasure()->findChord(0, 0)->duration() == Fraction(i, 32));
            }
}

QTEST_MAIN(TestDurationType)

#include "tst_durationtype.moc"

