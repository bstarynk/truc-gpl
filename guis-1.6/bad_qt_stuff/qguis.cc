/* file KnoBas/qguis.cc */
// emacs Time-stamp: <2003 Au 30 Sat 23h26:59 cest {qguis.cc} Basile STARYNKEVITCH@hector.lesours>
/* $Id: qguis.cc 1.1 Sun, 31 Aug 2003 02:17:59 +0200 basile $ */

// this code dont compile and dont work but could be reworked to make
// something usable

//  Copyright © 2003 Basile STARYNKEVITCH

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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qdatetime.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qtooltip.h>

#include "qguis.hh"

#error this dont work really but might be enhanced

bool dbgflag= 0;

const char qguis_release[] = "$Id: qguis.cc 1.1 Sun, 31 Aug 2003 02:17:59 +0200 basile $";

static void
usage (char *argv0, int exitcode= -1)
{
  fprintf (stderr,
	   "usage: %s [-i inputchan] [-o outputchan] [-v]  [...qtargs]\n"
	   "\t | [-p pipecommand] [...qtargs]\n" "\t | [-h] #for this help\n"
	   "\t [-S] #show protocol exchange\n"
	   "\t [ -l logfile] #log input protocol\n"
#ifndef NDEBUG
	   
	   "\t [-D] #for debug output\n"
#endif
	   "where a channel is either a file descriptor number or a file path\n"
	   "eg -i 0 reads from stdin and -i /tmp/input_pipe reads from a namedpipe \n"
	   "and the pipecommand is a command whose stdin is the inputchan\n"
	   "and whose stdout is the outputchan\n"
	   "Version is shown with -v\n"
	   "[this release is %s]\n", argv0, qguis_release);
  if (exitcode >= 0)
    exit (exitcode);
}



/****************************************************************/
GuisParser::GuisParser()
  : QXmlDefaultHandler(),
    _xmlloc(0), _depth(0), _objstack(), _elemstack(), _skiptoend(false) {
  _elemstack.setAutoDelete(true);
};

bool
GuisParser::startDocument(void) {
  dbgprintf("start of document @L%dC%d", lineno(), colno());
  return true;
} //end GuisParser::startDocument

bool
GuisParser::startElement(const QString& namesp, const QString& localname,
			 const QString& qname,
			 const QXmlAttributes& attrs) {
  _depth++;
  dbgprintf("startelem namesp=%s localname=%s qname=%s depth%d @L%dC%d",
	    (const char*)namesp,
	    (const char*)localname, (const char*)qname, _depth,
	    lineno(), colno());
  if (_skiptoend) return true;
  GuisXmlElement* elem = GuisParser::make_element(qname, _depth);
  if (elem) {
    elem->start(this, attrs, lineno(), colno());
    push_element(elem);
  };
  return true;
} // end of GuisParser::startElement


QMap<QString,GuisTagBuilderF_t> GuisParser::_tagdict;

bool
GuisParser::endElement(const QString& namesp, const QString& localname,
		       const QString& qname) {
  dbgprintf("endelem localname=%s qname=%s @L%dC%d",
	    (const char*)localname, (const char*)qname, lineno(), colno());
  GuisXmlElement* elem = top_element();
  dbgprintf("endelem top elem %p tag %s depth %d",
	    elem, elem?((const char*)(elem->name())):"*nil*", _depth);
  if (elem && elem->_qname == qname) {
    _skiptoend = false;
    elem->end(this, lineno(), colno());
    pop_element();
  } else {
    if (_skiptoend) goto end;
    GuisTagBuilderF_t fun = _tagdict[qname];
    dbgprintf("endelem fun %p", fun);
    if (fun) {
      fprintf(stderr,  "qguis got unexpected </%s>",
	      (const char*)qname);
      if (elem) 
	fprintf(stderr, " instead of </%s>", (const char*)(elem->_qname));
      fprintf(stderr,  " at line%d col%d depth%d\n",
	      lineno(), colno(), _depth);
      return false;
    };
  };
 end:
  _depth--;
  return true;
} // end of GuisParser::endElement

