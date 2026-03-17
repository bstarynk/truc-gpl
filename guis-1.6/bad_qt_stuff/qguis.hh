/* file Knobas/qguis.hh */
#ifndef QGUIS_INCLUDED_
#define QGUIS_INCLUDED_ 1
/* $Id: qguis.hh 1.1 Sun, 31 Aug 2003 02:17:59 +0200 basile $ */
/* emacs Time-stamp: <2003 Au 30 Sat 23h26:10 cest {qguis.hh} Basile STARYNKEVITCH@hector.lesours> */

//  Copyright © 2003 Basile STARYNKEVITCH

// this code dont compile and dont work but could be worked out to
// make something usable

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qapplication.h>
#include <qsocketnotifier.h>
#include <qmainwindow.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qtextedit.h>
#include <qstringlist.h>
#include <qxml.h>
#include <qdict.h>
#include <qptrstack.h>
#include <qguardedptr.h>
#include <qvaluevector.h>

#include <cstdio>

#error this does not really work and should be modified and debugged

#ifndef NDEBUG
extern bool dbgflag;

#define dbgprintf(Fmt,Args...) do { if(dbgflag) {			\
  fprintf(stderr, "%s:%d!! " Fmt "\n", __FILE__, __LINE__, ##Args);	\
  fflush(stderr);							\
} } while (0)
#else
#define dbgprintf(Fmt,Args...) do{}while(0)
#endif


#warning perhaps not use XML for reading but a real scripting language?
class GuisXmlElement;

typedef QXmlSimpleReader GuisXmlReader;


typedef GuisXmlElement* (*GuisTagBuilderF_t) (const QString&, int);

class GuisParser : public QXmlDefaultHandler {
private:
  QXmlLocator *_xmlloc;		// locator
  int _depth;			// current depth
  QPtrStack<QObject> _objstack;	// object stack
  QPtrStack<GuisXmlElement> _elemstack;	// element stack
  static QMap<QString,GuisTagBuilderF_t> _tagdict; // dictionnary of known tags
  bool _skiptoend;		// skip to matching end tag
public:
  static void register_tag(const QString& qname, GuisTagBuilderF_t fun) {
    _tagdict.insert(qname,fun);
  };
  static GuisXmlElement* make_element(const QString& qname, int depth) {
    GuisTagBuilderF_t fun = _tagdict[qname];
    if (fun) return (*fun)(qname, depth);
    else return 0;
  };
  QObject* top_object() const { return _objstack.top(); };
  inline void push_object(QObject*ob);
  inline void pop_object(void);
  inline void push_element(GuisXmlElement*el);
  inline void pop_element(void);
  GuisXmlElement* top_element() const { return _elemstack.top(); };
  virtual bool startDocument(void);
  virtual bool startElement(const QString& namesp, const QString& localname,
		    const QString& qname,
		    const QXmlAttributes& attrs);
  virtual bool endElement(const QString& namesp, const QString& localname,
		  const QString& qname);
  virtual bool characters(const QString& ch);
  virtual void setDocumentLocator(QXmlLocator* loc) {
    dbgprintf("locator %p", loc);
    _xmlloc = loc;
  };
  void setskip(bool skip=true) { _skiptoend=skip; };
  bool skip(void) const { return _skiptoend; };
  int lineno(void) const { return _xmlloc?(_xmlloc->lineNumber()):(-1); };
  int colno(void) const { return _xmlloc?(_xmlloc->columnNumber()):(-1); };
  GuisParser();
  virtual ~GuisParser() { _xmlloc=0; };
};				// end of GuisParser



class GuisXmlElement {
  friend class GuisApplication;
  friend class GuisParser;
protected:
  struct Register {		// empty structure for registration
    Register(const char* qn, GuisTagBuilderF_t fun) {
      GuisParser::register_tag(QString(qn),fun);
    }
  };
  const QString _qname;
  const int _depth;
  GuisXmlElement(const QString& qn, int depth) : _qname(qn), _depth(depth) {};
public:
  virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
  virtual void end(GuisParser*p, int line, int col);
  virtual void chars(GuisParser*p, const QString& ch, int line, int col);
  virtual ~GuisXmlElement();
  const QString& name() const {return _qname;};
  int depth() const {return _depth;};
};


