//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "figuredbass.h"
#include "score.h"
#include "note.h"
#include "measure.h"
#include "system.h"
#include "segment.h"
#include "chord.h"
#include "rest.h"
#include "score.h"
#include "sym.h"
#include "xml.h"

// trying to do without it
//#include <QQmlEngine>

namespace Ms {

static constexpr qreal  FB_CONTLINE_HEIGHT            = 0.875;    // the % of font EM to raise the cont. line at
                                                                  // (0 = top of font; 1 = bottom of font)
static constexpr qreal  FB_CONTLINE_LEFT_PADDING      = 0.1875;   // (3/16sp) the blank space at the left of a cont. line (in sp)
static constexpr qreal  FB_CONTLINE_OVERLAP           = 0.125;    // (1/8sp)  the overlap of an extended cont. line (in sp)
static constexpr qreal  FB_CONTLINE_THICKNESS         = 0.09375;  // (3/32sp) the thickness of a cont. line (in sp)

// the array of configured fonts
static QList<FiguredBassFont> g_FBFonts;

//---------------------------------------------------------
//   F I G U R E D   B A S S   I T E M
//---------------------------------------------------------

// used for indexed access to parenthesis chars
// (these is no normAccidToChar[], as accidentals may use mult. chars in normalized display):
const QChar FiguredBassItem::normParenthToChar[int(FiguredBassItem::Parenthesis::NUMOF)] =
{ 0, '(', ')', '[', ']'};


FiguredBassItem::FiguredBassItem(Score* s, int l)
      : Element(s), ord(l)
      {
      _prefix     = _suffix = Modifier::NONE;
      _digit      = FBIDigitNone;
      parenth[0]  = parenth[1] = parenth[2] = parenth[3] = parenth[4] = Parenthesis::NONE;
      _contLine   = ContLine::NONE;
      }

FiguredBassItem::FiguredBassItem(const FiguredBassItem& item)
      : Element(item)
      {
      ord         = item.ord;
      _prefix     = item._prefix;
      _digit      = item._digit;
      _suffix     = item._suffix;
      parenth[0]  = item.parenth[0];
      parenth[1]  = item.parenth[1];
      parenth[2]  = item.parenth[2];
      parenth[3]  = item.parenth[3];
      parenth[4]  = item.parenth[4];
      _contLine   = item._contLine;
      textWidth   = item.textWidth;
      _displayText= item._displayText;
      }

FiguredBassItem::~FiguredBassItem()
      {
      }

//---------------------------------------------------------
//   FiguredBassItem parse()
//
// converts a string into a property-based representation, if possible;
// return true on success | false if the string is non-conformant
//---------------------------------------------------------

bool FiguredBassItem::parse(QString& str)
      {
      int               retVal;

      parseParenthesis(str, 0);
      retVal = parsePrefixSuffix(str, true);          // prefix
      if(retVal == -1)
            return false;
      parseParenthesis(str, 1);
      retVal = parseDigit(str);                       // digit
      if(retVal == -1)
            return false;
      parseParenthesis(str, 2);
      retVal = parsePrefixSuffix(str, false);         // suffix
      if(retVal == -1)
            return false;
      parseParenthesis(str, 3);
      // check for a possible cont. line symbol(s)
      _contLine = ContLine::NONE;                       // contLine
      if(str[0] == '-' || str[0] == '_') {            // 1 symbol: simple continuation
            _contLine = ContLine::SIMPLE;
            str.remove(0, 1);
      }
      while(str[0] == '-' || str[0] == '_') {         // more than 1 symbol: extended continuation
            _contLine = ContLine::EXTENDED;
            str.remove(0, 1);
      }
      parseParenthesis(str, 4);

      // remove useless parentheses, moving external parentheses toward central digit element
      if(_prefix == Modifier::NONE && parenth[1] == Parenthesis::NONE) {
            parenth[1] = parenth[0];
            parenth[0] = Parenthesis::NONE;
            }
      if(_digit == FBIDigitNone && parenth[2] == Parenthesis::NONE) {
            parenth[2] = parenth[1];
            parenth[1] = Parenthesis::NONE;
            }
      if(_contLine == ContLine::NONE && parenth[3] == Parenthesis::NONE) {
            parenth[3] = parenth[4];
            parenth[4] = Parenthesis::NONE;
            }
      if(_suffix == Modifier::NONE && parenth[2] == Parenthesis::NONE) {
            parenth[2] = parenth[3];
            parenth[3] = Parenthesis::NONE;
            }

      // some checks:
      // if some extra input, str is not conformant
      if(str.size())
            return false;
      // can't have BOTH prefix and suffix
      // prefix, digit, suffix and cont.line cannot be ALL empty
      // suffix cannot combine with empty digit
      if( (_prefix != Modifier::NONE && _suffix != Modifier::NONE)
            || (_prefix == Modifier::NONE && _digit == FBIDigitNone && _suffix == Modifier::NONE && _contLine == ContLine::NONE)
            || ( (_suffix == Modifier::CROSS || _suffix == Modifier::BACKSLASH || _suffix == Modifier::SLASH)
                  && _digit == FBIDigitNone) )
            return false;
      return true;
}

//---------------------------------------------------------
//   FiguredBassItem parsePrefixSuffix()
//
//    scans str to extract prefix or suffix properties. Stops at the first char which cannot fit.
//    Fitting chars are removed from str. DOES NOT generate any display text
//
// returns the number of QChar's read from str or -1 if prefix / suffix has an illegal format
// (no prefix / suffix at all IS legal)
//---------------------------------------------------------

int FiguredBassItem::parsePrefixSuffix(QString& str, bool bPrefix)
      {
      Modifier *  dest  = bPrefix ? &_prefix : &_suffix;
      bool        done  = false;
      int         size  = str.size();
      str = str.trimmed();

      *dest       = Modifier::NONE;

      while(str.size()) {
            switch(str.at(0).unicode())
            {
            case 'b':
                  if(*dest != Modifier::NONE) {
                        if(*dest == Modifier::FLAT)     // FLAT may double a previous FLAT
                              *dest = Modifier::DOUBLEFLAT;
                        else
                              return -1;              // but no other combination is acceptable
                        }
                  else
                        *dest = Modifier::FLAT;
                  break;
            case 'h':
                  if(*dest != Modifier::NONE)           // cannot combine with any other accidental
                        return -1;
                  *dest = Modifier::NATURAL;
                  break;
            case '#':
                  if(*dest != Modifier::NONE) {
                        if(*dest == Modifier::SHARP)    // SHARP may double a preivous SHARP
                              *dest = Modifier::DOUBLESHARP;
                        else
                              return -1;              // but no other combination is acceptable
                        }
                  else
                        *dest = Modifier::SHARP;
                  break;
            case '+':
                  // accept '+' as both a prefix and a suffix for harmony notation
                  if(*dest != Modifier::NONE)           // cannot combine with any other accidental
                        return -1;
                  *dest = Modifier::CROSS;
                  break;
            // '\\' and '/' go into the suffix
            case '\\':
                  if(_suffix != Modifier::NONE)         // cannot combine with any other accidental
                        return -1;
                  _suffix = Modifier::BACKSLASH;
                  break;
            case '/':
                  if(_suffix != Modifier::NONE)         // cannot combine with any other accidental
                        return -1;
                  _suffix = Modifier::SLASH;
                  break;
            default:                                 // any other char: no longer in prefix/suffix
                  done = true;
                  break;
            }
            if(done)
                  break;
            str.remove(0,1);                         // 'eat' the char and continue
            }

      return size - str.size();                      // return how many chars we had read into prefix/suffix
      }

//---------------------------------------------------------
//   FiguredBassItem parseDigit()
//
//    scans str to extract digit properties. Stops at the first char which cannot belong to digit part.
//    Fitting chars are removed from str. DOES NOT generate any display text
//
// returns the number of QChar's read from str or -1 if no legal digit can be constructed
// (no digit at all IS legal)
//---------------------------------------------------------

int FiguredBassItem::parseDigit(QString& str)
      {
      int  size   = str.size();
      str         = str.trimmed();

      _digit = FBIDigitNone;

      while(str.size()) {
            // any digit acceptable
            if(str[0] >= '0' && str[0] <= '9') {
                  if (_digit == FBIDigitNone)
                        _digit = 0;
                  _digit = _digit*10 + (str[0].unicode() - '0');
                  str.remove(0, 1);
                  }
            // anything else: no longer in digit part
            else
                  break;
            }

      return size  - str.size();
      }

//---------------------------------------------------------
//   FiguredBassItem parseParenthesis()
//
//    scans str to extract a (possible) parenthesis, stores its code into parenth[parenthIdx]
//    and removes it from str. Only looks at first str char.
//
// returns the number of QChar's read from str (actually 0 or 1).
//---------------------------------------------------------

int FiguredBassItem::parseParenthesis(QString& str, int parenthIdx)
      {
      int c = str[0].unicode();
      Parenthesis code = Parenthesis::NONE;
      switch(c)
      {
      case '(':
            code = Parenthesis::ROUNDOPEN;
            break;
      case ')':
            code = Parenthesis::ROUNDCLOSED;
            break;
      case '[':
            code =Parenthesis::SQUAREDOPEN;
            break;
      case ']':
            code = Parenthesis::SQUAREDCLOSED;
            break;
      default:
            break;
            }
      parenth[parenthIdx] = code;
      if(code != Parenthesis::NONE) {
            str.remove(0, 1);
            return 1;
            }
      return 0;
      }

//---------------------------------------------------------
//   FiguredBassItem normalizedText()
//
// returns a string with the normalized text, i.e. the text displayed while editing;
// this is a standard textual representation of the item properties
//---------------------------------------------------------

QString FiguredBassItem::normalizedText() const
      {
      QString str = QString();
      if(parenth[0] != Parenthesis::NONE)
            str.append(normParenthToChar[int(parenth[0])]);

      if(_prefix != Modifier::NONE) {
            switch(_prefix)
            {
            case Modifier::FLAT:
                  str.append('b');
                  break;
            case Modifier::NATURAL:
                  str.append('h');
                  break;
            case Modifier::SHARP:
                  str.append('#');
                  break;
            case Modifier::CROSS:
                  str.append('+');
                  break;
            case Modifier::DOUBLEFLAT:
                  str.append("bb");
                  break;
            case Modifier::DOUBLESHARP:
                  str.append("##");
                  break;
            default:
                  break;
            }
            }

      if(parenth[1] != Parenthesis::NONE)
            str.append(normParenthToChar[int(parenth[1])]);

      // digit
      if(_digit != FBIDigitNone)
            str.append(QString::number(_digit));

      if(parenth[2] != Parenthesis::NONE)
            str.append(normParenthToChar[int(parenth[2])]);

      // suffix
      if(_suffix != Modifier::NONE) {
            switch(_suffix)
            {
            case Modifier::FLAT:
                  str.append('b');
                  break;
            case Modifier::NATURAL:
                  str.append('h');
                  break;
            case Modifier::SHARP:
                  str.append('#');
                  break;
            case Modifier::CROSS:
                  str.append('+');
                  break;
            case Modifier::BACKSLASH:
                  str.append('\\');
                  break;
            case Modifier::SLASH:
                  str.append('/');
                  break;
            case Modifier::DOUBLEFLAT:
                  str.append("bb");
                  break;
            case Modifier::DOUBLESHARP:
                  str.append("##");
                  break;
            default:
                  break;
            }
            }

      if(parenth[3] != Parenthesis::NONE)
            str.append(normParenthToChar[int(parenth[3])]);
      if(_contLine > ContLine::NONE) {
            str.append('_');
            if (_contLine > ContLine::SIMPLE)
                  str.append('_');
            }
      if(parenth[4] != Parenthesis::NONE)
            str.append(normParenthToChar[int(parenth[4])]);

      return str;
      }

//---------------------------------------------------------
//   FiguredBassItem write()
//---------------------------------------------------------

void FiguredBassItem::write(Xml& xml) const
{
      xml.stag("FiguredBassItem");
      xml.tagE(QString("brackets b0=\"%1\" b1=\"%2\" b2=\"%3\" b3=\"%4\" b4=\"%5\"")
                    .arg(int(parenth[0])) .arg(int(parenth[1])) .arg(int(parenth[2])) .arg(int(parenth[3])) .arg(int(parenth[4])) );
      if(_prefix != Modifier::NONE)
            xml.tag(QString("prefix"), int(_prefix));
      if(_digit != FBIDigitNone)
            xml.tag(QString("digit"), _digit);
      if(_suffix != Modifier::NONE)
            xml.tag(QString("suffix"), int(_suffix));
      if(_contLine != ContLine::NONE)
            xml.tag("continuationLine", int(_contLine));
      xml.etag();
}

//---------------------------------------------------------
//   FiguredBassItem read()
//---------------------------------------------------------

void FiguredBassItem::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());

            if (tag == "brackets") {
                  parenth[0] = (Parenthesis)e.intAttribute("b0");
                  parenth[1] = (Parenthesis)e.intAttribute("b1");
                  parenth[2] = (Parenthesis)e.intAttribute("b2");
                  parenth[3] = (Parenthesis)e.intAttribute("b3");
                  parenth[4] = (Parenthesis)e.intAttribute("b4");
                  e.readNext();
                  }
            else if (tag == "prefix")
                  _prefix = (Modifier)(e.readInt());
            else if (tag == "digit")
                  _digit = e.readInt();
            else if (tag == "suffix")
                  _suffix = (Modifier)(e.readInt());
            else if(tag == "continuationLine")
                  _contLine = (ContLine)(e.readInt());
            else if (!Element::readProperties(e))
                  e.unknown();
            }
      }