bool
GuisParser::characters(const QString& ch) {
  dbgprintf("characters '%s' @L%dC%d", (const char*)ch, lineno(), colno());
  if (_skiptoend) return true;
  GuisXmlElement* elem = top_element();
  if (elem  && elem->_depth == _depth)
    elem->chars(this, ch, lineno(), colno());
  return true;
} // end of GuisParser::characters

void
GuisApplication::parse_arguments(int& argc, char**argv) { 
  char *inarg = 0;
  char *outarg = 0;
  char *pipearg = 0;
  char *pipecmd = 0;
  char *logname = 0;
  int inpipefds[2] = {-1,-1};
  int outpipefds[2] = {-1,-1};
  bool showprotoc=false;
  for (int i = 1; i < argc; i++) {
    char *curarg = argv[i];
    if (curarg[0] == '-') {
#define ISLONGARG(Arg) (curarg[1]=='-' && !strcmp(curarg,Arg))
      if (curarg[1] == 'i' || ISLONGARG ("--input")) {
	if (curarg[2])
	  inarg = curarg + 2;
	else if (i < argc - 1)
	  inarg = argv[++i];
      } else if (curarg[1] == 'o' || ISLONGARG ("--output")) {
	if (curarg[2])
	  outarg = curarg + 2;
	else if (i < argc - 1)
	  outarg = argv[++i];
      } else if (curarg[1] == 'l' || ISLONGARG ("--log")) {
	if (curarg[2])
	  logname = curarg + 2;
	else if (i < argc - 1)
	  logname = argv[++i];
      } else if (curarg[1] == 'p' || ISLONGARG ("--pipe")) {
	if (curarg[2])
	  pipearg = curarg + 2;
	else if (i < argc - 1)
	  pipearg = argv[++i];
      } else if (curarg[1] == 'h' || ISLONGARG ("--help"))
	usage (argv[0], 0);
#ifndef NDEBUG
      else if (curarg[1] == 'D' || ISLONGARG ("--debug"))
	showprotoc = dbgflag = 1;
#endif
      else if (curarg[1] == 'S' || ISLONGARG ("--showproto"))
	showprotoc = 1;
      else if (curarg[1] == 'v' || ISLONGARG ("--version"))
	
	fprintf (stderr,
		 "%s:\n release %s\n compiled on " __DATE__ " at " __TIME__
		 "\n", argv[0], qguis_release);
      else
	goto badarg;
    } else {
    badarg:
      fprintf (stderr, "%s bad argument %s\n", argv[0], curarg);
      usage (argv[0], 1);
    }
  };
  if (inarg) {
    if (isdigit (inarg[0]))
      _infd = atoi (inarg);
    else {
      _infd = open (inarg, O_RDONLY);
      if (_infd < 0) {
	fprintf (stderr, "%s bad input %s : %s\n",
		 argv[0], inarg, strerror (errno));
	exit (1);
      }
    }
  }
  if (outarg) {
    if (isdigit (outarg[0]))
      _outfd = atoi (outarg);
    else {
      _outfd = open (outarg, O_WRONLY);
      if (_outfd < 0) {
	fprintf (stderr, "%s bad output %s : %s\n",
		 argv[0], outarg, strerror (errno));
	exit (1);
      }
    }
  }
  if (pipearg) {
    pipecmd = pipearg;
    if (_infd >= 0 || _outfd >= 0) {
      fprintf (stderr,
	       "%s pipe command -p %s incompatible with -i or -o\n",
	       argv[0], pipearg);
      exit (1);
    };
    if (pipe (inpipefds)) {
      perror ("pipe in");
      exit (1);
    };
    if (pipe (outpipefds)) {
      perror ("pipe out");
      exit (1);
    };
    _pipepid = fork ();
    if (_pipepid == 0) {
      /* child process */
      int i = 0;
      char *shell = 0;
      nice (2);
      if (dup2 (outpipefds[0], 0) < 0 || dup2 (inpipefds[1], 1) < 0) {
	perror ("dup2");
	exit (1);
      };
      close (outpipefds[1]);
      close (inpipefds[0]);
      for (i = 64; i > 2; i--)
	(void) close (i);
      fprintf (stderr, "before exec %s\n", pipearg);
      if (!strchr (pipearg, ' '))
	/* optimize case when no space, so no shell needed... */
	execlp (pipearg, pipearg, 0);
      else {
	shell = getenv ("SHELL");
	if (!shell)
	  shell = "/bin/sh";
	execl (shell, shell, "-c", pipearg, 0);
      }
      fprintf (stderr, "exec %s failed - %s\n", pipearg, strerror (errno));
      exit (1);
    } else if (_pipepid < 0) {
      perror ("fork");
      exit (1);
    } else {
      /* parent process */
      _infd = inpipefds[0];
      close (inpipefds[1]);
      _outfd = outpipefds[1];
      close (outpipefds[0]);
      dbgprintf ("after fork _pipepid=%d _infd=%d _outfd=%d", _pipepid, _infd,
		 _outfd);
    }
  };
  if (_infd>=0) {
    fcntl(_infd, F_SETFL, O_NONBLOCK);
    dbgprintf("_infd%d", _infd);
    _insn = new QSocketNotifier(_infd,QSocketNotifier::Read,this);
    QObject::connect(_insn, SIGNAL(activated(int)),
		     this, SLOT(dataReceived()) );
  }
  if (_outfd>=0) {
    fcntl(_outfd, F_SETFL, O_NONBLOCK);
    dbgprintf("_outfd%d", _outfd);
    _outsn = new QSocketNotifier(_outfd,QSocketNotifier::Write,this);
    QObject::connect(_outsn, SIGNAL(activated(int)), 
		     this, SLOT(outputPossible()) );
  }
  if (logname)
    _loginput = fopen(logname, "w");
  if (showprotoc) {
    char hnam[100];
    _mwin = new QMainWindow();
    _mwin->setCaption("QGuis protocol");
    QString titlestr;
    memset(hnam, 0, sizeof(hnam));
    gethostname(hnam, sizeof(hnam)-1);
    titlestr.sprintf("<b>Qguis</b> on <tt>%s</tt> pid %d", hnam, (int) getpid());
    QLabel* titlelab = new QLabel(titlestr,(QWidget*)_mwin);
    titlelab->show();
    QMenuBar* mbar = _mwin->menuBar();
    QPopupMenu* tracemenu = new QPopupMenu(_mwin);
    QToolTip::add(tracemenu,"menu for <em>protocol</em> trace");
    mbar->insertItem("Trace", tracemenu);
    tracemenu->insertItem("&Start trace", this, SLOT(trace_on()), CTRL+Key_S);
    tracemenu->insertItem("trace &Off", this, SLOT(trace_off()), CTRL+Key_O);
    tracemenu->insertItem("&Clear trace", this, SLOT(trace_clear()), CTRL+Key_C);
#ifndef NDEBUG
    tracemenu->insertItem("Toggle &Debug", this, SLOT(toggle_debug()), CTRL+Key_D);
#endif
    _ptextedit = new QTextEdit(_mwin);
    _mwin->setCentralWidget(_ptextedit);
    _ptextedit->setReadOnly(true);
    _ptextedit->setTextFormat(RichText);
  };
} // end of GuisApplication::parse_arguments

