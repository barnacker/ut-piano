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

#ifndef __LEDGERLINE_H__
#define __LEDGERLINE_H__

#include "element.h"

namespace Ms {

class Chord;

//---------------------------------------------------------
//    @@ LedgerLine
///     Graphic representation of a ledger line.
//!
//!    parent:     Chord
//!    x-origin:   Chord
//!    y-origin:   SStaff
//---------------------------------------------------------

class LedgerLine : public Line {
      Q_OBJECT

      LedgerLine* _next;
      int         _lineId;

   public:
      LedgerLine(Score*);
      LedgerLine &operator=(const LedgerLine&) = delete;
      virtual LedgerLine* clone() const override { return new LedgerLine(*this); }
      virtual Element::Type type() const override { return Element::Type::LEDGER_LINE; }
      virtual QPointF pagePos() const override;      ///< position in page coordinates
      Chord* chord() const { return (Chord*)parent(); }
      virtual void layout() override;
      virtual void draw(QPainter*) const override;
      qreal measureXPos() const;
      LedgerLine* next() const    { return _next; }
      void setNext(LedgerLine* l) { _next = l;    }
      void setLineId(int line)    { _lineId = line; }
      int LineId()  const         { return _lineId; }
      bool hasNext() const        { return (_next != 0); }
      };


}     // namespace Ms
#endif