//---------------------------------------------------------
//   FiguredBassItem layout()
//    creates the display text (set as element text) and computes
//    the horiz. offset needed to align the right part as well as the vert. offset
//---------------------------------------------------------

void FiguredBassItem::layout()
      {
      qreal             h, w, x, x1, x2, y;

      // contruct font metrics
      int   fontIdx = 0;
      QFont f(g_FBFonts.at(fontIdx).family);

      // font size in pixels, scaled according to spatium()
      // (use the same font selection as used in draw() below)
      qreal m = score()->styleD(StyleIdx::figuredBassFontSize);
      m *= spatium() / SPATIUM20;     // make spatium dependent
      f.setPixelSize(lrint(m));
/* USING POINTS RATHER PIXELS MAKES FOR COARSER STEPS IN Qt ROUNDING TO INTEGER FONT SIZES
      // font size in points, scaled according to spatium()
      qreal m = score()->styleD(StyleIdx::figuredBassFontSize) * spatium() / SPATIUM20;
      f.setPointSizeF(m);
*/
      QFontMetrics      fm(f);

      QString           str = QString();
      x = symWidth(SymId::noteheadBlack) * .5;
      x1 = x2 = 0.0;

      // create display text
      int font = 0;
      int style = score()->styleI(StyleIdx::figuredBassStyle);

      if(parenth[0] != Parenthesis::NONE)
            str.append(g_FBFonts.at(font).displayParenthesis[int(parenth[0])]);

      // prefix
      if(_prefix != Modifier::NONE) {
            // if no digit, the string created so far 'hangs' to the left of the note
            if(_digit == FBIDigitNone)
                  x1 = fm.width(str);
            str.append(g_FBFonts.at(font).displayAccidental[int(_prefix)]);
            // if no digit, the string from here onward 'hangs' to the right of the note
            if(_digit == FBIDigitNone)
                  x2 = fm.width(str);
            }

      if(parenth[1] != Parenthesis::NONE)
            str.append(g_FBFonts.at(font).displayParenthesis[int(parenth[1])]);

      // digit
      if(_digit != FBIDigitNone) {
            // if some digit, the string created so far 'hangs' to the left of the note
            x1 = fm.width(str);
            // if suffix is a combining shape, combine it with digit (multi-digit numbers cannot be combined)
            // unless there is a parenthesis in between
            if( (_digit < 10)
                        && (_suffix == Modifier::CROSS || _suffix == Modifier::BACKSLASH || _suffix == Modifier::SLASH)
                        && parenth[2] == Parenthesis::NONE)
                  str.append(g_FBFonts.at(font).displayDigit[style][_digit][int(_suffix)-(int(Modifier::CROSS)-1)]);
            // if several digits or no shape combination, convert _digit to font styled chars
            else {
                  QString digits    = QString();
                  int digit         = _digit;
                  while (true) {
                        digits.prepend(g_FBFonts.at(font).displayDigit[style][(digit % 10)][0]);
                        digit /= 10;
                        if (digit == 0)
                              break;
                        }
                  str.append(digits);
                  }
            // if some digit, the string from here onward 'hangs' to the right of the note
            x2 = fm.width(str);
            }

      if(parenth[2] != Parenthesis::NONE)
            str.append(g_FBFonts.at(font).displayParenthesis[int(parenth[2])]);

      // suffix
      // append only if non-combining shape or cannot combine (no digit or parenthesis in between)
      if( _suffix != Modifier::NONE
                  && ( (_suffix != Modifier::CROSS && _suffix != Modifier::BACKSLASH && _suffix != Modifier::SLASH)
                        || _digit == FBIDigitNone
                        || parenth[2] != Parenthesis::NONE) )
            str.append(g_FBFonts.at(font).displayAccidental[int(_suffix)]);

      if(parenth[3] != Parenthesis::NONE)
            str.append(g_FBFonts.at(font).displayParenthesis[int(parenth[3])]);

      setDisplayText(str);                // this text will be displayed

      if (str.size())                     // if some text
            x = x - (x1+x2) * 0.5;        // position the text so that [x1<-->x2] is centered below the note
      else                                // if no text (but possibly a line)
            x = 0;                        // start at note left margin
      // vertical position
      h = fm.lineSpacing();
      h *= score()->styleD(StyleIdx::figuredBassLineHeight);
      if (score()->styleI(StyleIdx::figuredBassAlignment) == 0)          // top alignment: stack down from first item
            y = h * ord;
      else                                                        // bottom alignment: stack up from last item
            y = -h * (figuredBass()->numOfItems() - ord);
      setPos(x, y);
      // determine bbox from text width
//      w = fm.width(str);
      w = fm.boundingRect(str).width();
      textWidth = w;
      // if there is a cont.line, extend width to cover the whole FB element duration line
      int lineLen;
      if(_contLine != ContLine::NONE && (lineLen=figuredBass()->lineLength(0)) > w)
            w = lineLen;
      bbox().setRect(0, 0, w, h);
      }