GuisApplication::GuisApplication(int& argc, char**argv) :
  QApplication(argc, argv),
  _mwin(0), _ptextedit(0), _trace(false), _loginput(0),
  _insn(0), _outsn(0), 
  _infd(-1), _outfd(-1), _pipepid(-1), _buf(0), _off(0), _buflen(0),
  _outlist(), _xmlreader(), _parser(), _objdict(4021) { 
#ifndef NDEBUG
  char *dbgenv = getenv ("GUISRV_DEBUG");
  if (dbgenv)
    dbgflag = atoi (dbgenv);
#endif
  parse_arguments(argc, argv);
  _xmlreader.setContentHandler(&_parser);
  if (_mwin) _mwin->show();
} // end of GuisApplication::GuisApplication


void GuisApplication::processRequest(QString& lin) {
  dbgprintf("request lin=%s", (const char*)lin);
  QXmlInputSource src;
  src.setData(lin);
  _reqlin = lin;
  if (_ptextedit && _trace) {
    QString s;
    QTime nowtim = QTime::currentTime();
    //s += "<font size='-1'>";
    s += QString(nowtim.toString(QString("hh:mm:ss.zzz")));
    s += "* ";
    //s += "</font>";
    s += " <font color='darkblue'><b>" ;
    s += QStyleSheet::escape(lin) ;
    s += "</b></font>" ;
    _ptextedit->append(s);
  };
  if (_loginput) {
    fputs((const char*)lin, _loginput);
    putc('\n', _loginput);
    fflush(_loginput);
  };
  _xmlreader.parse(&src);
} // end GuisApplication::processRequest



