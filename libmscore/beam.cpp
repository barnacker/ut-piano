//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "beam.h"
#include "segment.h"
#include "score.h"
#include "chord.h"
#include "sig.h"
#include "style.h"
#include "note.h"
#include "tuplet.h"
#include "system.h"
#include "tremolo.h"
#include "measure.h"
#include "undo.h"
#include "staff.h"
#include "stafftype.h"
#include "stem.h"
#include "hook.h"
#include "mscore.h"
#include "icon.h"
#include "stemslash.h"
#include "groups.h"
#include "xml.h"

namespace Ms {

//---------------------------------------------------------
//   BeamFragment
//    position of primary beam
//    idx 0 - MScore::Direction::AUTO or MScore::Direction::DOWN
//        1 - MScore::Direction::UP
//---------------------------------------------------------

struct BeamFragment {
      qreal py1[2];
      qreal py2[2];
      };

//---------------------------------------------------------
//   Beam
//---------------------------------------------------------

Beam::Beam(Score* s)
   : Element(s)
      {
      setFlags(ElementFlag::SELECTABLE);
      _direction       = MScore::Direction::AUTO;
      _up              = true;
      _distribute      = false;
      _userModified[0] = false;
      _userModified[1] = false;
      _grow1           = 1.0;
      _grow2           = 1.0;
      editFragment     = -1;
      _isGrace          = false;
      _cross            = false;
      _noSlope         = score()->styleB(StyleIdx::beamNoSlope);
      noSlopeStyle     = PropertyStyle::STYLED;
      }

//---------------------------------------------------------
//   Beam
//---------------------------------------------------------

Beam::Beam(const Beam& b)
   : Element(b)
      {
      _elements     = b._elements;
      _id           = b._id;
      foreach(QLineF* bs, b.beamSegments)
            beamSegments.append(new QLineF(*bs));
      _direction       = b._direction;
      _up              = b._up;
      _distribute      = b._distribute;
      _userModified[0] = b._userModified[0];
      _userModified[1] = b._userModified[1];
      _grow1           = b._grow1;
      _grow2           = b._grow2;
      editFragment     = b.editFragment;
      foreach(BeamFragment* f, b.fragments)
            fragments.append(new BeamFragment(*f));
      minMove          = b.minMove;
      maxMove          = b.maxMove;
      _isGrace         = b._isGrace;
      _cross           = b._cross;
      maxDuration      = b.maxDuration;
      slope            = b.slope;
      _noSlope         = b._noSlope;
      noSlopeStyle     = b.noSlopeStyle;
      }

//---------------------------------------------------------
//   Beam
//---------------------------------------------------------

Beam::~Beam()
      {
      //
      // delete all references from chords
      //
      foreach(ChordRest* cr, _elements)
            cr->setBeam(0);
      qDeleteAll(beamSegments);
      qDeleteAll(fragments);
      }

//---------------------------------------------------------
//   pagePos
//---------------------------------------------------------

QPointF Beam::pagePos() const
      {
      System* system = static_cast<System*>(parent());
      if (system == 0)
            return pos();
      qreal yp = y() + system->staff(staffIdx())->y() + system->y();
      return QPointF(pageX(), yp);
      }

//---------------------------------------------------------
//   canvasPos
//---------------------------------------------------------

QPointF Beam::canvasPos() const
      {
      QPointF p(pagePos());
      if (system() && system()->parent())
            p += system()->parent()->pos();
      return p;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Beam::add(Element* e) {
      if (e && e->isChordRest())
            addChordRest(static_cast<ChordRest*>(e));
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Beam::remove(Element* e) {
      if (e && e->isChordRest())
            removeChordRest(static_cast<ChordRest*>(e));
      }

//---------------------------------------------------------
//   addChordRest
//---------------------------------------------------------

void Beam::addChordRest(ChordRest* a)
      {
      a->setBeam(this);
      if (!_elements.contains(a)) {
            //
            // insert element in same order as it appears
            // in the score
            //
            if (a->segment() && !_elements.isEmpty()) {
                  for (int i = 0; i < _elements.size(); ++i) {
                        Segment* s = _elements[i]->segment();
                        if ((s->tick() > a->segment()->tick())
                           || ((s->tick() == a->segment()->tick()) && (a->segment()->next(Segment::Type::ChordRest) == s))
                           )  {
                              _elements.insert(i, a);
                              return;
                              }
                        }
                  }
            _elements.append(a);
            }
      }

//---------------------------------------------------------
//   removeChordRest
//---------------------------------------------------------

void Beam::removeChordRest(ChordRest* a)
      {
      if (!_elements.removeOne(a))
            qDebug("Beam::remove(): cannot find ChordRest");
      a->setBeam(0);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Beam::draw(QPainter* painter) const
      {
      painter->setBrush(QBrush(curColor()));
      painter->setPen(Qt::NoPen);
      qreal lw2 = point(score()->styleS(StyleIdx::beamWidth)) * .5 * mag();
      foreach (const QLineF* bs, beamSegments) {
            QPolygonF pg;
               pg << QPointF(bs->x1(), bs->y1()-lw2)
                  << QPointF(bs->x2(), bs->y2()-lw2)
                  << QPointF(bs->x2(), bs->y2()+lw2)
                  << QPointF(bs->x1(), bs->y1()+lw2);
            painter->drawPolygon(pg, Qt::OddEvenFill);
            }
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void Beam::move(const QPointF& offset)
      {
      Element::move(offset);
      foreach (QLineF* bs, beamSegments)
            bs->translate(offset);
      }

//---------------------------------------------------------
//   twoBeamedNotes
//    calculate stem direction of two beamed notes
//    return true if two beamed notes found
//---------------------------------------------------------

bool Beam::twoBeamedNotes()
      {
      // if not two elements or elements are not chords or chords have more than 1 note, return failure
      if ((_elements.size() != 2)
         || (_elements[0]->type() != Element::Type::CHORD)
         || _elements[1]->type() != Element::Type::CHORD) {
            return false;
            }
      const Chord* c1 = static_cast<const Chord*>(_elements[0]);
      const Chord* c2 = static_cast<const Chord*>(_elements[1]);
      if (c1->notes().size() != 1 || c2->notes().size() != 1)
            return false;

      int upDnLimit = staff()->lines() - 1;           // was '4' hard-coded in the next 2 lines
      int dist1     = c1->upLine() - upDnLimit;
      int dist2     = c2->upLine() - upDnLimit;
      if ((dist1 == -dist2) || (-dist1 == dist2)) {
            _up = false;
            Segment* s = c1->segment();
            s = s->prev1(Segment::Type::ChordRest);
            if (s && s->element(c1->track())) {
                  Chord* c = static_cast<Chord*>(s->element(c1->track()));
                  if ((c->type() == Element::Type::CHORD) && c->beam())
                        _up = c->beam()->up();
                  }
            }
      else if (qAbs(dist1) > qAbs(dist2))
            _up = dist1 > 0;
      else
            _up = dist2 > 0;
      return true;
      }

//---------------------------------------------------------
//   layout1
//---------------------------------------------------------

void Beam::layout1()
      {
      //delete old segments
      qDeleteAll(beamSegments);
      beamSegments.clear();

      maxDuration.setType(TDuration::DurationType::V_INVALID);
      Chord* c1 = 0;
      Chord* c2 = 0;

      // TAB's with stem beside staves have special layout
      if (staff()->isTabStaff() && !staff()->staffType()->stemThrough()) {
            //TABULATURES: all beams (and related chords) are:
            //    UP or DOWN according to TAB duration position
            //    slope 0
            _up   = !staff()->staffType()->stemsDown();
            slope = 0.0;
            _cross = false;
            minMove = maxMove = 0;              // no cross-beaming in TAB's!
            foreach (ChordRest* cr, _elements) {
                  if (cr->type() == Element::Type::CHORD) {
                        // set members maxDuration, c1, c2
                        if (!maxDuration.isValid() || (maxDuration < cr->durationType()))
                              maxDuration = cr->durationType();
                        c2 = static_cast<Chord*>(cr);
                        if (c1 == 0)
                              c1 = c2;
                        }
                  }
            }
      else if (staff()->isDrumStaff()) {
            if (_direction != MScore::Direction::AUTO) {
                  _up = _direction == MScore::Direction::UP;
                  }
            else {
                  foreach (ChordRest* cr, _elements) {
                        if (cr->type() == Element::Type::CHORD) {
                              c2 = static_cast<Chord*>(cr);
                              _up = c2->up();
                              break;
                              }
                        }
                  }
            for (ChordRest* cr : _elements)
                  cr->setUp(_up);
            }
      else {
            //PITCHED STAVES (and TAB's with stems through staves)
            minMove = 1000;
            maxMove = -1000;
            _isGrace = false;
            qreal mag = 0.0;

            int mUp     = 0;
            int mDown   = 0;
            int upDnLimit = staff()->lines() - 1;           // was '4' hard-coded in following code
#ifndef DISABLE_UTPIANO
        if (staff() && staff()->isUtPianoRightStaff()) {
            if (_direction != MScore::Direction::AUTO)
             _up = _direction == MScore::Direction::UP;
            else
             _up = true;
        }
        else  if (staff() && staff()->isUtPianoLeftStaff()) {
            if (_direction != MScore::Direction::AUTO)
             _up = _direction == MScore::Direction::UP;
            else
             _up = false;
        }
        else
#endif
		{
            //int staffIdx = -1;
            for (ChordRest* cr : _elements) {
                  qreal m = cr->small() ? score()->styleD(StyleIdx::smallNoteMag) : 1.0;
                  mag = qMax(mag, m);
                  if (cr->type() == Element::Type::CHORD) {
                        c2 = static_cast<Chord*>(cr);
                        if (c1 == 0)
                              c1 = c2;
                        int i = c2->staffMove();
                        if (i < minMove)
                              minMove = i;
                        if (i > maxMove)
                              maxMove = i;
                        int line = c2->upLine();
                        if ((upDnLimit - line) > mUp)
                              mUp = upDnLimit - line;
                        line = c2->downLine();
                        if (line - upDnLimit > mDown)
                              mDown = line - upDnLimit;
                        }
                  if (!maxDuration.isValid() || (maxDuration < cr->durationType()))
                        maxDuration = cr->durationType();
                  }
            setMag(mag);
            //
            // determine beam stem direction
            //
            if (_direction != MScore::Direction::AUTO) {
                  _up = _direction == MScore::Direction::UP;
                  }
            else {
                  if (maxMove > 0)            // cross staff beaming down
                        _up = false;
                  else if (c1) {
                        Measure* m = c1->measure();
                        if (c1->stemDirection() != MScore::Direction::AUTO)
                              _up = c1->stemDirection() == MScore::Direction::UP;
                        else if (m->hasVoices(c1->staffIdx()))
                              _up = !(c1->voice() % 2);
                        else if (!twoBeamedNotes()) {
                              // highest or lowest note determines stem direction
                              // interval higher is bigger -> downstem
                              // interval lower is  bigger -> upstem
                              // down-stems is preferred if equal
                              _up = mUp < mDown;
                              }
                        }
                  else
                        _up = true;
                  }
		}

            _cross = minMove < maxMove;
            // int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
            slope = 0.0;

            for (ChordRest* cr : _elements) {
                  // leave initial guess alone for moved chords within a beam that crosses staves
                  // otherwise, assume beam direction is stem direction
                  if (!_cross || !cr->staffMove())
                        cr->setUp(_up);
                  }

            }     // end of if/else(tablature)
      }

//---------------------------------------------------------
//   layoutGraceNotes
//---------------------------------------------------------

void Beam::layoutGraceNotes()
      {
      //delete old segments
      qDeleteAll(beamSegments);
      beamSegments.clear();

      maxDuration.setType(TDuration::DurationType::V_INVALID);
      Chord* c1 = 0;
      Chord* c2 = 0;

      //PITCHED STAVES (and TAB's with stems through staves)
      minMove = 1000;
      maxMove = -1000;
      _isGrace = true;
      qreal graceMag   = score()->styleD(StyleIdx::graceNoteMag);
      setMag(graceMag);

      foreach (ChordRest* cr, _elements) {
            c2 = static_cast<Chord*>(cr);
            if (c1 == 0)
                  c1 = c2;
            int i = c2->staffMove();
            if (i < minMove)
                  minMove = i;
            if (i > maxMove)
                  maxMove = i;
            if (!maxDuration.isValid() || (maxDuration < cr->durationType()))
                  maxDuration = cr->durationType();
            }
      //
      // determine beam stem direction
      //
      if (staff()->isTabStaff()) {
            //TABULATURES: all beams (and related chords) are:
            //    UP or DOWN according to TAB duration position
            //    slope 0
            _up   = !staff()->staffType()->stemsDown();
            }
      else {
            if (_direction != MScore::Direction::AUTO)
                  _up = _direction == MScore::Direction::UP;
            else {
                  ChordRest* cr = _elements[0];

                  Measure* m = cr->measure();
                  if (m->hasVoices(cr->staffIdx()))
                        _up = !(cr->voice() % 2);
                  else
                        _up = true;
                  }
            }

      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      slope   = 0.0;

      if (!_userModified[idx]) {
            foreach(ChordRest* cr, _elements)
                  cr->setUp(_up);
            }
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Beam::layout()
      {
      System* system = _elements.front()->measure()->system();
      setParent(system);

      QList<ChordRest*> crl;

      int n = 0;
      foreach(ChordRest* cr, _elements) {
            if (cr->measure()->system() != system) {
                  SpannerSegmentType st;
                  if (n == 0)
                        st = SpannerSegmentType::BEGIN;
                  else
                        st = SpannerSegmentType::MIDDLE;
                  ++n;
                  if (fragments.size() < n)
                        fragments.append(new BeamFragment);
                  layout2(crl, st, n-1);
                  crl.clear();
                  system = cr->measure()->system();
                  }
            crl.append(cr);
            }
      if (!crl.isEmpty()) {
            SpannerSegmentType st;
            if (n == 0)
                  st = SpannerSegmentType::SINGLE;
            else
                  st = SpannerSegmentType::END;
            if (fragments.size() < (n+1))
                  fragments.append(new BeamFragment);
            layout2(crl, st, n);
            }

      setbbox(QRectF());
      qreal lw2 = point(score()->styleS(StyleIdx::beamWidth)) * .5 * mag();
      foreach(const QLineF* bs, beamSegments) {
            QPolygonF a(4);
            a[0] = QPointF(bs->x1(), bs->y1()-lw2);
            a[1] = QPointF(bs->x2(), bs->y2()-lw2);
            a[2] = QPointF(bs->x2(), bs->y2()+lw2);
            a[3] = QPointF(bs->x1(), bs->y1()+lw2);
            addbbox(a.boundingRect());
            }
      }

//---------------------------------------------------------
//   shape
//---------------------------------------------------------

QPainterPath Beam::shape() const
      {
      QPainterPath pp;
      qreal lw2 = point(score()->styleS(StyleIdx::beamWidth)) * .5 * mag();
      foreach(const QLineF* bs, beamSegments) {
            QPolygonF a(5);
            a[0] = QPointF(bs->x1(), bs->y1()-lw2);
            a[1] = QPointF(bs->x2(), bs->y2()-lw2);
            a[2] = QPointF(bs->x2(), bs->y2()+lw2);
            a[3] = QPointF(bs->x1(), bs->y1()+lw2);
            a[4] = QPointF(bs->x1(), bs->y1()-lw2);
            pp.addPolygon(a);
            }
      return pp;
      }

//---------------------------------------------------------
//   contains
//---------------------------------------------------------

bool Beam::contains(const QPointF& p) const
      {
      return shape().contains(p - pagePos());
      }

//---------------------------------------------------------
//   absLimit
//---------------------------------------------------------

inline qreal absLimit(qreal val, qreal limit)
      {
      if (val > limit)
            return limit;
      if (val < -limit)
            return -limit;
      return val;
      }

//---------------------------------------------------------
//   hasNoSlope
//---------------------------------------------------------

bool Beam::hasNoSlope()
      {
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      return _noSlope && !_userModified[idx];
      }

//---------------------------------------------------------
//   slopeZero
//---------------------------------------------------------

bool Beam::slopeZero(const QList<ChordRest*>& cl)
      {
      if (hasNoSlope() || cl.size() < 2)
            return true;

      //
      // return true if beam spans a rest
      //
//      for(const ChordRest* cr : cl) {
//            if (cr->type() != Element::Type::CHORD)
//                  return true;
//            }
      int l1 = cl.front()->line();
      int le = cl.back()->line();

      // look for some pattern
      if (cl.size() == 4) {
            int l2 = cl[1]->line();
            int l3 = cl[2]->line();

            if ((l1 < le) && (l2 > l1) && (l2 > l3) && (l3 > le)) {
                  return true;
                  }
            if ((l1 == l3) && (l2 == le))
                  return true;
            }
      else if (cl.size() == 6) {
            int l2 = cl[1]->line();
            int l3 = cl[2]->line();
            int l4 = cl[3]->line();
            int l5 = cl[4]->line();
            if ((l2 > l1) && (l3 > l2) && (l1 == l4) && (l2 == l5) && (l3 == le))
                  return true;
            }
      //
      //    concave beams have a slope of 0.0
      //
      bool sameLine = true;

      slope = 0.0;
      if (cl.size() >= 3) {
            int l4 = cl[1]->line(_up);
            for (int i = 1; i < cl.size()-1; ++i) {
                  // Don't consider interior rests
                  if (cl[i]->type() != Element::Type::CHORD)
                        continue;
                  int l3 = cl[i]->line(_up);
                  if (l3 != l4)
                        sameLine = false;
                  if (_up) {
                        if (l3 < l1 && l3 < le)
                              return true;
                        }
                  else {
                        if (l3 > l1 && l3 > le)
                              return true;
                        }
                  }
            if (sameLine && (l1 == l4 || le == l4) && cl[1]->type() == Element::Type::CHORD) {
                  if (_up) {
                        if (l1 == l4 && l1 < le)
                              return true;
                        if (le == l4 && le < l1)
                              return true;
                        }
                  else {
                        if (l1 == l4 && l1 > le)
                              return true;
                        else if (le == l4 && le > l1)
                              return true;
                        }
                  }
            }
      return l1 == le;
      }

//---------------------------------------------------------
//   BeamMetric
//---------------------------------------------------------

struct Bm
      {
      signed char l;     // stem len   in 1/4 spatium units
      signed char s;     // beam slant in 1/4 spatium units
      Bm() : l(0), s(0) {}
      Bm(signed char a, signed char b) : l(a), s(b) {}
      static int key(int a, int b, int c) { return ((a & 0xff) << 16) | ((b & 0xff) << 8) | (c & 0xff); }
      };

static QHash<int, Bm> bMetrics;

//---------------------------------------------------------
//   initBeamMetrics
//---------------------------------------------------------

#define B(a,b,c,d,e) bMetrics[Bm::key(a, b, c)] = Bm(d, e);

static void initBeamMetrics()
      {
      // up  step1 step2 stemLen1 slant
      //                 (- up)   (- up)
      // =================================== C
      B(1,  10, 10, -12,  0);
      B(0,   3,  3,  11,  0);
      B(1,   3,  3, -11,  0);

      B(1,  10,  9, -12, -1);
      B(1,  10,  8, -12, -4);
      B(1,  10,  7, -12, -5);
      B(1,  10,  6, -15, -5);
      B(1,  10,  5, -16, -5);
      B(1,  10,  4, -20, -4);
      B(1,  10,  3, -20, -5);

      B(1,  10,  11, -12, 1);
      B(1,  10,  12, -13, 2);      // F
      B(1,  10,  13, -13, 2);
      B(1,  10,  14, -13, 2);
      B(1,  10,  15, -13, 2);

      B(1,  3,  4, -11, 1);
      B(1,  3,  5, -11, 2);
      B(1,  3,  6, -11, 4);
      B(1,  3,  7, -11, 5);
      B(1,  3,  8, -11, 5);
      B(1,  3,  9, -11, 5);
      B(1,  3, 10, -11, 5);

      B(0, -4, -3,  15, 1);
      B(0, -4, -2,  15, 2);
      B(0, -4, -1,  15, 2);
      B(0, -4,  0,  15, 5);
      B(0, -4,  1,  16, 5);
      B(0, -4,  2,  20, 4);
      B(0, -4,  3,  20, 5);

      B(0,  3,  4,  13, 1);
      B(0,  3,  5,  13, 2);
      B(0,  3,  6,  14, 4);
      B(0,  3,  7,  14, 4);
      B(0,  3,  8,  14, 6);

      B(0,  3,  2,  11, -1);
      B(0,  3,  1,  11, -2);
      B(0,  3,  0,  11, -5);
      B(0,  3, -1,  11, -5);
      B(0,  3, -2,  11, -5);
      B(0,  3, -3,  11, -5);
      B(0,  3, -4,  11, -5);

      // =================================== D
      B(1,  9,  9,  -13, 0);
      B(0,  2,  2,   12, 0);
      B(1,  2,  2,  -11, 0);

      B(1,  9,  8,  -13, -1);
      B(1,  9,  7,  -13, -2);
      B(1,  9,  6,  -13, -5);
      B(1,  9,  5,  -14, -5);
      B(1,  9,  4,  -16, -6);
      B(1,  9,  3,  -17, -5);
      B(1,  9,  2,  -17, -8);

      B(1,  9, 10,  -11, 1);
      B(1,  9, 11,  -11, 2);
      B(1,  9, 12,  -11, 2);
      B(1,  9, 13,  -11, 2);
      B(1,  9, 14,  -11, 2);
      B(1,  9, 15,  -11, 2);

      B(1,  2, 3,   -12, 1);
      B(1,  2, 4,   -12, 2);
      B(1,  2, 5,   -12, 4);
      B(1,  2, 6,   -12, 5);
      B(1,  2, 7,   -11, 5);
      B(1,  2, 8,   -12, 5);
      B(1,  2, 9,   -12, 8);

      B(0, -5,-4,   16, 2);
      B(0, -5,-3,   16, 2);
      B(0, -5,-2,   17, 2);
      B(0, -5,-1,   17, 2);
      B(0, -5, 0,   18, 4);
      B(0, -5, 1,   18, 5);
      B(0, -5, 2,   21, 5);

      B(0,  2, 3,   12, 1);
      B(0,  2, 4,   12, 4);
      B(0,  2, 5,   13, 4);  // F
      B(0,  2, 6,   15, 5);
      B(0,  2, 7,   15, 6);
      B(0,  2, 8,   16, 8);
      B(0,  2, 9,   16, 8);

      B(0,  2,  1,   12, -1);
      B(0,  2,  0,   12, -4);
      B(0,  2, -1,   12, -5);
      B(0,  2, -2,   12, -5);
      B(0,  2, -3,   12, -4);
      B(0,  2, -4,   12, -4);
      B(0,  2, -5,   12, -5);

      // =================================== E
      B(1, 8, 8,  -12, 0);
      B(0, 1, 1,   13, 0);
      B(1, 1, 1,  -12, 0);

      B(1, 8, 7, -12, -1);
      B(1, 8, 6, -12, -4);
      B(1, 8, 5, -12, -5);
      B(1, 8, 4, -15, -5);
      B(1, 8, 3, -16, -5);
      B(1, 8, 2, -17, -6);
      B(1, 8, 1, -19, -6);

      B(1, 15, 11, -21, -1);
      B(1, 15, 10, -21, -1);
      B(1, 15,  9, -21, -4);
      B(1, 15,  8, -21, -5);

      B(1,  1,  8, -11,  6);
      B(1,  1,  7, -11,  6);
      B(1,  1,  6, -12,  6);

      B(1,  8,  9, -12,  1);
      B(1,  8, 10, -12,  4);
      B(1,  8, 11, -12,  5);
      B(1,  8, 12, -12,  5);
      B(1,  8, 13, -12,  4);
      B(1,  8, 14, -12,  5);
      B(1,  8, 15, -12,  5);

      B(0,  1,  0, 11,  -1);
      B(0,  1, -1, 11,  -2);
      B(0,  1, -2, 11,  -5);
      B(0,  1, -3, 11,  -5);
      B(0,  1, -4, 11,  -5);
      B(0,  1, -5, 11,  -5);
      B(0,  1, -6, 11,  -5);

      B(0, 1, 2, 13, 1);
      B(0, 1, 3, 13, 2);
      B(0, 1, 4, 13, 5);
      B(0, 1, 5, 14, 5);
      B(0, 1, 6, 15, 5);
      B(0, 1, 7, 17, 5);
      B(0, 1, 8, 17, 8);

      B(0, -6, -2,  19, 2);
      B(0, -6, -1,  19, 4);
      B(0, -6,  0,  20, 4);
      B(0, -6,  1,  20, 5);

      B(0, 8, 3, 9,  -6);
      B(0, 8, 2, 12, -8);
      B(0, 8, 1, 12, -8);

      // =================================== F
      B(1, 7, 7,-13, 0);      //F
      B(0, 0, 0, 12, 0);
      B(0, 7, 7, 12, 0);

      B(1, 7, 6, -13, -1);
      B(1, 7, 5, -13, -2);
      B(1, 7, 4, -13, -5);
      B(1, 7, 3, -14, -5);
      B(1, 7, 2, -15, -6);
      B(1, 7, 1, -17, -6);
      B(1, 7, 0, -18, -8);

      B(1, 14, 10, -19, -2);
      B(1, 14,  9, -19, -2);
      B(1, 14,  8, -20, -4);
      B(1, 14,  7, -20, -5);

      B(1,  0,  5,  -9, 6);
      B(1,  0,  6, -12, 8);
      B(1,  0,  7, -12, 8);

      B(1, 7,  8, -11, 1);
      B(1, 7,  9, -11, 2);
      B(1, 7, 10, -11, 5);
      B(1, 7, 11, -11, 5);
      B(1, 7, 12, -11, 5);
      B(1, 7, 13, -11, 5);
      B(1, 7, 14, -11, 5);

      B(0, 0, -1, 12, -1);
      B(0, 0, -2, 12, -4);
      B(0, 0, -3, 12, -5);
      B(0, 0, -4, 12, -5);
      B(0, 0, -5, 12, -4);
      B(0, 0, -6, 12, -4);
      B(0, 0, -7, 12, -4);

      B(0, 0, 1, 12, 1);
      B(0, 0, 2, 12, 4);
      B(0, 0, 3, 12, 5);
      B(0, 0, 4, 15, 5);
      B(0, 0, 5, 16, 5);
      B(0, 0, 6, 17, 5);
      B(0, 0, 7, 19, 6);

      B(0, -7, -3, 21, 2);
      B(0, -7, -2, 21, 2);
      B(0, -7, -1, 21, 2);
      B(0, -7,  0, 22, 4);

      B(0, 7, 2, 12, -6);
      B(0, 7, 1, 11, -6);
      B(0, 7, 0, 11, -6);

      // =================================== G
      B(1,  6,  6, -12, 0);
      B(0, -1, -1,  13, 0);
      B(0,  6,  6,  11, 0);

      B(1, 6,  5, -12, -1);
      B(1, 6,  4, -12, -4);
      B(1, 6,  3, -13, -4);
      B(1, 6,  2, -15, -5);
      B(1, 6,  1, -13, -7);
      B(1, 6,  0, -16, -8);
      B(1, 6, -1, -16, -8);

      B(1, 13, 10, -17, -2);
      B(1, 13,  9, -17, -2);
      B(1, 13,  8, -18, -4);
      B(1, 13,  7, -18, -5);
      B(1, 13,  6, -21, -5);

      B(1, -1, 6, -10, 8);

      B(1, 6,  7, -12, 1);
      B(1, 6,  8, -12, 4);
      B(1, 6,  9, -12, 5);
      B(1, 6, 10, -12, 5);
      B(1, 6, 11, -12, 4);
      B(1, 6, 12, -12, 5);
      B(1, 6, 13, -12, 5);

      B(0, -1, -2, 11, -1);
      B(0, -1, -3, 11, -2);
      B(0, -1, -4, 11, -2);
      B(0, -1, -5, 11, -2);
      B(0, -1, -6, 11, -2);
      B(0, -1, -7, 11, -2);

      B(0, -1,  0, 13, 1);
      B(0, -1,  1, 13, 2);
      B(0, -1,  2, 13, 5);
      B(0, -1,  3, 14, 5);
      B(0, -1,  4, 17, 6);
      B(0, -1,  5, 18, 5);
      B(0, -1,  6, 18, 8);

      B(0,  6,  5, 12, -4);
      B(0,  6,  4, 12, -4);
      B(0,  6,  3, 12, -4);
      B(0,  6,  2, 12, -6);
      B(0,  6,  1, 11, -6);
      B(0,  6,  0, 12, -7);
      B(0,  6, -1, 12, -8);

      // =================================== A
      B(1,  5,  5, -11, 0);
      B(0, -2, -2,  12, 0);
      B(0,  5,  5,  11, 0);

      B(1,  5,  4, -13, -1);
      B(1,  5,  3, -13, -2);
      B(1,  5,  2, -14, -4);
      B(1,  5,  1, -15, -4);
      B(1,  5,  0, -15, -6);

      B(1, 12, 11, -15, -1);
      B(1, 12, 10, -15, -2);
      B(1, 12,  9, -15, -2);
      B(1, 12,  8, -15, -5);
      B(1, 12,  7, -16, -5);
      B(1, 12,  6, -20, -4);
      B(1, 12,  5, -20, -5);

      B(1,  5,  6, -11,  1);
      B(1,  5,  7, -11,  2);
      B(1,  5,  8, -11,  5);
      B(1,  5,  9, -11,  5);
      B(1,  5, 10, -11,  5);
      B(1,  5, 11, -11,  5);
      B(1,  5, 12, -11,  5);

      B(0, -2, -1, 12, 1);
      B(0, -2,  0, 12, 4);
      B(0, -2,  1, 12, 5);
      B(0, -2,  2, 15, 5);
      B(0, -2,  3, 16, 5);
      B(0, -2,  4, 20, 4);
      B(0, -2,  5, 20, 5);

      B(0, -2, -3, 12, -1);
      B(0, -2, -4, 13, -2);
      B(0, -2, -5, 13, -2);
      B(0, -2, -6, 13, -2);
      B(0, -2, -7, 13, -2);

      B(0,  5,  4, 11, -1);
      B(0,  5,  3, 11, -2);
      B(0,  5,  2, 11, -4);
      B(0,  5,  1, 11, -5);
      B(0,  5,  0, 11, -5);
      B(0,  5, -1, 11, -5);
      B(0,  5, -2, 11, -5);

      // =================================== B
      B(1,  4,  4, -12, 0);
      B(1, 11, 11, -13, 0);
      B(0,  4,  4,  12, 0);
      B(0, -3, -3,  13, 0);

      B(1, 11, 10, -13, -1);
      B(1, 11,  9, -13, -2);
      B(1, 11,  8, -13, -5);
      B(1, 11,  7, -14, -5);
      B(1, 11,  6, -18, -4);
      B(1, 11,  5, -18, -5);
      B(1, 11,  4, -21, -5);

      B(1,  4,  3, -12, -1);
      B(1,  4,  2, -12, -4);
      B(1,  4,  1, -14, -4);
      B(1,  4,  0, -16, -4);

      B(1, 11, 12, -14, 1);
      B(1, 11, 13, -14, 1);
      B(1, 11, 14, -14, 1);
      B(1, 11, 15, -15, 2);
      B(1, 11, 16, -15, 2);

      B(1,  4,  5, -12, 1);
      B(1,  4,  6, -12, 4);
      B(1,  4,  7, -12, 5);
      B(1,  4,  8, -12, 5);
      B(1,  4,  9, -13, 6);
      B(1,  4, 10, -12, 4);
      B(1,  4, 11, -12, 5);

      B(0,  4,  3, 12, -1);
      B(0,  4,  2, 12, -4);
      B(0,  4,  1, 12, -5);
      B(0,  4,  0, 12, -5);
      B(0,  4, -1, 13, -6);
      B(0,  4, -2, 12, -4);
      B(0,  4, -3, 12, -5);

      B(0,  4,  5, 12, 1);
      B(0,  4,  6, 12, 4);

      B(0, -3, -4, 14, -1);
      B(0, -3, -5, 14, -1);
      B(0, -3, -6, 14, -1);
      B(0, -3, -7, 15, -2);
      B(0, -3, -8, 15, -2);
      B(0, -3, -9, 15, -2);

      B(0, -3, -2, 13, 1);
      B(0, -3, -1, 13, 2);
      B(0, -3,  0, 13, 5);
      B(0, -3,  1, 14, 5);
      B(0, -3,  2, 18, 4);
      B(0, -3,  3, 18, 5);
      B(0, -3,  4, 21, 5);
      }

//---------------------------------------------------------
//   beamMetric1
//    table driven
//---------------------------------------------------------

static Bm beamMetric1(bool up, char l1, char l2)
      {
      static int initialized = false;
      if (!initialized) {
            initBeamMetrics();
            initialized = true;
            }
      return bMetrics[Bm::key(up, l1, l2)];
      }

//---------------------------------------------------------
//   adjust
//    adjust stem len for notes between start-end
//    return 1/4 spatium units
//---------------------------------------------------------

static int adjust(qreal _spatium4, int slant, const QList<ChordRest*>& cl)
      {
      int n = cl.size();
      const ChordRest* c1 = cl[0];
      const ChordRest* c2 = cl[n-1];

      QPointF p1(c1->stemPosBeam());   // canvas coordinates
      qreal slope = (slant * _spatium4) / (c2->stemPosBeam().x() - p1.x());
      int ml = -1000;
      if (c1->up()) {
            for (int i = 1; i < n; ++i) {
                  QPointF p3(cl[i]->stemPosBeam());
                  qreal yUp   = p1.y() + (p3.x() - p1.x()) * slope;
                  int l       = lrint((yUp - p3.y()) / (_spatium4));
                  ml          = qMax(ml, l);
                  }
            }
      else {
            for (int i = 1; i < n; ++i) {
                  const ChordRest* c = cl[i];
                  QPointF p3(c->stemPosBeam());
                  qreal yUp   = p1.y() + (p3.x() - p1.x()) * slope;
                  int l       = lrint((p3.y() - yUp) / (_spatium4));
                  ml          = qMax(ml, l);
                  }
            }
      // on tab staff, reduce a bit the stems (value 4 is experimental)
      // TODO : proper fix should adapt all the numeric vaues used in Beam::computeStemLen() below
      // to variable line distance
      if (c1->staff() && c1->staff()->isTabStaff()) {
            ml = (ml != 0) ? ml - 4 : 0;
            return ml;
            }
      return (ml > 0) ? ml : 0;
      }

//---------------------------------------------------------
//   adjust2
//    adjust stem position for single beams
//---------------------------------------------------------

static void adjust2(Bm& bm, const ChordRest* c1)
      {
      static const int dd[4][4] = {
            // St   H  --   S
            {0,  0,  1,  0},     // St
            {0,  0, -1,  0},     // S
            {1,  1,  1, -1},     // --
            {0,  0, -1,  0}      // H
            };
      int ys = bm.l + c1->line() * 2;
      int e1 = qAbs((ys  + 1000) % 4);
      int e2 = qAbs((ys + 1000 + bm.s) % 4);
      bm.l  -= dd[e1][e2];
      }

//---------------------------------------------------------
//   minSlant
//---------------------------------------------------------

static int minSlant(uint interval)
      {
      static const int minSlantTable[] = { 0, 1, 2, 4, 5 };
      if (interval > 4)
            return 5;
      return minSlantTable[interval];
      }

//---------------------------------------------------------
//   maxSlant
//---------------------------------------------------------

static int maxSlant(uint interval)
      {
      static const int maxSlantTable[] = { 0, 1, 4, 5, 5, 6, 7, 8 };
      if (interval > 7)
            return 8;
      return maxSlantTable[interval];
      }

//---------------------------------------------------------
//   slantTable
//---------------------------------------------------------

static int* slantTable(uint interval)
      {
      static int t[8][5] = {
            { 0, -1,  0,  0,  0 },
            { 1, -1,  0,  0,  0 },
            { 3,  4,  2, -1,  0 },
            { 4,  5, -1,  0,  0 },
            { 5, -1,  0,  0,  0 },
            { 5,  6, -1,  0,  0 },
            { 6,  5,  7, -1,  0 },
            { 6,  7,  5,  8, -1 },
            };
      if (interval > 7)
            interval = 7;
      return &t[interval][0] ;
      }

//---------------------------------------------------------
//   computeStemLen
//---------------------------------------------------------

void Beam::computeStemLen(const QList<ChordRest*>& cl, qreal& py1, int beamLevels)
      {
      qreal _spatium      = spatium();
      qreal _spatium4     = _spatium * .25;
      qreal _spStaff4     = _spatium4 * staff()->lineDistance();  // scaled to staff line distance for vert. pos. within a staff
      const ChordRest* c1 = cl.front();
      const ChordRest* c2 = cl.back();
      qreal dx            = c2->pagePos().x() - c1->pagePos().x();
      bool zeroSlant      = slopeZero(cl);

      int l1 = c1->line() * 2;
      int l2 = c2->line() * 2;

      Bm bm;

      // shorten stem length if grace notes beam is under main notes beam.
      // Value 4 estimated. Desired: to find a good formula.

      int graceStemLengthCorrection;
      if (_isGrace)
            graceStemLengthCorrection = static_cast<const Chord*>(c1)->underBeam() ? 4 : 3;
      else
            graceStemLengthCorrection = 0;

      if (beamLevels == 1) {
            bm = beamMetric1(_up, l1 / 2, l2 / 2);
            if (hasNoSlope())
                  bm.s = 0.0;

            // special case for two beamed notes: flatten to max of 1sp
            static int maxShortSlant = 4;
            if (bm.l && elements().size() == 2) {
                  //qDebug("computeStemLen: l = %d, s = %d", (int)bm.l, (int)bm.s);
                  if (bm.s > maxShortSlant) {
                        // slant downward
                        // lengthen first stem if down
                        if (bm.l > 0)
                              bm.l += bm.s - maxShortSlant;
                        // flatten beam
                        bm.s = maxShortSlant;
                        }
                  else if (bm.s < -maxShortSlant) {
                        // slant upward
                        // lengthen first stem if up
                        if (bm.l < 0)
                              bm.l -= -maxShortSlant - bm.s;
                        // flatten beam
                        bm.s = -maxShortSlant;
                        }
                  }

            if (bm.l) {
                  if (bm.l > 0)
                        bm.l -= graceStemLengthCorrection;
                  else
                        bm.l += graceStemLengthCorrection;
                  }

            if (bm.l && !(zeroSlant && cl.size() > 2)) {
                  if (cl.size() > 2) {
                        if (_up)
                              bm.l = -12 - adjust(_spStaff4, bm.s, cl);
                        else
                              bm.l = 12 + adjust(_spStaff4, bm.s, cl);
                        adjust2(bm, c1);
                        }
                  }
            else {
                  int* st = slantTable(zeroSlant ? 0 : qAbs((l2 - l1) / 2));
                  int ll1;
                  if (_up) {
                        ll1 = l1 - ((l1 & 3) ? 11 : 12);
                        int ll1m = l1 - 10;
                        int rll1 = ll1;
                        if ((l1 > 20) && (l2 > 20)) {
                              st = slantTable(zeroSlant ? 0 : 1);
                              rll1 = (zeroSlant || (l2 < l1)) ? 9 : 8;
                              }
                        for (int n = 0; ; ll1--) {
                              int i;
                              for (i = 0; st[i] != -1; ++i) {
                                    int slant = (l2 > l1) ? st[i] : -st[i];
                                    int lll1  = qMin(rll1, ll1m - n - adjust(_spStaff4, slant, cl));
                                    int ll2   = lll1 + slant;
                                    static bool ba[4][4] = {
                                          { true,  true,  false, true },
                                          { true,  true,  false, true },
                                          { false, false, false, true },
                                          { true,  true,  false, true }
                                          };
                                    if (ba[lll1 & 3][ll2 & 3]) {
                                          ll1 = lll1;
                                          bm.s = slant;
                                          break;
                                          }
                                    }
                              if (st[i] != -1)
                                    break;
                              if (++n > 4) {
                                    qDebug("beam note not found 1");
                                    break;
                                    }
                              }
                        }
                  else {
                        ll1 = ((l1 & 3) ? 11 : 12) + l1;
                        int rll1 = ll1;
                        if ((l1 < -4) && (l2 < -4)) {
                              // extend to middle line, slant is always 0 <= 1
                              st = slantTable(zeroSlant ? 0 : 1);
                              rll1 = (zeroSlant || (l2 > l1)) ? 7 : 8;
                              }
                        for (int n = 0;;ll1++) {
                              int i;
                              for (i = 0; st[i] != -1; ++i) {
                                    int slant = (l2 > l1) ? st[i] : -st[i];
                                    int lll1  = qMax(rll1, ll1 + adjust(_spStaff4, slant, cl));
                                    int e1    = lll1 & 3;
                                    int ll2   = lll1 + slant;
                                    int e2    = ll2 & 3;
                                    static bool ba[4][4] = {
                                          { true,  true,  false, true },
                                          { true,  true,  false, true },
                                          { false, false, false, true },
                                          { true,  true,  false, true }
                                          };
                                    if (ba[e1][e2]) {
                                          ll1 = lll1;
                                          bm.s = slant;
                                          break;
                                          }
                                    }
                              if (st[i] != -1)
                                    break;
                              if (++n > 4) {
                                    qDebug("beam not found 2");
                                    break;
                                    }
                              }
                        }
                  bm.l = ll1 - l1;
                  }
            }
      else if (beamLevels == 2) {
            int minS, maxS;
            if (zeroSlant)
                  minS = maxS = 0;
            else {
                  uint interval = qAbs((l2 - l1) / 2);
                  minS          = minSlant(interval);
                  maxS          = maxSlant(interval);
                  if (elements().size() == 2) {
                        minS = qMin(minS, 2);
                        maxS = qMin(maxS, 4);
                        }
                  }
            int ll1;
            if (_up) {
                  ll1 = l1 - 12;     // sp minimum to primary beam
                  int rll1 = ll1;
                  if ((l1 > 20) && (l2 > 20)) {
                        minS = zeroSlant ? 0 : 1;
                        maxS = minS;
                        rll1 = (zeroSlant || (l2 < l1)) ? 9 : 8;
                        }
                  for (int n = 0; ; ll1--) {
                        int i;
                        for (i = minS; i <= maxS; ++i) {
                              int slant = (l2 > l1) ? i : -i;
                              int lll1  = qMin(rll1, ll1 - adjust(_spStaff4, slant, cl));
                              int ll2   = lll1 + slant;
                              static bool ba[4][4] = {
                                    { true,  true,  false, false  },
                                    { true,  true,  false, false },
                                    { false, false, false, false },
                                    { false, false, false, false }
                                    };
                              if (ba[lll1 & 3][ll2 & 3]) {
                                    ll1 = lll1;
                                    break;
                                    }
                              }
                        if (i <= maxS) {
                              bm.s = l2 > l1 ? i : -i;
                              break;
                              }
                        if (++n > 4) {
                              qDebug("beam note not found 1 %d-%d", minS, maxS);
                              break;
                              }
                        }
                  }
            else {
                  ll1       = 12 + l1;
                  int rll1  = ll1;
                  bool down = l2 > l1;
                  if ((l1 < -4) && (l2 < -4)) {
                        // extend to middle line, slant is always 0 <= 1
                        minS = zeroSlant ? 0 : 1;
                        maxS = minS;
                        rll1 = (zeroSlant || down) ? 7 : 8;
                        }
                  for (int n = 0;;ll1++) {
                        int i;
                        for (i = minS; i <= maxS; ++i) {
                              int slant = down ? i : -i;
                              int lll1  = qMax(rll1, ll1 + adjust(_spStaff4, slant, cl));
                              int ll2   = lll1 + slant;
                              static bool ba[4][4] = {
                                    { true,  false, false, true  },
                                    { false, false, false, false },
                                    { false, false, false, false },
                                    { true,  false, false, true }
                                    };
                              if (ba[lll1 & 3][ll2 & 3]) {
                                    ll1 = lll1;
                                    bm.s = slant;
                                    break;
                                    }
                              }
                        if (i <= maxS)
                              break;
                        if (++n > 4) {
                              qDebug("beam not found 2");
                              break;
                              }
                        }
                  }
            bm.l = ll1 - l1;
            }
      else if (beamLevels == 3) {
            int slant;
            bool outside;
            if (zeroSlant) {
                  outside = (_up && qMin(l1, l2) <= 10) ||
                     (!_up && qMax(l1, l2) >= 6);
                  slant = 0;
                  }
            else {
                  outside = (_up && (l1 <= 10) && (l2 <= 10)) ||
                     (!_up && (l1 >= 6) && (l2 >= 6));
                  if (outside)
                        slant = *slantTable(qAbs(l1-l2) / 2);
                  else
                        slant = 4;
                  if (l1 > l2)
                        slant = -slant;
                  }
            int ll1;
            if (_up) {
                  static const int t[4] = { 3, 0, 1, 2 };
                  ll1 = l1 - 15 - adjust(_spStaff4, slant, cl);
                  ll1 = qMin(ll1, 5);
                  if (!outside)
                        ll1 -= t[ll1 & 3];      // extend to sit on line
                  }
            else {
                  ll1 = 15 + l1 + adjust(_spStaff4, slant, cl);
                  ll1 = qMax(ll1, 11);
                  if (!outside)
                        ll1 += 3 - (ll1 & 3);   // extend to hang on line
                  }
            bm.s = slant;
            bm.l = ll1 - l1;
            }
      else if (beamLevels == 4) {
            int slant = zeroSlant ? 0 : (l2 > l1 ? 4 : -4);
            int ll1;
            if (_up) {
                  ll1 = l1 - 17 - adjust(_spStaff4, slant, cl);
                  ll1 = qMin(ll1, 1);
                  static const int t[4] = { 3, 0, 1, 2 };
                  ll1 -= t[ll1 & 3];      // extend to sit on line
                  }
            else {
                  ll1 = 17 + l1 + adjust(_spStaff4, slant, cl);
                  ll1 = qMax(ll1, 15);
                  ll1 += 3 - (ll1 & 3);   // extend to hang on line
                  }
            bm.s = slant;
            bm.l = ll1 - l1;
            }
      else { // if (beamLevels > 4) {
            static const int t[] = { 0, 0, 4, 4, 8, 12, 16 }; // spatium4 added to stem len
            int n = t[beamLevels] + 12;
            bm.s = 0;
            if (_up) {
                  bm.l = -n;
                  bm.l -= adjust(_spStaff4, bm.s, cl);
                  }
            else {
                  bm.l += n;
                  bm.l += adjust(_spStaff4, bm.s, cl);
                  }
            }
      if (_isGrace && (beamLevels > 1) && bm.l) {
            if (bm.l > 0)
                  bm.l -= graceStemLengthCorrection;
            else
                  bm.l += graceStemLengthCorrection;
            }
      if (dx == 0.0)
            slope = 0.0;
      else
            slope   = (bm.s * _spatium4) / dx;
      int dy = (c1->line(_up) - c1->line(!_up)) * 2;

      py1 += (dy + bm.l) * _spStaff4;
      }

//---------------------------------------------------------
//   layout2
//---------------------------------------------------------

void Beam::layout2(QList<ChordRest*>crl, SpannerSegmentType, int frag)
      {
      if (_distribute)
            score()->respace(&crl);       // fix horizontal spacing of stems

      if (crl.isEmpty())                  // no beamed Elements
            return;
      const ChordRest* c1 = crl.front();       // first chord/rest in beam
      const ChordRest* c2 = crl.back();        // last chord/rest in beam

      int beamLevels = 1;
      for (const ChordRest* c : crl)
            beamLevels = qMax(beamLevels, c->durationType().hooks());

      BeamFragment* f = fragments[frag];
      int dIdx        = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      qreal& py1      = f->py1[dIdx];
      qreal& py2      = f->py2[dIdx];

      qreal _spatium   = spatium();
      QPointF _pagePos(pagePos());
      qreal beamMinLen = point(score()->styleS(StyleIdx::beamMinLen)) * mag();

      if (beamLevels == 4)
            _beamDist = score()->styleP(StyleIdx::beamWidth) * (1 + score()->styleD(StyleIdx::beamDistance)*4/3);
      else
            _beamDist = score()->styleP(StyleIdx::beamWidth) * (1 + score()->styleD(StyleIdx::beamDistance));

      _beamDist *= mag();
      _beamDist *= c1->staff()->mag();
      int n = crl.size();

      StaffType* tab = 0;
      if (staff()->isTabStaff() )
            tab = staff()->staffType();
      if (tab && !tab->stemThrough() ) {
            //
            // TAB STAVES with stems beside staves: beam position is fixed depending on TAB parameters and chordrest up/down
            // (all the chordrests of a beam have the same up/down, as it depends on TAB parameters if there are no voices
            // or from the voice the beam belongs to if there are voices; then, it is enough to check only the first chordrest)
            _up = c1->up();
            // compute vert. pos. of beam, relative to staff (top line = 0)
            qreal y = tab->chordRestStemPosY(c1) + (_up ? - STAFFTYPE_TAB_DEFAULTSTEMLEN_UP : STAFFTYPE_TAB_DEFAULTSTEMLEN_DN);
            y *= _spatium;
            py1 = py2 = y;          // in this case, beams are always horizontal: py1 = py2
            }
      else {
            //
            // PITCHED STAVES (or TAB with stems through staves)
            //
            qreal px1 = c1->stemPosX() + c1->pageX();
            qreal px2 = c2->stemPosX() + c2->pageX();

            if (_userModified[dIdx]) {
                  py1 += _pagePos.y();
                  py2 += _pagePos.y();

                  qreal beamY = py1;
                  slope       = (py2 - py1) / (px2 - px1);
                  //
                  // set stem direction for every chord
                  //
                  bool relayoutGrace = false;
                  for (int i = 0; i < n; ++i) {
                        Chord* c = static_cast<Chord*>(crl.at(i));
                        if (c->type() == Element::Type::REST)
                              continue;
                        QPointF p = c->upNote()->pagePos();
                        qreal y1  = beamY + (p.x() - px1) * slope;
                        bool nup  = y1 < p.y();
                        if (c->up() != nup) {
                              c->setUp(nup);
                              // guess was wrong, have to relayout
                              if (!_isGrace) {
                                    score()->layoutChords1(c->segment(), c->staffIdx());
                                    // DEBUG: attempting to layout during beam edit causes crash
                                    // probably because ledger lines are deleted and added back
                                    if (editFragment == -1)
                                          c->layout();
                                    }
                              else {
                                    relayoutGrace = true;
                                    score()->layoutChords3(c->notes(), c->staff(), 0);
                                    }
                              }
                        }
                  _up = crl.front()->up();
                  if (relayoutGrace)
                        c1->parent()->layout();
                  }
            else if (_cross) {
                  qreal beamY   = 0.0;  // y position of main beam start
                  qreal y1   = -200000;
                  qreal y2   = 200000;
                  for (int i = 0; i < n; ++i) {
                        Chord* c = static_cast<Chord*>(crl.at(i));
                        qreal y;
                        if (c->type() == Element::Type::REST)
                              continue;   //y = c->pagePos().y();
                        else
                              y  = c->upNote()->pagePos().y();
                        y1       = qMax(y1, y);
                        y2       = qMin(y2, y);
                        }
                  if (y1 > y2)
                        beamY = y2 + (y1 - y2) * .5;
                  else
                        beamY = _up ? y2 : y1;
                  py1 = beamY;

                  //
                  // set stem direction for every chord
                  //
                  for (ChordRest* cr : crl) {
                        if (cr->type() != Element::Type::CHORD)
                              continue;
                        Chord* c = static_cast<Chord*>(cr);
                        qreal y  = c->upNote()->pagePos().y();
                        bool nup = beamY < y;
                        if (c->up() != nup) {
                              c->setUp(nup);
                              // guess was wrong, have to relayout
                              score()->layoutChords1(c->segment(), c->staffIdx());
                              c->layout();
                              // TODO: this might affect chord space, which might affect segment position
                              // we should relayout entire measure
                              // this probably means starting over for beam as well
                              // see https://musescore.org/en/node/71901
                              }
                        }

                  qreal yDownMax = -300000;
                  qreal yUpMin   = 300000;

                  for (ChordRest* cr : crl) {
                        if (cr->type() != Element::Type::CHORD)
                              continue;
                        Chord* c = static_cast<Chord*>(cr);
                        bool _up = c->up();
                        qreal y = (_up ? c->upNote() : c->downNote())->pagePos().y();
                        if (_up)
                              yUpMin = qMin(y, yUpMin);
                        else
                              yDownMax = qMax(y, yDownMax);
                        }
                  qreal slant = hasNoSlope() ? 0 : _spatium;
                  if (crl.front()->up())
                        slant = -slant;
                  py1   = yUpMin + (yDownMax - yUpMin) * .5 - slant * .5;
                  slope = slant / (px2 - px1);
                  if (_direction == MScore::Direction::AUTO)
                        _up = crl.front()->up();
                  }
            else {
                  py1 = c1->stemPos().y();
                  py2 = c2->stemPos().y();      // for debug
                  computeStemLen(crl, py1, beamLevels);
                  }
            py2  = (px2 - px1) * slope + py1;   // for debug
            py2 -= _pagePos.y();                //
            py1 -= _pagePos.y();
            }

      //---------------------------------------------
      //   create beam segments
      //---------------------------------------------

      qreal x1 = crl[0]->stemPosX() + crl[0]->pageX() - pageX();

      int baseLevel = 0;      // beam level that covers all notes of beam
      int crBase[n];          // offset of beam level 0 for each chord
      bool growDown = _up;

      for (int beamLevel = 0; beamLevel < beamLevels; ++beamLevel) {

            // loop through the different groups for this beam level
            // inner loop will advance through chordrests within each group
            for (int i = 0; i < n;) {
                  ChordRest* cr1 = crl[i];
                  int l = cr1->durationType().hooks() - 1;

                  if ((cr1->type() == Element::Type::REST && i) || l < beamLevel) {
                        ++i;
                        continue;
                        }

                  // at the beginning of a group
                  // loop through chordrests looking for end
                  int c1 = i;
                  ++i;
                  bool b32 = false, b64 = false;
                  for (; i < n; ++i) {
                        ChordRest* c = crl[i];
                        ChordRest* p = i ? crl[i - 1] : 0;
                        int l = c->durationType().hooks() - 1;

                        Mode bm = Groups::endBeam(c, p);
                        b32 = (beamLevel >= 1) && (bm == Mode::BEGIN32);
                        b64 = (beamLevel >= 2) && (bm == Mode::BEGIN64);

                        if ((l >= beamLevel && (b32 || b64)) || (l < beamLevel)) {
                              if (i > 1 && crl[i-1]->type() == Element::Type::REST) {
                                    --i;
                                    }
                              break;
                              }
                        }

                  // found end of group
                  int c2 = i;
                  ChordRest* cr2 = crl[c2 - 1];

                  // if group covers whole beam, we are still at base level
                  if (c1 == 0 && c2 == n)
                        baseLevel = beamLevel;

                  // default assumption - everything grows in same direction
                  int bl = growDown ? beamLevel : -beamLevel;
                  bool growDownGroup = growDown;

                  // calculate direction for this group
                  if (beamLevel > baseLevel) {

                        if ((c1 && (cr1->up() == cr2->up()))
                            || ((c2 == n) && (cr1->up() != cr2->up()))) {
                              // matching direction for outer stems, not first group
                              // or, opposing direction for outer stems, last group
                              // recalculate beam for this group based on its *first* cr
                              growDownGroup = cr1->up();
                              }

                        else if (!c1 && (c2 < n) && (cr1->up() != cr2->up())) {
                              // opposing directions for outer stems, first (but not only) group
                              // recalculate beam for this group if necessary based on its *last* cr
                              growDownGroup = cr2->up();
                              }

                        // recalculate segment offset bl
                        int base = crBase[c1];
                        if (growDownGroup && base <= 0)
                              bl = base + beamLevel;
                        else if (growDownGroup)
                              bl = base + 1;
                        else if (!growDownGroup && base >= 0)
                              bl = base - beamLevel;
                        else if (!growDownGroup)
                              bl = base - 1;

                        }

                  // if there are more beam levels,
                  // record current beam offsets for all notes of this group for re-use
                  if (beamLevel < beamLevels - 1) {
                        for (int i = c1; i < c2; ++i)
                              crBase[i] = bl;
                        }

                  qreal stemWidth  = point(score()->styleS(StyleIdx::stemWidth));
                  qreal x2   = cr1->stemPosX() + cr1->pageX() - _pagePos.x();
                  qreal x3;

                  if ((c2 - c1) > 1) {
                        ChordRest* cr2 = crl[c2-1];
                        // create segment
                        x3 = cr2->stemPosX() + cr2->pageX() - _pagePos.x();

                        if (tab) {
                              x2 -= stemWidth * 0.5;
                              x3 += stemWidth * 0.5;
                              }
                        else {
                              if (cr1->up())
                                    x2 -= stemWidth;
                              if (!cr2->up())
                                    x3 += stemWidth;
                              }
                        }
                  else {
                        // create broken segment
                        if (cr1->type() == Element::Type::REST)
                              continue;

                        int n = crl.size();
                        qreal len = beamMinLen;
                        //
                        // find direction (by default, segment points to right)
                        //
                        // if first or last of group (including tuplet groups)
                        // unconditionally set beam at right or left side
                        Tuplet* tuplet = cr1->tuplet();
                        if (c1 == 0)
                              ;
                        else if (c1 == n - 1)
                              len = -len;
                        else if (tuplet && cr1 == tuplet->elements().first())
                              ;
                        else if (tuplet && cr1 == tuplet->elements().last())
                              len = -len;
                        else if (b32 || b64)          // end of a sub-beam group
                              len = -len;
                        else if (!(cr1->isGrace())) {
                              // inside group - here it gets more complex
                              // see http://musescore.org/en/node/42856, http://musescore.org/en/node/40806
                              // our strategy:
                              // decide if we have reached the end of a "logical" grouping
                              // even if we are not literally at the end of a beam group
                              // we do this two ways:
                              // 1) see if beam groups would have indicated a break or sub-beam if the next chord were same length as this
                              // 2) see if next note is on a "sub-beat" as defined by 2 * current note duration
                              // in either case, broken segment should point left; otherwise right
                              // however, we should try to be careful to avoid "floating" segments
                              // caused by mismatches between number of incoming versus outgoing beams
                              // so, we favor the side with more beams (to the extent we can count reliably)
                              // if there is a corner case missed, this would probably be where
                              ChordRest* prevCR = crl[c1-1];
                              ChordRest* nextCR = crl[c1+1];
                              TDuration currentDuration = cr1->durationType();
                              int currentHooks = currentDuration.hooks();

                              // since we have already established that we are not at end of sub-beam,
                              // outgoing beams should always be # hooks of next chord
                              int beamsOut = nextCR->durationType().hooks();

                              // incoming beams is normally # hooks of previous chord
                              // unless this is start of sub-beam
                              const Groups& g = cr1->staff()->group(cr1->measure()->tick());
                              Fraction stretch = cr1->staff()->timeStretch(cr1->measure()->tick());
                              int currentTick = (cr1->rtick() * stretch.numerator()) / stretch.denominator();
                              Beam::Mode bm = g.beamMode(currentTick, currentDuration.type());
                              int beamsIn;
                              if (bm == Beam::Mode::BEGIN32)
                                    beamsIn = 1;
                              else if (bm == Beam::Mode::BEGIN64)
                                    beamsIn = 2;
                              else
                                    beamsIn = prevCR->durationType().hooks();

                              // remember, we are checking whether nextCR would have started sub-beam *if* same duration as this
                              int nextTick = (nextCR->rtick() * stretch.numerator()) / stretch.denominator();
                              bm = g.beamMode(nextTick, currentDuration.type());

                              if (currentHooks - beamsOut > 1 && beamsIn > beamsOut && currentHooks > beamsIn) {
                                    // point left to avoid floating segment
                                    len = -len;
                                    }
                              else if (beamsIn < beamsOut) {
                                    // point right to avoid floating segment
                                    ;
                                    }
                              else if (bm != Beam::Mode::AUTO) {
                                    // beam group info suggests this is a logical group end as per 1) above
                                    len = -len;
                                    }
                              else {
                                    // determine if this is a logical group end as per 2) above

                                    int baseTick = tuplet ? tuplet->tick() : cr1->measure()->tick();
                                    int tickNext = nextCR->tick() - baseTick;
                                    if (tuplet) {
                                          // for tuplets with odd ratios, apply ratio
                                          // thus, we are performing calculation relative to apparent rather than actual beat
                                          // for tuplets with even ratios, use actual beat
                                          // see https://musescore.org/en/node/58061
                                          Fraction r = tuplet->ratio();
                                          if (r.numerator() & 1)
                                                tickNext = (tickNext * r.numerator()) / r.denominator();
                                          }

                                    // determine the tick length of a chord with one beam level less than this
                                    // (i.e. twice the ticks of this)

                                    int tickMod  = cr1->duration().ticks() * 2;     // (tickNext - (crl[c1]->tick() - baseTick)) * 2;

                                    // if this completes, within the measure or tuplet, a unit of tickMod length, flip beam to left
                                    // (allow some tolerance for tick rounding in tuplets
                                    // without tuplet tolerance, could be simplified)

                                    static const int BEAM_TUPLET_TOLERANCE = 6;
                                    int mod = tickNext % tickMod;
                                    if (mod <= BEAM_TUPLET_TOLERANCE || (tickMod - mod) <= BEAM_TUPLET_TOLERANCE)
                                          len = -len;
                                    }
                              }
                        if (tab) {
                              if (len > 0)
                                    x2 -= stemWidth * 0.5;
                              else
                                    x2 += stemWidth * 0.5;
                              }
                        else {
                              bool stemUp = cr1->up();
                              if (stemUp && len > 0)
                                    x2 -= stemWidth;
                              else if (!stemUp && len < 0)
                                    x2 += stemWidth;
                              }
                        x3 = x2 + len;
                        }
                  //feathered beams
                  qreal yo   = py1 + bl * _beamDist * _grow1;
                  qreal yoo  = py1 + bl * _beamDist * _grow2;
                  qreal ly1  = (x2 - x1) * slope + yo;
                  qreal ly2  = (x3 - x1) * slope + yoo;

                  if (!qIsFinite(x2) || !qIsFinite(ly1)
                     || !qIsFinite(x3) || !qIsFinite(ly2)) {
                        qDebug("bad beam segment: slope %f", slope);
                        }
                  else {
                        beamSegments.push_back(new QLineF(x2, ly1, x3, ly2));
                        }
                  }
            }

      //
      //  calculate stem length
      //
      for (ChordRest* cr : crl) {
            if (cr->type() != Element::Type::CHORD)
                  continue;
            Chord* c = static_cast<Chord*>(cr);
            c->layoutStem1();
            Stem* stem = c->stem();
#if 0
            if (!stem) {
                  // is this ever true? -> yes
qDebug("create stem in layout beam, track %d", c->track());
                  stem = new Stem(score());
                  stem->setParent(c);
                  score()->undoAddElement(stem);
//                  stem->rypos() = (c->up() ? c->downNote() : c->upNote())->rypos();
                  }
#endif
            if (c->hook())
                  score()->undoRemoveElement(c->hook());

            QPointF stemPos(c->stemPos());
            qreal x2 = stemPos.x() - _pagePos.x();
            qreal y1 = (x2 - x1) * slope + py1 + _pagePos.y();
            qreal y2 = stemPos.y();

            qreal fuzz = _spatium * .1;

            qreal by = y2 < y1 ? -1000000 : 1000000;
            foreach (const QLineF* l, beamSegments) {
                  if ((x2+fuzz) >= l->x1() && (x2-fuzz) <= l->x2()) {
                        qreal y = (x2 - l->x1()) * slope + l->y1();
                        by = y2 < y1 ? qMax(by, y) : qMin(by, y);
                        }
                  }
            if (by == -1000000 || by == 1000000) {
                  if (beamSegments.isEmpty())
                        qDebug("no BeamSegments");
                  else {
                        qDebug("BeamSegment not found: x %f  %f-%f",
                           x2, beamSegments.front()->x1(),
                           beamSegments.back()->x2());
                        }
                  }
            if (stem)
                  stem->setLen(y2 - (by + _pagePos.y()));

#if 0       // TODO ??
            if (!tab) {
                  bool _up = c->up();
                  qreal stemWidth5 = stem->lineWidth() * .5;
                  qreal noteWidth  = c->notes().size() ? c->notes().at(0)->headWidth() :
                     symbols[score()->symIdx()][quartheadSym].width(magS());
                  qreal stemX;
                  if (_up)
                        stemX = noteWidth - stemWidth5;
                  else
                        stemX = stemWidth5;
                  stem->rxpos() = stemX;
                  }
#endif


            //
            // layout stem slash for acciacatura
            //
            if ((c == crl.front()) && c->noteType() == NoteType::ACCIACCATURA) {
                  StemSlash* stemSlash = c->stemSlash();
                  // if (!stemSlash)
                  //      c->add(new StemSlash(score()));
                  if (stemSlash)
                        stemSlash->layout();
                  }
#if 0
            else {
                  if (c->stemSlash())
                        c->remove(c->stemSlash());
                  }
#endif
            Tremolo* tremolo = c->tremolo();
            if (tremolo)
                  tremolo->layout();
            }
      }

//---------------------------------------------------------
//   spatiumChanged
//---------------------------------------------------------

void Beam::spatiumChanged(qreal oldValue, qreal newValue)
      {
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      if (_userModified[idx]) {
            qreal diff = newValue / oldValue;
            foreach(BeamFragment* f, fragments) {
                  f->py1[idx] = f->py1[idx] * diff;
                  f->py2[idx] = f->py2[idx] * diff;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Beam::write(Xml& xml) const
      {
      if (_elements.isEmpty())
            return;
      xml.stag(QString("Beam id=\"%1\"").arg(_id));
      Element::writeProperties(xml);

      writeProperty(xml, P_ID::STEM_DIRECTION);
      writeProperty(xml, P_ID::DISTRIBUTE);
      writeProperty(xml, P_ID::BEAM_NO_SLOPE);
      writeProperty(xml, P_ID::GROW_LEFT);
      writeProperty(xml, P_ID::GROW_RIGHT);

      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      if (_userModified[idx]) {
            qreal _spatium = spatium();
            foreach(BeamFragment* f, fragments) {
                  xml.stag("Fragment");
                  xml.tag("y1", f->py1[idx] / _spatium);
                  xml.tag("y2", f->py2[idx] / _spatium);
                  xml.etag();
                  }
            }

      // this info is used for regression testing
      // l1/l2 is the beam position of the layout engine
      if (MScore::testMode) {
            qreal _spatium4 = spatium() * .25;
            foreach(BeamFragment* f, fragments) {
                  xml.tag("l1", int(lrint(f->py1[idx] / _spatium4)));
                  xml.tag("l2", int(lrint(f->py2[idx] / _spatium4)));
                  }
            }

      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Beam::read(XmlReader& e)
      {
      QPointF p1, p2;
      qreal _spatium = spatium();
      _id = e.intAttribute("id");
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "StemDirection") {
                  setProperty(P_ID::STEM_DIRECTION, Ms::getProperty(P_ID::STEM_DIRECTION, e));
                  e.readNext();
                  }
            else if (tag == "distribute")
                  setDistribute(e.readInt());
            else if (tag == "noSlope") {
                  setNoSlope(e.readInt());
                  noSlopeStyle = PropertyStyle::UNSTYLED;
                  }
            else if (tag == "growLeft")
                  setGrowLeft(e.readDouble());
            else if (tag == "growRight")
                  setGrowRight(e.readDouble());
            else if (tag == "y1") {
                  if (fragments.isEmpty())
                        fragments.append(new BeamFragment);
                  BeamFragment* f = fragments.back();
                  int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
                  _userModified[idx] = true;
                  f->py1[idx] = e.readDouble() * _spatium;
                  }
            else if (tag == "y2") {
                  if (fragments.isEmpty())
                        fragments.append(new BeamFragment);
                  BeamFragment* f = fragments.back();
                  int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
                  _userModified[idx] = true;
                  f->py2[idx] = e.readDouble() * _spatium;
                  }
            else if (tag == "Fragment") {
                  BeamFragment* f = new BeamFragment;
                  int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
                  _userModified[idx] = true;
                  qreal _spatium = spatium();

                  while (e.readNextStartElement()) {
                        const QStringRef& tag(e.name());
                        if (tag == "y1")
                              f->py1[idx] = e.readDouble() * _spatium;
                        else if (tag == "y2")
                              f->py2[idx] = e.readDouble() * _spatium;
                        else
                              e.unknown();
                        }
                  fragments.append(f);
                  }
            else if (tag == "l1" || tag == "l2")      // ignore
                  e.skipCurrentElement();
            else if (tag == "subtype")          // obsolete
                  e.skipCurrentElement();
            else if (!Element::readProperties(e))
                  e.unknown();
            }
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void Beam::editDrag(const EditData& ed)
      {
      int idx  = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      qreal dy = ed.delta.y();
      BeamFragment* f = fragments[editFragment];
      if (ed.curGrip == Grip::START)
            f->py1[idx] += dy;
      f->py2[idx] += dy;
      _userModified[idx] = true;
      setGenerated(false);
      if (_elements.front()->isGrace()) {
            layoutGraceNotes();
            score()->rebuildBspTree(); // ledger lines might be deleted. See #138256
            }
      else
            layout1();
      layout();
      for (ChordRest* cr : _elements) {
            if (cr->tuplet())
                  cr->tuplet()->layout();
            // TODO: would be nice to redraw stems while dragging beam,
            // the following is not sufficient - a full (?) relayout would still be needed
            //if (cr->type() == Element::Type::CHORD)
            //      static_cast<Chord*>(cr)->layoutStem();
            }
      }

//---------------------------------------------------------
//   updateGrips
//---------------------------------------------------------

void Beam::updateGrips(Grip* defaultGrip, QVector<QRectF>& grip) const
      {
      *defaultGrip = Grip::END;
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      BeamFragment* f = fragments[editFragment];

      ChordRest* c1 = nullptr;
      ChordRest* c2 = nullptr;
      int n = _elements.size();
      for (int i = 0; i < n; ++i) {
            if (_elements[i]->isChordRest()) {
                  c1 = static_cast<ChordRest*>(_elements[i]);
                  break;
                  }
            }
      for (int i = n-1; i >= 0; --i) {
            if (_elements[i]->isChordRest()) {
                  c2 = static_cast<ChordRest*>(_elements[i]);
                  break;
                  }
            }

      int y = pagePos().y();
      grip[0].translate(QPointF(c1->stemPosX() + c1->pageX(), f->py1[idx] + y));
      grip[1].translate(QPointF(c2->stemPosX() + c2->pageX(), f->py2[idx] + y));
      }

//---------------------------------------------------------
//   setBeamDirection
//---------------------------------------------------------

void Beam::setBeamDirection(MScore::Direction d)
      {
      _direction = d;
      if (d != MScore::Direction::AUTO)
            _up = d == MScore::Direction::UP;
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Beam::reset()
      {
      if (distribute())
            score()->undoChangeProperty(this, P_ID::DISTRIBUTE, false);
      if (growLeft() != 1.0)
            score()->undoChangeProperty(this, P_ID::GROW_LEFT, 1.0);
      if (growRight() != 1.0)
            score()->undoChangeProperty(this, P_ID::GROW_RIGHT, 1.0);
      if (userModified()) {
            score()->undoChangeProperty(this, P_ID::BEAM_POS, QVariant(beamPos()));
            score()->undoChangeProperty(this, P_ID::USER_MODIFIED, false);
            }
      if (beamDirection() != MScore::Direction::AUTO)
            score()->undoChangeProperty(this, P_ID::STEM_DIRECTION, int(MScore::Direction::AUTO));
      if (noSlopeStyle == PropertyStyle::UNSTYLED)
            score()->undoChangeProperty(this, P_ID::BEAM_NO_SLOPE, propertyDefault(P_ID::BEAM_NO_SLOPE), PropertyStyle::STYLED);

      setGenerated(true);
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

void Beam::startEdit(MuseScoreView*, const QPointF& p)
      {
      undoPushProperty(P_ID::BEAM_POS);
      undoPushProperty(P_ID::USER_MODIFIED);
      undoPushProperty(P_ID::GENERATED);

      QPointF pt(p - pagePos());
      qreal ydiff = 100000000.0;
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      int i = 0;
      editFragment = 0;
      foreach (BeamFragment* f, fragments) {
            qreal d = fabs(f->py1[idx] - pt.y());
            if (d < ydiff) {
                  ydiff = d;
                  editFragment = i;
                  }
            ++i;
            }
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Beam::endEdit()
      {
      Element::endEdit();
      editFragment = -1;
      // we need a full relayout to trigger stems to be redrawn
      score()->setLayoutAll(true);
      }

//---------------------------------------------------------
//   acceptDrop
//---------------------------------------------------------

bool Beam::acceptDrop(const DropData& data) const
      {
      return (data.element->type() == Element::Type::ICON)
         && ((static_cast<Icon*>(data.element)->iconType() == IconType::FBEAM1)
         || (static_cast<Icon*>(data.element)->iconType() == IconType::FBEAM2));
      }

//---------------------------------------------------------
//   drop
//---------------------------------------------------------

Element* Beam::drop(const DropData& data)
      {
      Element* e = data.element;
      if (e->type() != Element::Type::ICON)
            return 0;
      Icon* icon = static_cast<Icon*>(e);
      qreal g1;
      qreal g2;

      if (icon->iconType() == IconType::FBEAM1) {
            g1 = 1.0;
            g2 = 0.0;
            }
      else if (icon->iconType() == IconType::FBEAM2) {
            g1 = 0.0;
            g2 = 1.0;
            }
      else
            return 0;
      if (g1 != growLeft())
            score()->undoChangeProperty(this, P_ID::GROW_LEFT, g1);
      if (g2 != growRight())
            score()->undoChangeProperty(this, P_ID::GROW_RIGHT, g2);
      return 0;
      }

//---------------------------------------------------------
//   beamPos
//    misuse QPointF for y1-y2 real values
//---------------------------------------------------------

QPointF Beam::beamPos() const
      {
      if (fragments.isEmpty())
            return QPointF(0.0, 0.0);
      BeamFragment* f = fragments.back();
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      qreal _spatium = spatium();
      return QPointF(f->py1[idx] / _spatium, f->py2[idx] / _spatium);
      }

//---------------------------------------------------------
//   setBeamPos
//---------------------------------------------------------

void Beam::setBeamPos(const QPointF& bp)
      {
      if (fragments.isEmpty())
            fragments.append(new BeamFragment);
      BeamFragment* f = fragments.back();
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      _userModified[idx] = true;
      setGenerated(false);
      qreal _spatium = spatium();
      f->py1[idx] = bp.x() * _spatium;
      f->py2[idx] = bp.y() * _spatium;
      }

//---------------------------------------------------------
//   userModified
//---------------------------------------------------------

bool Beam::userModified() const
      {
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      return _userModified[idx];
      }

//---------------------------------------------------------
//   setUserModified
//---------------------------------------------------------

void Beam::setUserModified(bool val)
      {
      int idx = (_direction == MScore::Direction::AUTO || _direction == MScore::Direction::DOWN) ? 0 : 1;
      _userModified[idx] = val;
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant Beam::getProperty(P_ID propertyId) const
      {
      switch(propertyId) {
            case P_ID::STEM_DIRECTION: return int(beamDirection());
            case P_ID::DISTRIBUTE:     return distribute();
            case P_ID::GROW_LEFT:      return growLeft();
            case P_ID::GROW_RIGHT:     return growRight();
            case P_ID::USER_MODIFIED:  return userModified();
            case P_ID::BEAM_POS:       return beamPos();
            case P_ID::BEAM_NO_SLOPE:  return noSlope();
            default:
                  return Element::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool Beam::setProperty(P_ID propertyId, const QVariant& v)
      {
      switch(propertyId) {
            case P_ID::STEM_DIRECTION:
                  setBeamDirection(MScore::Direction(v.toInt()));
                  break;
            case P_ID::DISTRIBUTE:
                  setDistribute(v.toBool());
                  break;
            case P_ID::GROW_LEFT:
                  setGrowLeft(v.toDouble());
                  break;
            case P_ID::GROW_RIGHT:
                  setGrowRight(v.toDouble());
                  break;
            case P_ID::USER_MODIFIED:
                  setUserModified(v.toBool());
                  break;
            case P_ID::BEAM_POS:
                  if (userModified())
                        setBeamPos(v.toPointF());
                  break;
            case P_ID::BEAM_NO_SLOPE:
                  setNoSlope(v.toBool());
                  noSlopeStyle = PropertyStyle::UNSTYLED;
                  break;
            default:
                  if (!Element::setProperty(propertyId, v))
                        return false;
                  break;
            }
      score()->setLayoutAll(true);
      setGenerated(false);
      return true;
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Beam::propertyDefault(P_ID id) const
      {
      switch(id) {
            case P_ID::STEM_DIRECTION: return int(MScore::Direction::AUTO);
            case P_ID::DISTRIBUTE:     return false;
            case P_ID::GROW_LEFT:      return 1.0;
            case P_ID::GROW_RIGHT:     return 1.0;
            case P_ID::USER_MODIFIED:  return false;
            case P_ID::BEAM_POS:       return beamPos();
            case P_ID::BEAM_NO_SLOPE:  return score()->styleB(StyleIdx::beamNoSlope);
            default:               return Element::propertyDefault(id);
            }
      }

//---------------------------------------------------------
//   propertyStyle
//---------------------------------------------------------

PropertyStyle Beam::propertyStyle(P_ID id) const
      {
      switch (id) {
            case P_ID::BEAM_NO_SLOPE:
                  return noSlopeStyle;

            default:
                  return Element::propertyStyle(id);
            }
      }

//---------------------------------------------------------
//   resetProperty
//---------------------------------------------------------

void Beam::resetProperty(P_ID id)
      {
      switch (id) {
            case P_ID::BEAM_NO_SLOPE:
                  setNoSlope(score()->styleB(StyleIdx::beamNoSlope));
                  noSlopeStyle = PropertyStyle::STYLED;
                  break;

            default:
                  return Element::resetProperty(id);
            }
      }

//---------------------------------------------------------
//   styleChanged
//    reset all styled values to actual style
//---------------------------------------------------------

void Beam::styleChanged()
      {
      if (noSlopeStyle == PropertyStyle::STYLED)
            setNoSlope(score()->styleB(StyleIdx::beamNoSlope));
      }

}