//---------------------------------------------------------
//   FiguredBassItem draw()
//---------------------------------------------------------

void FiguredBassItem::draw(QPainter* painter) const
      {
      int   font = 0;
      qreal _spatium = spatium();
      // set font from general style
      QFont f(g_FBFonts.at(font).family);
#ifdef USE_GLYPHS
      f.setHintingPreference(QFont::PreferVerticalHinting);
#endif
      // font size in pixels, scaled according to spatium()
      // (use the same font selection as used in layout() above)
      qreal m = score()->styleD(StyleIdx::figuredBassFontSize);
      m *= spatium() / SPATIUM20;     // make spatium dependent
      f.setPixelSize(lrint(m));
/* USING POINTS RATHER PIXELS MAKES FOR COARSER STEPS IN Qt ROUNDING TO INTEGER FONT SIZES
      // font size in points, scaled according to spatium()
      qreal m = score()->styleD(StyleIdx::figuredBassFontSize) * spatium() / SPATIUM20;
      f.setPointSizeF(m);
*/
      painter->setFont(f);
      painter->setBrush(Qt::NoBrush);
      QPen pen(figuredBass()->curColor(), FB_CONTLINE_THICKNESS * _spatium, Qt::SolidLine, Qt::RoundCap);
      painter->setPen(pen);
      painter->drawText(bbox(), Qt::TextDontClip | Qt::AlignLeft | Qt::AlignTop, displayText());

      // continuation line
      qreal lineEndX = 0.0;
      if (_contLine > ContLine::NONE) {
            qreal lineStartX   = textWidth;                       // by default, line starts right after text
            if (lineStartX > 0.0)
                  lineStartX += _spatium * FB_CONTLINE_LEFT_PADDING;    // if some text, give some room after it
            lineEndX = figuredBass()->printedLineLength();        // by default, line ends with item duration
            if(lineEndX - lineStartX < 1.0)                       // if line length < 1 sp, ignore it
                  lineEndX = 0.0;

            // if extended cont.line and no closing parenthesis: look at next FB element
            if (_contLine > ContLine::SIMPLE && parenth[4] == Parenthesis::NONE) {
                  FiguredBass * nextFB;
                  // if there is a contiguous FB element
                  if ( (nextFB=figuredBass()->nextFiguredBass()) != 0) {
                        // retrieve the X position (in page coords) of a possible cont. line of nextFB
                        // on the same line of 'this'
                        QPointF pgPos = pagePos();
                        qreal nextContPageX = nextFB->additionalContLineX(pgPos.y());
                        // if an additional cont. line has been found, extend up to its initial X coord
                        if (nextContPageX > 0)
                              lineEndX = nextContPageX - pgPos.x() + _spatium*FB_CONTLINE_OVERLAP;
                                                                              // with a little bit of overlap
                        else
                              lineEndX = figuredBass()->lineLength(0);        // if none found, draw to the duration end
                        }
                  }
            // if some line, draw it
            if (lineEndX > 0.0) {
                  qreal h = bbox().height() * FB_CONTLINE_HEIGHT;
                  painter->drawLine(lineStartX, h, lineEndX - ipos().x(), h);
                  }
            }

      // closing cont.line parenthesis
      if(parenth[4] != Parenthesis::NONE) {
            int x = lineEndX > 0.0 ? lineEndX : textWidth;
            painter->drawText(QRectF(x, 0, bbox().width(), bbox().height()), Qt::AlignLeft | Qt::AlignTop,
                  g_FBFonts.at(font).displayParenthesis[int(parenth[4])]);
            }
      }

//---------------------------------------------------------
//   PROPERTY METHODS
//---------------------------------------------------------

QVariant FiguredBassItem::getProperty(P_ID propertyId) const
      {
      switch(propertyId) {
            case P_ID::FBPREFIX:
                  return int(_prefix);
            case P_ID::FBDIGIT:
                  return _digit;
            case P_ID::FBSUFFIX:
                  return int(_suffix);
            case P_ID::FBCONTINUATIONLINE:
                  return int(_contLine);
            case P_ID::FBPARENTHESIS1:
                  return int(parenth[0]);
            case P_ID::FBPARENTHESIS2:
                  return int(parenth[1]);
            case P_ID::FBPARENTHESIS3:
                  return int(parenth[2]);
            case P_ID::FBPARENTHESIS4:
                  return int(parenth[3]);
            case P_ID::FBPARENTHESIS5:
                  return int(parenth[4]);
            default:
                  return Element::getProperty(propertyId);
            }
      }

