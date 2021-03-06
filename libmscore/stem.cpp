//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2010-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "stem.h"
#include "staff.h"
#include "chord.h"
#include "score.h"
#include "stafftype.h"
#include "hook.h"
#include "tremolo.h"
#include "note.h"
#include "xml.h"

// TEMPORARY HACK!!
#include "sym.h"
// END OF HACK

namespace Ms {

//---------------------------------------------------------
//   Stem
//    Notenhals
//---------------------------------------------------------

Stem::Stem(Score* s)
   : Element(s)
      {
      _len     = 0.0;
      _userLen = 0.0;
      setFlags(ElementFlag::SELECTABLE);
      }

//---------------------------------------------------------
//   up
//---------------------------------------------------------

bool Stem::up() const
{
   if (chord())
   {
       return chord()->up();
   }
   else
   {
#ifndef DISABLE_UTPIANO
        if (staff()->isUtPianoRightStaff())
        {
            return true;
        }
        else if (staff()->isUtPianoLeftStaff())
        {
            return false;
        }
        else
#endif
        {
           return true;
        }
    }
}

//---------------------------------------------------------
//   stemLen
//---------------------------------------------------------

qreal Stem::stemLen() const
      {
      return up() ? -_len : _len;
      }

//---------------------------------------------------------
//   lineWidth
//---------------------------------------------------------

qreal Stem::lineWidth() const
      {
      return point(score()->styleS(StyleIdx::stemWidth));
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Stem::layout()
      {
      qreal l = _len + _userLen;
      qreal _up = up() ? -1.0 : 1.0;
      l *= _up;

      qreal y1 = 0.0;                           // vertical displacement to match note attach point
      Staff* st = staff();
      if (chord() && st ) {
            if (st->isTabStaff() ) {            // TAB staves
                  if (st->staffType()->stemThrough()) {
                        // if stems through staves, gets Y pos. of stem-side note relative to chord other side
                        qreal lineDist = st->lineDistance() * spatium();
                        y1 = (chord()->downString() - chord()->upString() ) * _up * lineDist;
                        // if fret marks above lines, raise stem beginning by 1/2 line distance
                        if (!st->staffType()->onLines())
                              y1 -= lineDist * 0.5;
                        // shorten stem by 1/2 lineDist to clear the note and a little more to keep 'air' betwen stem and note
                        lineDist *= 0.7 * mag();
                        y1 += _up * lineDist;
                        }
                  // in other TAB types, no correction
                  }
            else {                              // non-TAB
                  // move stem start to note attach point
                  Note* n  = up() ? chord()->downNote() : chord()->upNote();
                  y1 += (up() ? n->stemUpSE().y() : n->stemDownNW().y());
#ifndef DISABLE_UTPIANO				  
                  if (up())
                    y1 += (n->mirror()) ? 1.0f:0.0f;
                  else
                    y1 -= (n->mirror()) ? 1.0f:0.0f;
#endif
                  rypos() = n->rypos();
                  }
            }

      line.setLine(0.0, y1, 0.0, l);

      // compute bounding rectangle
      QRectF r(line.p1(), line.p2());
      qreal lw5  = lineWidth() * .5;
      setbbox(r.normalized().adjusted(-lw5, -lw5, lw5, lw5));
      adjustReadPos();
      }

//---------------------------------------------------------
//   setLen
//---------------------------------------------------------

void Stem::setLen(qreal v)
      {
      _len = (v < 0.0) ? -v : v;
      layout();
      }

//---------------------------------------------------------
//   spatiumChanged
//---------------------------------------------------------

void Stem::spatiumChanged(qreal oldValue, qreal newValue)
      {
      _userLen = (_userLen / oldValue) * newValue;
      layout();
      Element::spatiumChanged(oldValue, newValue);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Stem::draw(QPainter* painter) const
      {
      // hide if second chord of a cross-measure pair
      if (chord() && chord()->crossMeasure() == CrossMeasure::SECOND)
            return;

      Staff* st = staff();
      bool useTab = st && st->isTabStaff();

      qreal lw = lineWidth();
      painter->setPen(QPen(curColor(), lw, Qt::SolidLine, Qt::RoundCap));
      painter->drawLine(line);

      if (!useTab || !chord())
            return;

      // TODO: adjust bounding rectangle in layout() for dots and for slash
      StaffType* stt = st->staffType();
      qreal sp = spatium();
      bool _up = up();

      // slashed half note stem
      if (chord()->durationType().type() == TDuration::DurationType::V_HALF && stt->minimStyle() == TablatureMinimStyle::SLASHED) {
            // position slashes onto stem
            qreal y = _up ? -(_len+_userLen) + STAFFTYPE_TAB_SLASH_2STARTY_UP*sp : (_len+_userLen) - STAFFTYPE_TAB_SLASH_2STARTY_DN*sp;
            // if stems through, try to align slashes within or across lines
            if (stt->stemThrough()) {
                  qreal halfLineDist = stt->lineDistance().val() * sp * 0.5;
                  qreal halfSlashHgt = STAFFTYPE_TAB_SLASH_2TOTHEIGHT * sp * 0.5;
                  y = lrint( (y + halfSlashHgt) / halfLineDist) * halfLineDist - halfSlashHgt;
                  }
            // draw slashes
            qreal hlfWdt= sp * STAFFTYPE_TAB_SLASH_WIDTH * 0.5;
            qreal sln   = sp * STAFFTYPE_TAB_SLASH_SLANTY;
            qreal thk   = sp * STAFFTYPE_TAB_SLASH_THICK;
            qreal displ = sp * STAFFTYPE_TAB_SLASH_DISPL;
            QPainterPath path;
            for (int i = 0; i < 2; ++i) {
                  path.moveTo( hlfWdt, y);            // top-right corner
                  path.lineTo( hlfWdt, y+thk);        // bottom-right corner
                  path.lineTo(-hlfWdt, y+thk+sln);    // bottom-left corner
                  path.lineTo(-hlfWdt, y+sln);        // top-left corner
                  path.closeSubpath();
                  y += displ;
                  }
            painter->setBrush(QBrush(curColor()));
            painter->setPen(Qt::NoPen);
            painter->drawPath(path);
            }

      // dots
      // NOT THE BEST PLACE FOR THIS?
      // with tablatures and stems beside staves, dots are not drawn near 'notes', but near stems
      int nDots = chord()->dots();
      if (nDots > 0 && !stt->stemThrough()) {
            qreal x     = chord()->dotPosX();
            qreal y     = ( (STAFFTYPE_TAB_DEFAULTSTEMLEN_DN * 0.2) * sp) * (_up ? -1.0 : 1.0);
            qreal step  = score()->styleS(StyleIdx::dotDotDistance).val() * sp;
            for (int dot = 0; dot < nDots; dot++, x += step)
                  drawSymbol(SymId::augmentationDot, painter, QPointF(x, y));
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Stem::write(Xml& xml) const
      {
      xml.stag("Stem");
      Element::writeProperties(xml);
      if (_userLen != 0.0)
            xml.tag("userLen", _userLen / spatium());
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Stem::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "userLen")
                  _userLen = e.readDouble() * spatium();
            else if (tag == "subtype")        // obsolete
                  e.skipCurrentElement();
            else if (!Element::readProperties(e))
                  e.unknown();
            }
      }

//---------------------------------------------------------
//   updateGrips
//---------------------------------------------------------

void Stem::updateGrips(Grip* defaultGrip, QVector<QRectF>& grip) const
      {
      *defaultGrip = Grip::START;
      grip[0].translate(pagePos() + line.p2());
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

void Stem::startEdit(MuseScoreView*, const QPointF&)
      {
      undoPushProperty(P_ID::USER_LEN);
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void Stem::editDrag(const EditData& ed)
      {
      qreal yDelta = ed.delta.y();
      _userLen += up() ? -yDelta : yDelta;
      layout();
      Chord* c = static_cast<Chord*>(parent());
      if (c->hook())
            c->hook()->move(QPointF(0.0, ed.delta.y()));
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Stem::reset()
      {
      score()->undoChangeProperty(this, P_ID::USER_LEN, 0.0);
      Element::reset();
      }

//---------------------------------------------------------
//   acceptDrop
//---------------------------------------------------------

bool Stem::acceptDrop(const DropData& data) const
      {
      Element* e = data.element;
      if ((e->type() == Element::Type::TREMOLO) && (static_cast<Tremolo*>(e)->tremoloType() <= TremoloType::R64)) {
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   drop
//---------------------------------------------------------

Element* Stem::drop(const DropData& data)
      {
      Element* e = data.element;
      Chord* ch = chord();
      switch(e->type()) {
            case Element::Type::TREMOLO:
                  e->setParent(ch);
                  score()->setLayoutAll(true);
                  score()->undoAddElement(e);
                  return e;
            default:
                  delete e;
                  break;
            }
      return 0;
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant Stem::getProperty(P_ID propertyId) const
      {
      switch(propertyId) {
            case P_ID::USER_LEN: return userLen();
            default:
                  return Element::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool Stem::setProperty(P_ID propertyId, const QVariant& v)
      {
      score()->addRefresh(canvasBoundingRect());
      switch(propertyId) {
            case P_ID::USER_LEN:  setUserLen(v.toDouble()); break;
            default:
                  return Element::setProperty(propertyId, v);
            }
      score()->addRefresh(canvasBoundingRect());
      layout();
      score()->addRefresh(canvasBoundingRect());
      score()->setLayoutAll(false);       //DEBUG
      return true;
      }

//---------------------------------------------------------
//   hookPos
//    in chord coordinates
//---------------------------------------------------------

QPointF Stem::hookPos() const
      {
      QPointF p(pos() + line.p2());

      qreal xoff = lineWidth() * .5;
      p.rx() += xoff;
      return p;
      }

}

