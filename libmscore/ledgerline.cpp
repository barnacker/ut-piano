//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "ledgerline.h"
#include "chord.h"
#include "measure.h"
#include "staff.h"
#include "system.h"
#include "score.h"
#include "utils.h"


namespace Ms {

//---------------------------------------------------------
//   LedgerLine
//---------------------------------------------------------

LedgerLine::LedgerLine(Score* s)
   : Line(s, false)
      {
      setZ(int(Element::Type::STEM) * 100 - 50);
      setSelectable(false);
      _next = 0;
      }

//---------------------------------------------------------
//   pagePos
//---------------------------------------------------------

QPointF LedgerLine::pagePos() const
      {
      System* system = chord()->measure()->system();
      qreal yp = y() + system->staff(staffIdx())->y() + system->y();
      return QPointF(pageX(), yp);
      }

//---------------------------------------------------------
//   measureXPos
//---------------------------------------------------------

qreal LedgerLine::measureXPos() const
      {
      qreal xp = x();                   // chord relative
      xp += chord()->x();                // segment relative
      xp += chord()->segment()->x();     // measure relative
      return xp;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void LedgerLine::layout()
      {
      setLineWidth(score()->styleS(StyleIdx::ledgerLineWidth) * chord()->mag());
      if (staff())
            setColor(staff()->color());
      Line::layout();
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void LedgerLine::draw(QPainter* painter) const
      {
      if(chord()->crossMeasure() == CrossMeasure::SECOND)
            return;
#ifndef DISABLE_UTPIANO
      //Draw the UT-Piano extended staff
      if (staff() && staff()->isUtPianoStaff())
      {
        qreal sp = spatium();
        qreal vLineStart = sp * 0.35;
        qreal vLineEnd = sp * 0.65;
        qreal ledW = 2.0 * sp;
        qreal hLine1 = ledW * 0.08;
        qreal hLine2 = ledW * 0.92;
        float lw = score()->styleS(StyleIdx::staffLineWidth).val() * sp;
        int line = LineId();
        int pitch = line2pitch(line,ClefType::UTPIANO_RIGHT,Key::C);
        if (staff()->isUtPianoLeftStaff())
        {
            pitch = line2pitch(line,ClefType::UTPIANO_LEFT,Key::C);
            // Fix bug remove ledger lines for lefthand if cross right hand section
            if (pitch>60) return;
        }
        //Fix bug remove lefger lines for right hand if cross over left hand section
        if (staff()->isUtPianoRightStaff() && pitch<60)
        {
            return;
        }

        //qDebug("!!! Ledger line number %d pitch %d", line, pitch);

        bool isASharp = isPitchValueType(PitchValueType::ASharp, pitch);
        bool isCSharp = isPitchValueType(PitchValueType::CSharp, pitch);
        if ((isPitchValueType(PitchValueType::DSharp, pitch) && line!=22) || isASharp)
        {
            painter->save();
            // Gray box
            qreal z = 2.0*sp;
            painter->setBrush(QBrush(QColor(200, 200, 200)));
            painter->setPen(Qt::NoPen);
            painter->drawRect(QRect(-lw, -z, ledW+lw, sp));
            // Black line
            painter->setPen(QPen(QColor(0, 0, 0), lw, Qt::SolidLine, Qt::SquareCap));
            painter->drawLine(QLineF(0,  -z, ledW,  -z));
            // Drawing marks indicating C
            if (isASharp)
            {
                 painter->setPen(QPen(QColor(255, 255, 255), lw * 3, Qt::SolidLine, Qt::SquareCap));
                 painter->drawLine(QLineF(hLine1, -z + vLineStart, hLine1, -z + vLineEnd));
                 painter->drawLine(QLineF(hLine2, -z + vLineStart, hLine2, -z + vLineEnd));
            }
            painter->restore();
        }
        else if (isCSharp || isPitchValueType(PitchValueType::FSharp, pitch))
        {
            painter->save();
            // Gray box
            painter->setBrush(QBrush(QColor(200, 200, 200)));
            painter->setPen(Qt::NoPen);
            painter->drawRect(QRect(-lw, 0, ledW+lw, sp));
            // Drawing marks indicating C
            if (isCSharp)
            {
                painter->setPen(QPen(QColor(255, 255, 255), lw * 3, Qt::SolidLine, Qt::SquareCap));
                painter->drawLine(QLineF(hLine1, vLineStart, hLine1, vLineEnd));
                painter->drawLine(QLineF(hLine2, vLineStart, hLine2, vLineEnd));
            }
            painter->restore();
        }

         if (!isPitchValueType(PitchValueType::B, pitch) && !isPitchValueType(PitchValueType::E, pitch))
         {
           painter->save();
           painter->setPen(QPen(QColor(0, 0, 0), lw, Qt::SolidLine, Qt::SquareCap));
           painter->drawLine(QLineF(0,  0, ledW,  0));
           painter->restore();
           // Line::draw(painter);
         }
      }
      else
#endif
      {
            Line::draw(painter);
      }
    }

}