bool FiguredBassItem::setProperty(P_ID propertyId, const QVariant& v)
      {
      score()->addRefresh(canvasBoundingRect());
      int   val = v.toInt();
      switch(propertyId) {
            case P_ID::FBPREFIX:
                  if(val < int(Modifier::NONE) || val >= int(Modifier::NUMOF))
                        return false;
                  _prefix = (Modifier)val;
                  break;
            case P_ID::FBDIGIT:
                  if(val < 1 || val > 9)
                        return false;
                  _digit = val;
                  break;
            case P_ID::FBSUFFIX:
                  if(val < int(Modifier::NONE) || val >= int(Modifier::NUMOF))
                        return false;
                  _suffix = (Modifier)val;
                  break;
            case P_ID::FBCONTINUATIONLINE:
                  _contLine = (ContLine)val;
                  break;
            case P_ID::FBPARENTHESIS1:
                  if(val < int(Parenthesis::NONE) || val >= int(Parenthesis::NUMOF))
                        return false;
                  parenth[0] = (Parenthesis)val;
                  break;
            case P_ID::FBPARENTHESIS2:
                  if(val < int(Parenthesis::NONE) || val >= int(Parenthesis::NUMOF))
                        return false;
                  parenth[1] = (Parenthesis)val;
                  break;
            case P_ID::FBPARENTHESIS3:
                  if(val < int(Parenthesis::NONE) || val >= int(Parenthesis::NUMOF))
                        return false;
                  parenth[2] = (Parenthesis)val;
                  break;
            case P_ID::FBPARENTHESIS4:
                  if(val < int(Parenthesis::NONE) || val >= int(Parenthesis::NUMOF))
                        return false;
                  parenth[3] = (Parenthesis)val;
                  break;
            case P_ID::FBPARENTHESIS5:
                  if(val < int(Parenthesis::NONE) || val >= int(Parenthesis::NUMOF))
                        return false;
                  parenth[4] = (Parenthesis)val;
                  break;
            default:
                  return Element::setProperty(propertyId, v);
            }
      score()->setLayoutAll(true);
      return true;
      }

QVariant FiguredBassItem::propertyDefault(P_ID id) const
      {
      switch(id) {
            case P_ID::FBPREFIX:
            case P_ID::FBSUFFIX:
                  return int(Modifier::NONE);
            case P_ID::FBDIGIT:
                  return FBIDigitNone;
            case P_ID::FBCONTINUATIONLINE:
                  return false;
            default:
                  return Element::propertyDefault(id);
            }
      }

//---------------------------------------------------------
//   UNDOABLE PROPERTY SETTERS
//---------------------------------------------------------

void FiguredBassItem::undoSetPrefix(Modifier pref)
      {
      if(pref <= Modifier::CROSS) {
            score()->undoChangeProperty(this, P_ID::FBPREFIX, (int)pref);
            // if setting some prefix and there is a suffix already, clear suffix
            if(pref != Modifier::NONE && _suffix != Modifier::NONE)
                  score()->undoChangeProperty(this, P_ID::FBSUFFIX, int(Modifier::NONE));
            layout();                     // re-generate displayText
            }
      }

void FiguredBassItem::undoSetDigit(int digit)
      {
      if(digit >= 0 && digit <= 9) {
            score()->undoChangeProperty(this, P_ID::FBDIGIT, digit);
            layout();                     // re-generate displayText
            }
      }

void FiguredBassItem::undoSetSuffix(Modifier suff)
      {
      score()->undoChangeProperty(this, P_ID::FBSUFFIX, int(suff));
      // if setting some suffix and there is a prefix already, clear prefix
      if(suff != Modifier::NONE && _prefix != Modifier::NONE)
            score()->undoChangeProperty(this, P_ID::FBPREFIX, int(Modifier::NONE));
      layout();                     // re-generate displayText
      }

void FiguredBassItem::undoSetContLine(ContLine val)
      {
      score()->undoChangeProperty(this, P_ID::FBCONTINUATIONLINE, int(val));
      layout();                     // re-generate displayText
      }

void FiguredBassItem::undoSetParenth1(Parenthesis par)
      {
      score()->undoChangeProperty(this, P_ID::FBPARENTHESIS1, int(par));
      layout();                     // re-generate displayText
      }
void FiguredBassItem::undoSetParenth2(Parenthesis par)
      {
      score()->undoChangeProperty(this, P_ID::FBPARENTHESIS2, int(par));
      layout();                     // re-generate displayText
      }
void FiguredBassItem::undoSetParenth3(Parenthesis par)
      {
      score()->undoChangeProperty(this, P_ID::FBPARENTHESIS3, int(par));
      layout();                     // re-generate displayText
      }
void FiguredBassItem::undoSetParenth4(Parenthesis par)
      {
      score()->undoChangeProperty(this, P_ID::FBPARENTHESIS4, int(par));
      layout();                     // re-generate displayText
      }
void FiguredBassItem::undoSetParenth5(Parenthesis par)
      {
      score()->undoChangeProperty(this, P_ID::FBPARENTHESIS5, int(par));
      layout();                     // re-generate displayText
      }

//---------------------------------------------------------
//
//    MusicXML I/O
//
//---------------------------------------------------------

//---------------------------------------------------------
//   Convert MusicXML prefix/suffix to Modifier
//---------------------------------------------------------

FiguredBassItem::Modifier FiguredBassItem::MusicXML2Modifier(const QString prefix) const
      {
      if (prefix == "sharp")
            return Modifier::SHARP;
      else if (prefix == "flat")
            return Modifier::FLAT;
      else if (prefix == "natural")
            return Modifier::NATURAL;
      else if (prefix == "double-sharp")
            return Modifier::DOUBLESHARP;
      else if (prefix == "flat-flat")
            return Modifier::DOUBLEFLAT;
      else if (prefix == "sharp-sharp")
            return Modifier::DOUBLESHARP;
      else if (prefix == "cross")
            return Modifier::CROSS;
      else if (prefix == "backslash")
            return Modifier::BACKSLASH;
      else if (prefix == "slash")
            return Modifier::SLASH;
      else
            return Modifier::NONE;
      }

//---------------------------------------------------------
//   Convert Modifier to MusicXML prefix/suffix
//---------------------------------------------------------

QString FiguredBassItem::Modifier2MusicXML(FiguredBassItem::Modifier prefix) const
      {
      switch (prefix) {
            case Modifier::NONE:        return "";
            case Modifier::DOUBLEFLAT:  return "flat-flat";
            case Modifier::FLAT:        return "flat";
            case Modifier::NATURAL:     return "natural";
            case Modifier::SHARP:       return "sharp";
            case Modifier::DOUBLESHARP: return "double-sharp";
            case Modifier::CROSS:       return "cross";
            case Modifier::BACKSLASH:   return "backslash";
            case Modifier::SLASH:       return "slash";
            case Modifier::NUMOF:       return ""; // prevent gcc "???FBINumOfAccid??? not handled in switch" warning
            }
      return "";
      }

//---------------------------------------------------------
//   Read MusicXML
//
// Set the FiguredBassItem state based on the MusicXML <figure> node de.
// In MusicXML, parentheses is set to "yes" or "no" for the figured-bass
// node instead of for each individual <figure> node.
//---------------------------------------------------------

#if 0
void FiguredBassItem::readMusicXML(XmlReader& e, bool paren)
      {
      // read the <figure> node de
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "figure-number") {
                  // MusicXML spec states figure-number is a number
                  // MuseScore can only handle single digit
                  int iVal = e.readInt();
                  if (1 <= iVal && iVal <= 9)
                        _digit = iVal;
                  }
            else if (tag == "prefix")
                  _prefix = MusicXML2Modifier(e.readElementText());
            else if (tag == "suffix")
                  _suffix = MusicXML2Modifier(e.readElementText());
            else
                  e.unknown();
            }
      // set parentheses
      if (paren) {
            // parenthesis open
            if (_prefix != Modifier::NONE)
                  parenth[0] = Parenthesis::ROUNDOPEN; // before prefix
            else if (_digit != FBIDigitNone)
                  parenth[1] = Parenthesis::ROUNDOPEN; // before digit
            else if (_suffix != Modifier::NONE)
                  parenth[2] = Parenthesis::ROUNDOPEN; // before suffix
            // parenthesis close
            if (_suffix != Modifier::NONE)
                  parenth[3] = Parenthesis::ROUNDCLOSED; // after suffix
            else if (_digit != FBIDigitNone)
                  parenth[2] = Parenthesis::ROUNDCLOSED; // after digit
            else if (_prefix != Modifier::NONE)
                  parenth[1] = Parenthesis::ROUNDCLOSED; // after prefix
            }
      }
#endif