void
GuisParser::push_object(QObject*ob)  {
  dbgprintf("push object %p (%s/%s) height %d",
	    ob, (const char*)(ob->name()), (ob->className()),
	    _objstack.count());
  _objstack.push(ob);
}

void
GuisParser::pop_object(void)  {
#ifndef NDEBUG
  QObject* ob = _objstack.top();
  dbgprintf("pop object %p height %d", ob, _objstack.count());
#endif
  _objstack.pop();
}

void
GuisParser::push_element(GuisXmlElement*el)  {
  dbgprintf("push elem %p tag %s depth %d height %d",
	    el, (const char*)(el->name()), el->depth(), _elemstack.count());
  _elemstack.push(el);
}

void
GuisParser::pop_element(void)  {
#ifndef NDEBUG
  GuisXmlElement* el = _elemstack.top();
  dbgprintf("pop element %p tag %s depth %d height %d",
	    el, (const char*)(el->name()), el->depth(), _elemstack.count());
#endif
  _elemstack.pop();
}

////////////////////////////////////////////////////////////////



class GuisResponder : public QObject {
  Q_OBJECT
private:
#warning should be designed - see comment
  // the vague idea is that responders are used to send back replies
  // from guis to the application; they should provides slots &
  // signals related to these tasks.
};				// end of class GuisResponder

////////////////////////////////////////////////////////////////
class GuisApplication : public QApplication {
  Q_OBJECT
public:
  GuisApplication(int& argc, char** argv);
  void add_out(QCString& str);
protected:
  void parse_arguments(int& argc, char** argv);
  GuisApplication& operator <<(QCString& str)
    { add_out(str); return *this; };
private:
  QMainWindow *_mwin;		// main window (for protocol)
  QTextEdit* _ptextedit;	// protocol text
  bool _trace;			// show the protocol in above text
  FILE* _loginput;		// file to log the input
  QSocketNotifier *_insn;	// input notifier
  QSocketNotifier *_outsn;	// output notifier
  int _infd, _outfd;		// inut & output file descriptors
  int _pipepid;			// pid of piped command (if any)
  char* _buf;			// input buffer
  int _off;			// offset inside _buf
  int _buflen;			// length of _buf
  QValueList<QCString> _outlist;  // output reply list
  QString _reqlin;		// current request line
  GuisXmlReader _xmlreader;	// the xml reader
  GuisParser _parser;		// the xml parser
  QDict<QObject> _objdict;	// global object dictionnary
public slots:
  void dataReceived(void);	// read possible on _infd
  void outputPossible(void);	// write possible on _outfd
  void trace_on(void);		// [re]start trace of protocol
  void trace_off(void);		// stop trace
  void trace_clear(void);	// clear the trace
  void destroyed_named(QObject*); // a named object has been destroyed
#ifndef NDEBUG
  void toggle_debug(void);	// toggle the debug output
#endif
public:
  virtual void processRequest(QString& lin); // process a request
  virtual QObject* namedobj(const QString&) const;
  virtual void set_namedobj(QObject* obj, const QString& str=0);
  virtual void forget_named(const QString& name, bool deleteflag=false);
  virtual void forget_named(QObject* obj, bool deleteflag=false);
  QString substr_req(int from, int to) const {
    if (from<0) from=0;
    if (to<from) return QString::null;
    return _reqlin.mid(from, to-from);
  };
  const QString& request(void) const { return _reqlin; };
};

inline int get_int_attr(const QXmlAttributes& attrs, const char*name,
			bool *ok=0)  {
  QString val = attrs.value(QString(name));
  if (!val.isEmpty()) {
    return val.toInt(ok);
  };
  if (ok) *ok=false;
  return 0;
}

static inline
void set_namedobj(QObject* obj, const QString&namestr) {
  ((GuisApplication*)qApp)->set_namedobj(obj,namestr);
}

static inline
QObject* namedobj(const QString&namestr) {
  return ((GuisApplication*)qApp)->namedobj(namestr);
};

static inline void
forget_named(const QString&namestr, bool deleteflag=false) {
  ((GuisApplication*)qApp)->forget_named(namestr, deleteflag);
}

static inline void forget_named(QObject*forgobj, bool deleteflag=false) {
  ((GuisApplication*)qApp)->forget_named(forgobj, deleteflag);
}

#endif /*QGUIS_INCLUDED_*/
/* eof $Id: qguis.hh 1.1 Sun, 31 Aug 2003 02:17:59 +0200 basile $ */