void GuisApplication::dataReceived(void) {
  char *pc=0, *eol=0;
  int cnt=0, eof=0;
  dbgprintf("datarecieved off=%d", _off);
  if (_buflen<=0) {
    _buflen=5000;
    _buf  = new char[_buflen];
  };
  do {
    if (_off+100 >= _buflen) {
      int newlen = (5*_buflen/4 + 300) | 0x1f;
      char* newbuf = new char[newlen];
      memset(newbuf, 0, newlen);
      memcpy(newbuf, _buf, _off);
      delete[] _buf;
      _buf=newbuf; _buflen=newlen;
    };
    do {
      cnt = read(_infd, _buf+_off, _buflen-_off-1);
      dbgprintf("after read cnt=%d - %s", cnt, strerror(errno));
    } while (cnt<0 && errno==EINTR);
    if (cnt>0) _off+=cnt;
    dbgprintf("cnt=%d off=%d buf=%s", cnt, _off, _buf);
  } while (cnt>0);
  // on eof add a newline 
  if (cnt==0) { eof=1; _buf[_off++] ='\n'; };
  for (pc=_buf; (eol=strchr(pc,'\n'))!=0; pc=eol+1) {
    *eol=(char)0;
    dbgprintf("line %s", pc);
    QString lin(pc);
    processRequest(lin);
  };
  int curoff= pc-_buf;
  memmove(_buf+curoff, _buf, _off-curoff);
  _off -= curoff;
  _buf[_off] = _buf[_off+1] = (char)0;
  if (_buflen>10000 && _off+6000<_buflen) {
    int newlen= (_off+5000) | 0x1f;
    char *newbuf = new char[newlen];
    memset(newbuf, 0, newlen);
    memcpy(newbuf, _buf, _off);
    delete[] _buf;
    _buf = newbuf;
    _buflen = newlen;
  };
  if (eof) {
    dbgprintf("eof!");
    _insn->setEnabled(false);
  };
} // end of GuisApplication::dataReceived

void GuisApplication::add_out(QCString& str) {
  if (str.isEmpty()) return;
  if (_ptextedit && _trace) {
    QString s;
    QTime nowtim = QTime::currentTime();
    //s += "<font size='-1'>";
    s += QString(nowtim.toString(QString("hh:mm:ss.zzz")));
    s += "= ";
    //s += "</font>";
    s += " <font color='darkgreen'><i>" ;
    s += QStyleSheet::escape(str) ;
    s += "</i></font>" ;
    _ptextedit->append(s);
  };
  if (_outsn)
    _outsn->setEnabled(true);
  if (str[str.length()-1]!='\n')
    str += ('\n');
  _outlist << str;
}