//---------------------------------------------------------
//   Write MusicXML
//
// Writes the portion within the <figure> tag.
//
// NOTE: Both MuseScore and MusicXML provide two ways of altering the (temporal) length of a
// figured bass object: extension lines and duration. The convention is that an EXTENSION is
// used if the figure lasts LONGER than the note (i.e., it "extends" to the following notes),
// whereas DURATION is used if the figure lasts SHORTER than the note (e.g., when notating a
// figure change under a note). However, MuseScore does not restrict durations in this way,
// allowing them to act as extensions themselves. As a result, a few more branches are
// required in the decision tree to handle everything correctly.
//---------------------------------------------------------

void FiguredBassItem::writeMusicXML(Xml& xml, bool isOriginalFigure, int crEndTick, int fbEndTick) const
      {
      xml.stag("figure");

      // The first figure of each group is the "original" figure. Practically, it is one inserted manually
      // by the user, rather than automatically by the "duration" extend method.
      if (isOriginalFigure) {
            QString strPrefix = Modifier2MusicXML(_prefix);
            if (strPrefix != "")
                  xml.tag("prefix", strPrefix);
            if (_digit != FBIDigitNone)
                  xml.tag("figure-number", _digit);
            QString strSuffix = Modifier2MusicXML(_suffix);
            if (strSuffix != "")
                  xml.tag("suffix", strSuffix);

            // Check if the figure ends before or at the same time as the current note. Otherwise, the figure
            // extends to the next note, and so carries an extension type "start" by definition.
            if (fbEndTick <= crEndTick) {
                  if (_contLine == ContLine::SIMPLE)
                        xml.tagE("extend type=\"stop\" ");
                  else if (_contLine == ContLine::EXTENDED) {
                        bool hasFigure = (strPrefix != "" || _digit != FBIDigitNone || strSuffix != "");
                        if (hasFigure)
                              xml.tagE("extend type=\"start\" ");
                        else
                              xml.tagE("extend type=\"continue\" ");
                        }
                  }
            else
                  xml.tagE("extend type=\"start\" ");
            }
      // If the figure is not "original", it must have been created using the "duration" feature of figured bass.
      // In other words, the original figure belongs to a previous note rather than the current note.
      else {
            if (crEndTick < fbEndTick)
                  xml.tagE("extend type=\"continue\" ");
            else
                  xml.tagE("extend type=\"stop\" ");
            }
      xml.etag();
      }

//---------------------------------------------------------
//   startsWithParenthesis
//---------------------------------------------------------

bool FiguredBassItem::startsWithParenthesis() const
      {
      if (_prefix != Modifier::NONE)
            return (parenth[0] != Parenthesis::NONE);
      if (_digit != FBIDigitNone)
            return (parenth[1] != Parenthesis::NONE);
      if (_suffix != Modifier::NONE)
            return (parenth[2] != Parenthesis::NONE);
      return false;
      }

//---------------------------------------------------------
//   F I G U R E D   B A S S
//---------------------------------------------------------

FiguredBass::FiguredBass(Score* s)
   : Text(s)
      {
      setFlag(ElementFlag::ON_STAFF, true);
      setOnNote(true);
      setTextStyleType(TextStyleType::FIGURED_BASS);
      TextStyle st("Figured Bass", g_FBFonts[0].family, score()->styleD(StyleIdx::figuredBassFontSize),
                  false, false, false, AlignmentFlags::LEFT | AlignmentFlags::TOP, QPointF(0, score()->styleD(StyleIdx::figuredBassYOffset)), OffsetType::SPATIUM);
      st.setSizeIsSpatiumDependent(true);
      setTextStyle(st);
      setTicks(0);
      items.clear();
      }

FiguredBass::FiguredBass(const FiguredBass& fb)
   : Text(fb)
      {
      setOnNote(fb.onNote());
      setTicks(fb.ticks());
      for (auto i : fb.items) {     // deep copy is needed
            FiguredBassItem* fbi = new FiguredBassItem(*i);
            fbi->setParent(this);
            items.push_back(fbi);
            }
//      items = fb.items;
      }

