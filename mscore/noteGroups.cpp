//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2013 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "noteGroups.h"
#include "libmscore/chord.h"
#include "libmscore/mcursor.h"
#include "libmscore/timesig.h"
#include "libmscore/score.h"
#include "libmscore/part.h"
#include "libmscore/key.h"
#include "libmscore/icon.h"
#include "musescore.h"

namespace Ms {

extern void populateIconPalette(Palette* p, const IconAction* a);

//---------------------------------------------------------
//   createScore
//---------------------------------------------------------

Score* NoteGroups::createScore(int n, TDuration::DurationType t, std::vector<Chord*>* chords)
      {
      MCursor c;
      c.setTimeSig(_sig);
      c.createScore("score8");
      c.addPart("voice");
      c.move(0, 0);
      c.addKeySig(Key::C);
      TimeSig* nts = c.addTimeSig(_sig);
      GroupNode node {0, 0};
      Groups ng;
      ng.push_back(node);
      nts->setGroups(ng);

      for (int i = 0; i < n; ++i) {
            Chord* chord = c.addChord(67, t);
            int tick = chord->rtick();
            chord->setBeamMode(_groups.beamMode(tick, t));
            chords->push_back(chord);
            }

      c.score()->parts().front()->setLongName("");
      c.score()->style()->set(StyleIdx::linearStretch, 1.3);
      c.score()->style()->set(StyleIdx::MusicalSymbolFont, QString("Bravura"));
      c.score()->style()->set(StyleIdx::MusicalTextFont, QString("Bravura Text"));
      return c.score();
      }

//---------------------------------------------------------
//   NoteGroups
//---------------------------------------------------------

NoteGroups::NoteGroups(QWidget* parent)
   : QGroupBox(parent)
      {
      setupUi(this);
      static const IconAction bpa[] = {
            { IconType::SBEAM,    "beam-start" },
            { IconType::MBEAM,    "beam-mid" },
            { IconType::BEAM32,   "beam32" },
            { IconType::BEAM64,   "beam64" },
            { IconType::NONE,     ""}
            };

      iconPalette->setName(QT_TRANSLATE_NOOP("Palette", "Beam Properties"));
      iconPalette->setGrid(27, 40);
      iconPalette->setMag(.5);
      iconPalette->setDrawGrid(true);
      populateIconPalette(iconPalette, bpa);
      iconPalette->setReadOnly(true);

      connect(resetGroups, SIGNAL(clicked()), SLOT(resetClicked()));
      connect(view8,  SIGNAL(noteClicked(Note*)), SLOT(noteClicked(Note*)));
      connect(view16, SIGNAL(noteClicked(Note*)), SLOT(noteClicked(Note*)));
      connect(view32, SIGNAL(noteClicked(Note*)), SLOT(noteClicked(Note*)));
      connect(view8,  SIGNAL(beamPropertyDropped(Chord*,Icon*)), SLOT(beamPropertyDropped(Chord*,Icon*)));
      connect(view16, SIGNAL(beamPropertyDropped(Chord*,Icon*)), SLOT(beamPropertyDropped(Chord*,Icon*)));
      connect(view32, SIGNAL(beamPropertyDropped(Chord*,Icon*)), SLOT(beamPropertyDropped(Chord*,Icon*)));
      }

//---------------------------------------------------------
//   setSig
//---------------------------------------------------------

void NoteGroups::setSig(Fraction sig, const Groups& g)
      {
      _sig    = sig;
      _groups = g;
      chords8.clear();
      chords16.clear();
      chords32.clear();
      Fraction f = _sig.reduced();
      int n   = f.numerator() * (8 / f.denominator());
      view8->setScore(createScore(n, TDuration::DurationType::V_EIGHTH, &chords8));
      n   = f.numerator() * (16 / f.denominator());
      view16->setScore(createScore(n, TDuration::DurationType::V_16TH, &chords16));
      n   = f.numerator() * (32 / f.denominator());
      view32->setScore(createScore(n, TDuration::DurationType::V_32ND, &chords32));
      view8->resetMatrix();
      view16->resetMatrix();
      view32->resetMatrix();
      }

//---------------------------------------------------------
//   groups
//---------------------------------------------------------

Groups NoteGroups::groups()
      {
      Groups g;
      for (Chord* chord : chords8)
            g.addStop(chord->rtick(), chord->durationType().type(), chord->beamMode());
      for (Chord* chord : chords16)
            g.addStop(chord->rtick(), chord->durationType().type(), chord->beamMode());
      for (Chord* chord : chords32)
            g.addStop(chord->rtick(), chord->durationType().type(), chord->beamMode());
      return g;
      }

//---------------------------------------------------------
//   resetClicked
//---------------------------------------------------------

void NoteGroups::resetClicked()
      {
      setSig(_sig, _groups);
      }

//---------------------------------------------------------
//   noteClicked
//---------------------------------------------------------

void NoteGroups::noteClicked(Note* note)
      {
      Chord* chord = note->chord();
      if (chord->beamMode() == Beam::Mode::AUTO)
            updateBeams(chord, Beam::Mode::BEGIN);
      else if (chord->beamMode() == Beam::Mode::BEGIN)
            updateBeams(chord, Beam::Mode::AUTO);
      }

//---------------------------------------------------------
//   beamPropertyDropped
//---------------------------------------------------------

void NoteGroups::beamPropertyDropped(Chord* chord, Icon* icon)
      {
      switch (icon->iconType()) {
            case IconType::SBEAM:
                  updateBeams(chord, Beam::Mode::BEGIN);
                  break;
            case IconType::MBEAM:
                  updateBeams(chord, Beam::Mode::AUTO);
                  break;
            case IconType::BEAM32:
                  updateBeams(chord, Beam::Mode::BEGIN32);
                  break;
            case IconType::BEAM64:
                  updateBeams(chord, Beam::Mode::BEGIN64);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   updateBeams
//     takes into account current state of changeShorterCheckBox to update smaller valued notes as well
//---------------------------------------------------------

void NoteGroups::updateBeams(Chord* chord, Beam::Mode m)
      {
      chord->setBeamMode(m);
      chord->score()->doLayout();

      if (changeShorterCheckBox->checkState() == Qt::Checked) {
            int tick = chord->tick();
            bool foundChord = false;
            for (Chord* c : chords8) {
                  if (c == chord) {
                        foundChord = true;
                        break;
                        }
                  }
            for (Chord* c : chords16) {
                  if (foundChord) {
                        if (c->tick() == tick) {
                              c->setBeamMode(m);
                              c->score()->doLayout();
                              break;
                              }
                        }
                  else if (c == chord) {
                        foundChord = true;
                        break;
                        }
                  }
            for (Chord* c : chords32) {
                  if (foundChord) {
                        if (c->tick() == tick) {
                              c->setBeamMode(m);
                              c->score()->doLayout();
                              break;
                              }
                        }
                  }
            }

      view8->update();
      view16->update();
      view32->update();
      }

}