void GuisApplication::trace_on(void) {
  _trace = true;
  if (_ptextedit) {
    QString s;
    QTime nowtim = QTime::currentTime();
    //s += "<font size='-1'>";
    s += "<hr>";
    s += QString(nowtim.toString(QString("hh:mm:ss.zzz")));
    //s += "</font>";
    s += " <font color='darkred'><big>TRACE STARTED</big></font><br/>" ;
    _ptextedit->append(s);
  };
}

void GuisApplication::trace_off(void) {
  _trace = false;
  if (_ptextedit) {
    QString s;
    QTime nowtim = QTime::currentTime();
    //s += "<font size='-1'>";
    s += "<hr>";
    s += QString(nowtim.toString(QString("hh:mm:ss.zzz")));
    //s += "</font>";
    s += " <font color='darkred'><big>TRACE STOPPED</big></font><br/>" ;
    _ptextedit->append(s);
  };
}

void GuisApplication::trace_clear(void) {
  if (_ptextedit) {
    QString s;
    QTime nowtim = QTime::currentTime();
    //s += "<font size='-1'>";
    s += QString(nowtim.toString(QString("hh:mm:ss.zzz")));
    //s += "</font>";
    s += " <font color='darkred'><big>TRACE CLEARED</big></font>" ;
    _ptextedit->setText(s);
  };
}

#ifndef NDEBUG
void GuisApplication::toggle_debug(void) {
  dbgflag = !dbgflag;
  fprintf(stderr, "***qguis debug flag set to %s\n", dbgflag?"on":"off");
  if (_ptextedit) {
    QString s;
    QTime nowtim = QTime::currentTime();
    //s += "<font size='-1'>";
    s += QString(nowtim.toString(QString("hh:mm:ss.zzz")));
    //s += "</font>";
    s += " <font color='darkred'><big>DEBUG ";
    s += dbgflag? "SET" : "CLEARED";
    s += "</big></font>" ;
    _ptextedit->append(s);
  };
}
#endif


void GuisApplication::outputPossible(void) {
  bool canwrite=true;
  while (canwrite) {
    if (_outlist.empty()) {
      if (_outsn) _outsn->setEnabled(false);
      return;
    }
    QCString& outstr = _outlist.first();
    if (outstr.isEmpty()) {
      _outlist.pop_front();
    } else {
      const char* ps = outstr;
      const char* pc = ps;
      int slen = strlen(ps);
      int cnt= 0;
      errno=0;
      while (slen>0) {
	cnt=write(_outfd,pc,slen);
	if (cnt<0) {
	  if (errno==EINTR) continue;
	  else { canwrite=false; break; };
	};
	if (cnt>0) { slen -= cnt; pc += cnt; };
      };
      if (pc>ps) outstr.remove(0, pc-ps);
    };
  };
} // end of GuisApplication::outputPossible

QObject* GuisApplication::namedobj(const QString&namestr) const {
  return _objdict.find(namestr);
}

void GuisApplication::set_namedobj(QObject* obj, const QString& str) {
  if (!str.isEmpty()) {
    dbgprintf("set named obj %s to %p (/%s)",
	      (const char*)str, obj, obj?(obj->className()):"*nil*");
    if (obj) {
      _objdict.insert(str, obj);
      QObject::connect(obj, SIGNAL(destroyed(QObject*)),
		       this, SLOT(destroyed_named(QObject*)));
    };
  };
}

void GuisApplication::destroyed_named(QObject* destrobj) {
  QString namestr(destrobj->name(0));
  if (!namestr.isEmpty()) {
    QObject* obn = _objdict.find(namestr);
    if (obn == destrobj) {
      _objdict.remove(namestr);
    };
  };
}