FiguredBass::~FiguredBass()
      {
      for (FiguredBassItem* item : items)
            delete item;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void FiguredBass::write(Xml& xml) const
      {
      if (!xml.canWrite(this))
            return;
      xml.stag("FiguredBass");
      if(!onNote())
            xml.tag("onNote", onNote());
      if (ticks() > 0)
            xml.tag("ticks", ticks());
      // if unparseable items, write full text data
      if (items.size() < 1)
            Text::writeProperties(xml, true);
      else {
            if (textStyleType() != TextStyleType::FIGURED_BASS)
                  // if all items parsed and not unstiled, we simply have a special style: write it
                  xml.tag("style", textStyle().name());
            for(FiguredBassItem* item : items)
                  item->write(xml);
            Element::writeProperties(xml);
            }
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void FiguredBass::read(XmlReader& e)
      {
      QString normalizedText;
      int idx = 0;
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "ticks")
                  setTicks(e.readInt());
            else if (tag == "onNote")
                  setOnNote(e.readInt() != 0l);
            else if (tag == "FiguredBassItem") {
                  FiguredBassItem * pItem = new FiguredBassItem(score(), idx++);
                  pItem->setTrack(track());
                  pItem->setParent(this);
                  pItem->read(e);
                  items.push_back(pItem);
                  // add item normalized text
                  if(!normalizedText.isEmpty())
                        normalizedText.append('\n');
                  normalizedText.append(pItem->normalizedText());
                  }
//            else if (tag == "style")
//                  setTextStyleType(e.readElementText());
            else if (!Text::readProperties(e))
                  e.unknown();
            }
      // if items could be parsed set normalized text
      if (items.size() > 0)
            setXmlText(normalizedText);      // this is the text to show while editing
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void FiguredBass::layout()
      {
      qreal yOff  = score()->styleD(StyleIdx::figuredBassYOffset);
      qreal _sp   = spatium();
      // if 'our' style, force 'our' style data from FiguredBass parameters
      if(textStyleType() == TextStyleType::FIGURED_BASS) {
            TextStyle st("Figured Bass", g_FBFonts[0].family, score()->styleD(StyleIdx::figuredBassFontSize),
                        false, false, false, AlignmentFlags::LEFT | AlignmentFlags::TOP, QPointF(0, yOff),
                        OffsetType::SPATIUM);
            st.setSizeIsSpatiumDependent(true);
            setTextStyle(st);
            }

      // if in edit mode or if style has been changed,
      // do nothing else, keeping default laying out and formatting
      if(editMode() || items.size() < 1 || textStyleType() != TextStyleType::FIGURED_BASS) {
            Text::layout();
            return;
      }

      // VERTICAL POSITION:
      yOff *= _sp;                                    // convert spatium value to raster units
      setPos(QPointF(0.0, yOff));

      // BOUNDING BOX and individual item layout (if requried)
      createLayout();                                 // prepare structs and data expected by Text methods
      // if element could be parsed into items, layout each element
      if(items.size() > 0) {
            layoutLines();
            bbox().setRect(0, 0, _lineLenghts.at(0), 0);
            // layout each item and enlarge bbox to include items bboxes
            for(FiguredBassItem* item : items) {
                  item->layout();
                  addbbox(item->bbox().translated(item->pos()));
                  }
            }
      adjustReadPos();
      }

//---------------------------------------------------------
//   layoutLines
//
//    lays out the duration indicator line(s), filling the _lineLengths array
//    and the length of printed lines (used by continuation lines)
//---------------------------------------------------------

void FiguredBass::layoutLines()
      {
      if(_ticks <= 0 || segment() == nullptr) {
NoLen:
            _lineLenghts.resize(1);                         // be sure to always have
            _lineLenghts[0] = 0;                            // at least 1 item in array
            return;
            }

      ChordRest * lastCR;                                   // the last ChordRest of this
      Segment *   nextSegm;                                 // the Segment beyond this' segment
      int         nextTick = segment()->tick() + _ticks;    // the tick beyond this' duration

      // locate the measure containing the last tick of this; it is either:
      // the same measure containing nextTick, if nextTick is not the first tick of a measure
      //    (and line should stop right before it)
      // or the previous measure, if nextTick is the first tick of a measure
      //    (and line should stop before any measure terminal segment (bar, clef, ...) )
      Measure* m = score()->tick2measure(nextTick-1);
      if (m != 0) {
            // locate the first segment (of ANY type) right after this' last tick
            for (nextSegm = m->first(Segment::Type::All); nextSegm; ) {
                  if(nextSegm->tick() >= nextTick)
                        break;
                  nextSegm = nextSegm->next();
                  }
            // locate the last ChordRest of this
            if (nextSegm)
                  lastCR = nextSegm->prev1()->nextChordRest(track(), true);
            }
      if (m == 0 || nextSegm == 0) {
            qDebug("FiguredBass layout: no segment found for tick %d", nextTick);
            goto NoLen;
            }

      // get length of printed lines from horiz. page position of lastCR
      // (enter a bit 'into' the ChordRest for clarity)
      _printedLineLength = lastCR ? lastCR->pageX() - pageX() + 1.5*spatium() : 3 * spatium();

      // get duration indicator line(s) from page position of nextSegm
      QList<System*>* systems = score()->systems();
      System* s1  = segment()->measure()->system();
      System* s2  = nextSegm->measure()->system();
      int sysIdx1 = systems->indexOf(s1);
      int sysIdx2 = systems->indexOf(s2);

      int i, len ,segIdx;
      for (i = sysIdx1, segIdx = 0; i <= sysIdx2; ++i, ++segIdx) {
            len = 0;
            if (sysIdx1 == sysIdx2 || i == sysIdx1) {
                  // single line
                  len = nextSegm->pageX() - pageX() - 4;         // stop 4 raster units before next segm
                  }
            else if (i == sysIdx1) {
                  // initial line
                  qreal w   = s1->staff(staffIdx())->right();
                  qreal x   = s1->pageX() + w;
                  len = x - pageX();
                  }
            else if (i > 0 && i != sysIdx2) {
                  // middle line
qDebug("FiguredBass: duration indicator middle line not implemented");
                  }
            else if (i == sysIdx2) {
                  // end line
qDebug("FiguredBass: duration indicator end line not implemented");
                  }
            // store length item, reusing array items if already present
            if (_lineLenghts.size() <= segIdx)
                  _lineLenghts.append(len);
            else
                  _lineLenghts[segIdx] = len;
            }
      // if more array items than needed, truncate array
      if(_lineLenghts.size() > segIdx)
            _lineLenghts.resize(segIdx);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void FiguredBass::draw(QPainter* painter) const
      {
      // if not printing, draw duration line(s)
      if (!score()->printing() && score()->showUnprintable()) {
            foreach(qreal len, _lineLenghts)
                  if(len > 0) {
                        painter->setPen(QPen(Qt::lightGray, 1));
                        painter->drawLine(0.0, -2, len, -2);      // -2: 2 rast. un. above digits
                        }
            }
      // if in edit mode or with custom style, use standard text drawing
      if (editMode() || textStyleType() != TextStyleType::FIGURED_BASS)
            Text::draw(painter);
      // not edit mode:
      else {
            if (items.size() < 1)                            // if not parseable into f.b. items
                  Text::draw(painter);                      // draw as standard text
            else
                  for (FiguredBassItem* item : items) {      // if parseable into f.b. items
                        painter->translate(item->pos());    // draw each item in its proper position
                        item->draw(painter);
                        painter->translate(-item->pos());
                        }
            }
/* DEBUG
      QString str = QString();
      str.setNum(_ticks);
      painter->drawText(0, (_onNote ? 40 : 30), str);
*/
      }

//---------------------------------------------------------
//   startEdit / edit / endEdit
//---------------------------------------------------------

void FiguredBass::startEdit(MuseScoreView * msv, const QPointF & pt)
      {
      Text::layout();               // convert layout to standard Text conventions
      Text::startEdit(msv, pt);
      }

void FiguredBass::endEdit()
      {
      int         idx;

      Text::endEdit();
      // as the standard text editor keeps inserting spurious HTML formatting and styles
      // retrieve and work only on the plain text
      QString txt = plainText();
      if (txt.isEmpty()) {                       // if no text, nothing to do
            setXmlText(txt);                       // clear the stored text: the empty f.b. element will be deleted
            return;
            }

      // split text into lines and create an item for each line
      QStringList list = txt.split('\n', QString::SkipEmptyParts);
      items.clear();
      QString normalizedText = QString();
      idx = 0;
      foreach(QString str, list) {
            FiguredBassItem* pItem = new FiguredBassItem(score(), idx++);
            if(!pItem->parse(str)) {            // if any item fails parsing
                  items.clear();                // clear item list
                  Text::layout();               // keeping text as entered by user
                  return;
                  }
            pItem->setTrack(track());
            pItem->setParent(this);
            items.push_back(pItem);

            // add item normalized text
            if(!normalizedText.isEmpty())
                  normalizedText.append('\n');
            normalizedText.append(pItem->normalizedText());
            }
      // if all items parsed and text is styled, replaced entered text with normalized text
      if (items.size()) {
            setXmlText(normalizedText);
            layout();
            }
      }

//---------------------------------------------------------
//   setSelected /setVisible
//
//    forward flags to items
//---------------------------------------------------------

void FiguredBass::setSelected(bool flag)
      {
      Element::setSelected(flag);
      for (FiguredBassItem* item : items) {
            item->setSelected(flag);
            }
      }

void FiguredBass::setVisible(bool flag)
      {
      Element::setVisible(flag);
      for(FiguredBassItem* item : items) {
            item->setVisible(flag);
            }
      }

//---------------------------------------------------------
//   nextFiguredBass
//
//    returns the next *contiguous* FiguredBass element if it exists,
//    i.e. the FiguredBass element which starts where 'this' ends
//    returns 0 if none
//---------------------------------------------------------

FiguredBass* FiguredBass::nextFiguredBass() const
      {
      if (_ticks <= 0)                                      // if _ticks unset, no clear idea of when 'this' ends
            return 0;
      Segment *   nextSegm;                                 // the Segment beyond this' segment
      int         nextTick = segment()->tick() + _ticks;    // the tick beyond this' duration

      // locate the ChordRest segment right after this' end
      nextSegm = score()->tick2segment(nextTick, true, Segment::Type::ChordRest);
      if (nextSegm == 0)
            return 0;

      // scan segment annotations for an existing FB element in the this' staff
      for (Element* e : nextSegm->annotations())
            if (e->type() == Element::Type::FIGURED_BASS && e->track() == track())
                  return static_cast<FiguredBass*>(e);

      return 0;
      }

//---------------------------------------------------------
//   additionalContLineX
//
//    if there is a continuation line, without other text elements, at pagePosY, returns its X coord (in page coords)
//    returns 0 if no cont.line there or if there are text elements before the cont.line
//
//    In practice, returns the X coord of a cont. line which can be the continuation of a previous cont. line
//
//    Note: pagePosY is the Y coord of the FiguredBassItem containing the line, not of the line itself,
//    as line position might depend on styles.
//---------------------------------------------------------

qreal FiguredBass::additionalContLineX(qreal pagePosY) const
{
      QPointF pgPos = pagePos();
      for (FiguredBassItem* fbi : items)
            // if item has cont.line but nothing before it
            // and item Y coord near enough to pagePosY
            if(fbi->contLine() != FiguredBassItem::ContLine::NONE
                  && fbi->digit() == FBIDigitNone
                     && fbi->prefix() == FiguredBassItem::Modifier::NONE
                        && fbi->suffix() == FiguredBassItem::Modifier::NONE
                           && fbi->parenth4() == FiguredBassItem::Parenthesis::NONE
                              && qAbs(pgPos.y() + fbi->ipos().y() - pagePosY) < 0.05)
                  return pgPos.x() + fbi->ipos().x();

      return 0.0;                               // no suitable line
}

//---------------------------------------------------------
//   PROPERTY METHODS
//---------------------------------------------------------

QVariant FiguredBass::getProperty(P_ID propertyId) const
      {
      switch(propertyId) {
            default:
                  return Text::getProperty(propertyId);
            }
      }

bool FiguredBass::setProperty(P_ID propertyId, const QVariant& v)
      {
      score()->addRefresh(canvasBoundingRect());
      switch(propertyId) {
            default:
                  return Text::setProperty(propertyId, v);
            }
      score()->setLayoutAll(true);
      return true;
      }

QVariant FiguredBass::propertyDefault(P_ID id) const
      {
      switch(id) {
            default:
                  return Text::propertyDefault(id);
            }
      }

//---------------------------------------------------------
//   TEMPORARY HACK!!!
//---------------------------------------------------------
/*
FiguredBassItem * FiguredBass::addItem()
      {
      int line = items.size();
      FiguredBassItem* fib = new FiguredBassItem(score(), line);
      // tell QML not to garbage collect this item
      QQmlEngine::setObjectOwnership(fib, QQmlEngine::CppOwnership);
      items.push_back(fib);
      return fib;
      }
*/
//---------------------------------------------------------
//   STATIC FUNCTION
//    adding a new FiguredBass to a Segment;
//    the main purpose of this function is to ensure that ONLY ONE F.b. element exists for each Segment/staff;
//    it either re-uses an existing FiguredBass or creates a new one if none is found;
//    returns the FiguredBass and sets pNew to true if it has been newly created.
//
//    Sets an initial duration of the element up to the next ChordRest of the same staff.
//
//    As the F.b. very concept requires the base chord to have ONLY ONE note,
//    FiguredBass elements are created and looked for only in the first track of the staff.
//---------------------------------------------------------

FiguredBass * FiguredBass::addFiguredBassToSegment(Segment * seg, int track, int extTicks, bool * pNew)
      {
      int         endTick;                      // where this FB is initially assumed to end
      int         staff = track / VOICES;       // convert track to staff
      track = staff * VOICES;                   // first track for this staff

      // scan segment annotations for an existing FB element in the same staff
      FiguredBass* fb = 0;
      for (Element* e : seg->annotations()) {
            if (e->type() == Element::Type::FIGURED_BASS && (e->track() / VOICES) == staff) {
                  // an FB already exists in segment: re-use it
                  fb = static_cast<FiguredBass*>(e);
                  *pNew = false;
                  endTick = seg->tick() + fb->ticks();
                  break;
                  }
            }
      if (fb == 0) {                          // no FB at segment: create new
            fb = new FiguredBass(seg->score());
            fb->setTrack(track);
            fb->setParent(seg);

            // locate next SegChordRest in the same staff to estimate presumed duration of element
            endTick = INT_MAX;
            Segment *   nextSegm;
            for (int iVoice = 0; iVoice < VOICES; iVoice++) {
                  nextSegm = seg->nextCR(track + iVoice);
                  if(nextSegm && nextSegm->tick() < endTick)
                        endTick = nextSegm->tick();
                  }
            if(endTick == INT_MAX) {            // no next segment: set up to score end
                  Measure * meas = seg->score()->lastMeasure();
                  endTick = meas->tick() + meas->ticks();
                  }
            fb->setTicks(endTick - seg->tick());

            // set onNote status
            fb->setOnNote(false);               // assume not onNote
            for (int i = track; i < track + VOICES; i++)         // if segment has chord in staff, set onNote
                  if (seg->element(i) && seg->element(i)->type() == Element::Type::CHORD) {
                        fb->setOnNote(true);
                        break;
                  }
            *pNew = true;
            }

      // if we are extending a previous FB
      if(extTicks > 0) {
            // locate previous FB for same staff
            Segment *         prevSegm;
            FiguredBass*      prevFB = 0;
            for(prevSegm = seg->prev1(Segment::Type::ChordRest); prevSegm; prevSegm = prevSegm->prev1(Segment::Type::ChordRest)) {
                  for (Element* e : prevSegm->annotations()) {
                        if (e->type() == Element::Type::FIGURED_BASS && (e->track() ) == track) {
                              prevFB = static_cast<FiguredBass*>(e);   // previous FB found
                              break;
                              }
                        }
                  if(prevFB) {
                        // if previous FB did not stop more than extTicks before this FB...
                        int delta = seg->tick() - prevFB->segment()->tick();
                        if(prevFB->ticks() + extTicks >= delta)
                              prevFB->setTicks(delta);      // update prev FB ticks to last up to this FB
                        break;
                        }
                  }
            }
      return fb;
      }

//---------------------------------------------------------
//   STATIC FUNCTIONS FOR FONT CONFIGURATION MANAGEMENT
//---------------------------------------------------------

bool FiguredBassFont::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());

            if (tag == "family")
                  family = e.readElementText();
            else if (tag == "displayName")
                  displayName = e.readElementText();
            else if (tag == "defaultPitch")
                  defPitch = e.readDouble();
            else if (tag == "defaultLineHeight")
                  defLineHeight = e.readDouble();
            else if (tag == "parenthesisRoundOpen")
                  displayParenthesis[1] = e.readElementText()[0];
            else if (tag == "parenthesisRoundClosed")
                  displayParenthesis[2] = e.readElementText()[0];
            else if (tag == "parenthesisSquareOpen")
                  displayParenthesis[3] = e.readElementText()[0];
            else if (tag == "parenthesisSquareClosed")
                  displayParenthesis[4] = e.readElementText()[0];
            else if (tag == "doubleflat")
                  displayAccidental[int(FiguredBassItem::Modifier::DOUBLEFLAT)]= e.readElementText()[0];
            else if (tag == "flat")
                  displayAccidental[int(FiguredBassItem::Modifier::FLAT)]      = e.readElementText()[0];
            else if (tag == "natural")
                  displayAccidental[int(FiguredBassItem::Modifier::NATURAL)]   = e.readElementText()[0];
            else if (tag == "sharp")
                  displayAccidental[int(FiguredBassItem::Modifier::SHARP)]     = e.readElementText()[0];
            else if (tag == "doublesharp")
                  displayAccidental[int(FiguredBassItem::Modifier::DOUBLESHARP)]= e.readElementText()[0];
            else if (tag == "cross")
                  displayAccidental[int(FiguredBassItem::Modifier::CROSS)]     = e.readElementText()[0];
            else if (tag == "backslash")
                  displayAccidental[int(FiguredBassItem::Modifier::BACKSLASH)] = e.readElementText()[0];
            else if (tag == "slash")
                  displayAccidental[int(FiguredBassItem::Modifier::SLASH)]     = e.readElementText()[0];
            else if (tag == "digit") {
                  int digit = e.intAttribute("value");
                  if (digit < 0 || digit > 9)
                        return false;
                  while (e.readNextStartElement()) {
                        const QStringRef& tag(e.name());
                        if (tag == "simple")
                              displayDigit[int(FiguredBassItem::Style::MODERN)]  [digit][int(FiguredBassItem::Combination::SIMPLE)]      = e.readElementText()[0];
                        else if (tag == "crossed")
                              displayDigit[int(FiguredBassItem::Style::MODERN)]  [digit][int(FiguredBassItem::Combination::CROSSED)]     = e.readElementText()[0];
                        else if (tag == "backslashed")
                              displayDigit[int(FiguredBassItem::Style::MODERN)]  [digit][int(FiguredBassItem::Combination::BACKSLASHED)] = e.readElementText()[0];
                        else if (tag == "slashed")
                              displayDigit[int(FiguredBassItem::Style::MODERN)]  [digit][int(FiguredBassItem::Combination::SLASHED)]     = e.readElementText()[0];
                        else if (tag == "simpleHistoric")
                              displayDigit[int(FiguredBassItem::Style::HISTORIC)][digit][int(FiguredBassItem::Combination::SIMPLE)]      = e.readElementText()[0];
                        else if (tag == "crossedHistoric")
                              displayDigit[int(FiguredBassItem::Style::HISTORIC)][digit][int(FiguredBassItem::Combination::CROSSED)]     = e.readElementText()[0];
                        else if (tag == "backslashedHistoric")
                              displayDigit[int(FiguredBassItem::Style::HISTORIC)][digit][int(FiguredBassItem::Combination::BACKSLASHED)] = e.readElementText()[0];
                        else if (tag == "slashedHistoric")
                              displayDigit[int(FiguredBassItem::Style::HISTORIC)][digit][int(FiguredBassItem::Combination::SLASHED)]     = e.readElementText()[0];
                        else {
                              e.unknown();
                              return false;
                              }
                        }
                  }
            else {
                  e.unknown();
                  return false;
                  }
            }
      displayParenthesis[0] = displayAccidental[int(FiguredBassItem::Modifier::NONE)] = ' ';
      return true;
      }