void GuisApplication::forget_named(const QString&namestr, bool deleteflag) {
  if (!namestr.isEmpty()) {
    QObject* obn = _objdict.find(namestr);
    if (obn) {
      _objdict.remove(namestr);
      if (deleteflag) delete obn;
    };
  };
}

void GuisApplication::forget_named(QObject*forgobj, bool deleteflag) {
  QString namestr(forgobj->name(0));
  if (!namestr.isEmpty()) {
    QObject* obn = _objdict.find(namestr);
    if (obn == forgobj) {
      _objdict.remove(namestr);
      if (deleteflag) delete forgobj;
    };
  };
}


void GuisXmlElement::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {};
void GuisXmlElement::end(GuisParser*p, int line, int col) {};
void GuisXmlElement::chars(GuisParser*p, const QString& ch, int line, int col) {};

GuisXmlElement::~GuisXmlElement() {};

int main(int argc, char**argv) {
  GuisApplication app(argc, argv);
  return app.exec();
}

///////////////////////////////////////////////////////////////////////

namespace GuisTags {
  template<class WidgetType> class WidgetElem : public GuisXmlElement {
  protected:
    WidgetType* _widg;
    QString _name;
    QString _parentname;
    QString _tip;
    QString _caption;
    bool _shown;
    WidgetElem(const QString& qn, int depth) : GuisXmlElement(qn,depth), _widg(0), _name(), _parentname(), _shown(0) {};
    void widget_attributes(const QXmlAttributes& attrs) {
      _name = attrs.value(QString("name"));
      _parentname = attrs.value(QString("parent"));
      _shown = get_int_attr(attrs,"show");
      _tip = attrs.value(QString("tip"));
      _caption = attrs.value(QString("caption"));
    };
    void widget_conf(void) {
      if (_widg) {
	if (_shown) _widg->show();
	if (!_tip.isEmpty())
	  QToolTip::add(_widg, _tip); // perhaps XML-escape the tip? ***
	if (!_caption.isEmpty())
	  _widg->setCaption(_caption);
      };
    }
    QWidget* get_parent(GuisParser* parser) const {
      if (_parentname.isEmpty()) 
	return dynamic_cast<QWidget*>(parser->top_object());
       else 
	 return dynamic_cast<QWidget*>(namedobj(_parentname));
    };
    virtual void end(GuisParser*p, int line, int col) {
      Q_ASSERT(p->top_object() == _widg);
      p->pop_object();
      widget_conf();
    };
  };
  /****************/
  typedef QHBox GuisHBox;
  class HBoxElem : public WidgetElem<GuisHBox> {
  protected:
    static Register reg_it;
    HBoxElem(const QString& qn, int depth) : WidgetElem<GuisHBox>(qn,depth) {};
    virtual QWidget* make_box(QWidget*parent, const char*name) 
    { return new GuisHBox(parent,name) ;  };
  public:
    static GuisXmlElement* hbox_builder(const QString& qn, int depth) {
      return new HBoxElem(qn, depth);
    };
  virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
  };
  ////
  GuisXmlElement::Register HBoxElem::reg_it("hbox", hbox_builder);
  ////
  void HBoxElem::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {
    widget_attributes(attrs);
    QWidget* parent = get_parent(p);
    _widg = (GuisHBox*) make_box(parent, _name);
    dbgprintf("boxelem start _widg=%p name=%s", _widg, (const char*)_name);
    set_namedobj(_widg, _name);
    bool ok=false;
    int spacing = get_int_attr(attrs, "spacing", &ok);
    if (ok) _widg->setSpacing(spacing);
    int margin = get_int_attr(attrs, "margin", &ok);
    if (ok) _widg->setMargin(margin);
    p->push_object(_widg);
  };
  /****************/
  typedef QVBox GuisVBox;
  class VBoxElem : public HBoxElem {
  protected:
    static Register reg_it;
    VBoxElem(const QString& qn, int depth) : HBoxElem(qn, depth) {};
  public:   
    static GuisXmlElement* vbox_builder(const QString& qn, int depth) {
      return new VBoxElem(qn, depth);
    };
    virtual QWidget* make_box(QWidget*parent, const char*name) 
     { return new GuisVBox(parent,name);  };
  };
  GuisXmlElement::Register VBoxElem::reg_it("vbox", vbox_builder);
  /****************/
  typedef QLabel GuisLabel;
  class LabelElem : public WidgetElem<GuisLabel> {
    int _labstartcol, _labendcol, _labline;
  protected:
    static Register reg_it;
    LabelElem(const QString& qn, int depth) : WidgetElem<GuisLabel>(qn,depth),
    _labstartcol(0), _labendcol(0), _labline(0){};
  public:
    static GuisXmlElement* label_builder(const QString& qn, int depth) {
      return new LabelElem(qn, depth);
    };
    virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
    virtual void end(GuisParser*p, int line, int col);
  };
  GuisXmlElement::Register LabelElem::reg_it("label", label_builder);
  void LabelElem::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {
    _labline=line; _labstartcol=col;
    dbgprintf("labelelem start L%dC%d", line, col);
    widget_attributes(attrs);
    QWidget* parent = get_parent(p);
    _widg = new GuisLabel(parent,_name);
    set_namedobj(_widg, _name);
    _widg->setTextFormat(Qt::RichText);
    p->push_object(_widg);
    p->setskip(true);
  };
  void LabelElem::end(GuisParser*p, int line, int col){
    Q_ASSERT(line==_labline);
    _labendcol=col-3-_qname.length();
    QString txts = ((GuisApplication*)qApp)->substr_req(_labstartcol, _labendcol);
    dbgprintf("labelelem end startcol%d endcol%d txts='%s'",
	      _labstartcol, _labendcol, (const char*)txts);
    txts.replace("&nl;", "\n");
    _widg->setText(txts);
    WidgetElem<GuisLabel>::end(p,line,col);    
  };
  /****************/
  class SetLabelElem : public GuisXmlElement {
    int _labstartcol, _labendcol, _labline;
    GuisLabel* _labw;
  protected:
    static Register reg_it;
    SetLabelElem(const QString& qn, int depth) : GuisXmlElement(qn,depth),
    _labstartcol(0), _labendcol(0), _labline(0), _labw(0) {};
    static GuisXmlElement* setlabel_builder(const QString& qn, int depth) {
      return new SetLabelElem(qn, depth);
    };
    virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
    virtual void end(GuisParser*p, int line, int col);
  };
  GuisXmlElement::Register SetLabelElem::reg_it("set_label", setlabel_builder);
  void SetLabelElem::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {
    _labline=line; _labstartcol=col;
    dbgprintf("setlabelelem start L%dC%d", line, col);
    QString labname=attrs.value(QString("of"));
    if (!labname.isEmpty())
      _labw = dynamic_cast<GuisLabel*>(namedobj(labname));
    if (!_labw) {
      fprintf(stderr, "qguis: invalid label to set (attr. of?): %.120s\n",
	      (const char*) (((GuisApplication*)qApp)->request()));
    };
    p->setskip(true);
  };
  void SetLabelElem::end(GuisParser*p, int line, int col) {
    Q_ASSERT(line==_labline);
    _labendcol=col-3-_qname.length();
    QString txts = ((GuisApplication*)qApp)->substr_req(_labstartcol, _labendcol);
    dbgprintf("setlabelelem end startcol%d endcol%d txts='%s'",
	      _labstartcol, _labendcol, (const char*)txts);
    txts.replace("&nl;", "\n");
    if (_labw) {
      dbgprintf("setting text of label %p", _labw);
      _labw->setText(txts);
    };
  };				// end of SetLabelElem
  /****************/
  class SetTipElem : public GuisXmlElement {
    int _tipstartcol, _tipendcol, _tipline;
    QWidget* _tipw;
  protected:
    static Register reg_it;
    SetTipElem(const QString& qn, int depth) : GuisXmlElement(qn,depth),
    _tipstartcol(0), _tipendcol(0), _tipline(0), _tipw(0) {};
    static GuisXmlElement* settip_builder(const QString& qn, int depth) {
      return new SetTipElem(qn, depth);
    };
    virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
    virtual void end(GuisParser*p, int line, int col);
  };
  GuisXmlElement::Register SetTipElem::reg_it("set_tip", settip_builder);
  void SetTipElem::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {
    _tipline=line; _tipstartcol=col;
    dbgprintf("settipelem start L%dC%d", line, col);
    QString labname=attrs.value(QString("of"));
    if (!labname.isEmpty())
      _tipw = dynamic_cast<QWidget*>(namedobj(labname));
    if (!_tipw) {
      fprintf(stderr, "qguis: invalid widget to tip (attr. of?): %.120s\n",
	      (const char*) (((GuisApplication*)qApp)->request()));
    };
    p->setskip(true);
  };
  void SetTipElem::end(GuisParser*p, int line, int col) {
    Q_ASSERT(line==_tipline);
    _tipendcol=col-3-_qname.length();
    QString txts = ((GuisApplication*)qApp)->substr_req(_tipstartcol, _tipendcol);
    dbgprintf("settipelem end startcol%d endcol%d txts='%s'",
	      _tipstartcol, _tipendcol, (const char*)txts);
    txts.replace("&nl;", "\n");
    if (_tipw) {
      dbgprintf("setting tip of widget %p", _tipw);
      QToolTip::add(_tipw, txts);
    };
  };				// end of SetTipElem
  /****************/
  typedef QMainWindow GuisMainWindow;
  class MainWindowElem : public WidgetElem<GuisMainWindow> {
  protected:
    static Register reg_it;
    MainWindowElem(const QString& qn, int depth) : WidgetElem<GuisMainWindow>(qn,depth) {};
  public:
    static GuisXmlElement* mainwindow_builder(const QString& qn, int depth) {
      return new MainWindowElem(qn, depth);
    };
    virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
  };
  GuisXmlElement::Register MainWindowElem::reg_it("mainwindow", mainwindow_builder);
  void MainWindowElem::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {
    widget_attributes(attrs);
    QWidget* parent = get_parent(p);
    _widg = new GuisMainWindow(parent,_name);
    set_namedobj(_widg, _name);
    p->push_object(_widg);
  };				// end of MainWindowElem
  /****************/
  typedef QMenuBar GuisMenuBar;
  class MenuBarElem : public WidgetElem<GuisMenuBar> {
  protected:
    static Register reg_it;
    MenuBarElem(const QString& qn, int depth) : WidgetElem<GuisMenuBar>(qn,depth) {};
  public:
    static GuisXmlElement* menubar_builder(const QString& qn, int depth) {
      return new MenuBarElem(qn, depth);
    };
    virtual void start(GuisParser*p, const QXmlAttributes& attrs, int line, int col);
  };
  GuisXmlElement::Register MenuBarElem::reg_it("menubar", menubar_builder);
  void MenuBarElem::start(GuisParser*p, const QXmlAttributes& attrs, int line, int col) {
    widget_attributes(attrs);
    QWidget* parent = get_parent(p);
    QMainWindow* mw = dynamic_cast<QMainWindow*>(parent);
    if (mw)
      _widg = mw->menuBar();
    else 
      _widg = new GuisMenuBar(parent,_name);
    set_namedobj(_widg, _name);
    p->push_object(_widg);
  };				// end of MenuBarElem
#warning perhaps add MenuItemElem?
};				// end of namespace GuisTags


// eof $Id: qguis.cc 1.1 Sun, 31 Aug 2003 02:17:59 +0200 basile $ 