//---------------------------------------------------------
//   Read Configuration File
//
//    reads a configuration and appends read data to g_FBFonts
//    resets everythings and reads the built-in config file if fileName is null or empty
//---------------------------------------------------------

bool FiguredBass::readConfigFile(const QString& fileName)
      {
      QString     path;

      if(fileName == 0 || fileName.isEmpty()) {       // defaults to built-in xml
#ifdef Q_OS_IOS
            {
            extern QString resourcePath();
            QString rpath = resourcePath();
            path = rpath + QString("/fonts_figuredbass.xml");
            }
#else
            path = ":/fonts/fonts_figuredbass.xml";
#endif
            g_FBFonts.clear();
            }
      else
            path = fileName;

      QFile f(path);
      if (!f.open(QIODevice::ReadOnly)) {
            MScore::lastError = tr("Cannot open figured bass description:\n%1\n%2").arg(f.fileName()).arg(f.errorString());
            qDebug("FiguredBass::read failed: <%s>", qPrintable(path));
            return false;
            }
      XmlReader e(&f);
      while (e.readNextStartElement()) {
            if (e.name() == "museScore") {
                  // QString version = e.attribute(QString("version"));
                  // QStringList sl = version.split('.');
                  // int _mscVersion = sl[0].toInt() * 100 + sl[1].toInt();

                  while (e.readNextStartElement()) {
                        if (e.name() == "font") {
                              FiguredBassFont f;
                              if (f.read(e))
                                    g_FBFonts.append(f);
                              else
                                    return false;
                              }
                        else
                              e.unknown();
                        }
                  return true;
                  }
            }
      return false;
      }

//---------------------------------------------------------
//   Get Font Names
//
//    returns a list of display names for the fonts  configured to work with Figured Bass;
//    the index of a name in the list can be used to retrieve the font data with fontData()
//---------------------------------------------------------

QList<QString> FiguredBass::fontNames()
      {
      QList<QString> names;
      foreach(const FiguredBassFont& f, g_FBFonts)
            names.append(f.displayName);
      return names;
      }

//---------------------------------------------------------
//   Get Font Data
//
//    retrieves data about a Figured Bass font.
//    returns: true if idx is valid | false if it is not
// any of the pointer parameter can be null, if that datum is not needed
//---------------------------------------------------------

bool FiguredBass::fontData(int nIdx, QString * pFamily, QString * pDisplayName,
            qreal * pSize, qreal * pLineHeight)
{
      if(nIdx >= 0 && nIdx < g_FBFonts.size()) {
            FiguredBassFont f = g_FBFonts.at(nIdx);
            if(pFamily)       *pFamily          = f.family;
            if(pDisplayName)  *pDisplayName     = f.displayName;
            if(pSize)         *pSize            = f.defPitch;
            if(pLineHeight)   *pLineHeight      = f.defLineHeight;
            return true;
      }
      return false;
}

//---------------------------------------------------------
//
//    MusicXML I/O
//
//---------------------------------------------------------

//---------------------------------------------------------
//   Read MusicXML
//
// Set the FiguredBass state based on the MusicXML <figured-bass> node de.
// Note that onNote and ticks must be set by the MusicXML importer,
// as the required context is not present in the items DOM tree.
// Exception: if a <duration> element is present, tick can be set.
// Return true if valid, non-empty figure(s) are found
//---------------------------------------------------------

#if 0
bool FiguredBass::readMusicXML(XmlReader& e, int divisions)
      {
      bool parentheses = e.attribute("parentheses") == "yes";
      QString normalizedText;
      int idx = 0;
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "duration") {
                  QString val(e.readElementText());
                  bool ok = true;
                  int duration = val.toInt(&ok);
                  if (ok) {
                        duration *= MScore::division;
                        duration /= divisions;
                        setTicks(duration);
                        }
                  else
                        qDebug("MusicXml-Import: bad duration value: <%s>",
                               qPrintable(val));
                  }
            else if (tag == "figure") {
                  FiguredBassItem * pItem = new FiguredBassItem(score(), idx++);
                  pItem->setTrack(track());
                  pItem->setParent(this);
                  pItem->readMusicXML(e, parentheses);
                  items.append(*pItem);
                  // add item normalized text
                  if (!normalizedText.isEmpty())
                        normalizedText.append('\n');
                  normalizedText.append(pItem->normalizedText());
                  }
            else {
                  e.unknown();
                  return false;
                  }
            }
      setText(normalizedText);                  // this is the text to show while editing
      bool res = !normalizedText.isEmpty();
      return res;
      }
#endif

//---------------------------------------------------------
//   hasParentheses
//
//   return true if any FiguredBassItem starts with a parenthesis
//---------------------------------------------------------

bool FiguredBass::hasParentheses() const
      {
      for(FiguredBassItem* item : items)
            if (item->startsWithParenthesis())
                  return true;
      return false;
      }

//---------------------------------------------------------
//   Write MusicXML
//---------------------------------------------------------

void FiguredBass::writeMusicXML(Xml& xml, bool isOriginalFigure, int crEndTick, int fbEndTick, bool writeDuration, int divisions) const
      {
      QString stag = "figured-bass";
      if (hasParentheses())
            stag += " parentheses=\"yes\"";
      xml.stag(stag);
      for(FiguredBassItem* item : items)
            item->writeMusicXML(xml, isOriginalFigure, crEndTick, fbEndTick);
      if (writeDuration)
            xml.tag("duration", ticks() / divisions);
      xml.etag();
      }

//---------------------------------------------------------
//
// METHODS BELONGING TO OTHER CLASSES
//
//    Work In Progress: kept here until the FiguredBass framwork is reasonably set up;
//    To be finally moved to their respective class implementation files.
//
//---------------------------------------------------------

//---------------------------------------------------------
//   Score::addFiguredBass
//    called from Keyboard Accelerator & menus
//---------------------------------------------------------


FiguredBass* Score::addFiguredBass()
      {
      Element* el = selection().element();
      if (el == 0 || (el->type() != Element::Type::NOTE && el->type() != Element::Type::FIGURED_BASS)) {
            if (!MScore::noGui)
                  QMessageBox::information(0,
                     QMessageBox::tr("MuseScore"),
                     QMessageBox::tr("No note or figured bass selected:\n"
                        "Please select a single note or figured bass and retry.\n"),
                     QMessageBox::Ok, QMessageBox::NoButton);
            return 0;
            }

      FiguredBass * fb;
      bool bNew;
      if (el->type() == Element::Type::NOTE) {
            ChordRest * cr = static_cast<Note*>(el)->chord();
            fb = FiguredBass::addFiguredBassToSegment(cr->segment(),
                        (cr->track() / VOICES) * VOICES, 0, &bNew);
            }
      else if (el->type() == Element::Type::FIGURED_BASS) {
            fb = static_cast<FiguredBass*>(el);
            bNew = false;
            }
      else
            return 0;

      if(fb == 0)
            return 0;

      if(bNew)
            undoAddElement(fb);
      select(fb, SelectType::SINGLE, 0);
      return fb;
      }

}

