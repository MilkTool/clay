#include "claydoc.hpp"
#include <fstream>
#include <sstream>
#include <errno.h>

using namespace clay;
using namespace std;

static string getCssStyle()
{
    return
    "* { padding: 0; margin: 0 }\n"
    "body { background: #fff; font-family: sans-serif; font-size: 0.8em; }\n"
    "#mainContentInner { margin-left: 10%; }\n"
    "#navigation { background: #999; }\n"
    "#navigation li { list-style: none; }\n"
    "#navigation li a { "
        "display: block;"
        "padding: 0.5em;"
        "float: left;"
        "background: #999;"
        "text-decoration: underline;"
        "color: #000;"
        "font-weight: bold;"
     "}\n"
    "#navigation .post-ul { clear: both; }\n"
    "#main h1 { background: #aaa; padding-left: 10% }\n"
    "#mainContentHeader { background: #aaa; border-bottom: 1px solid black; }\n"
    "#mainContentHeader .inlinedoc { padding-left: 11%; }\n"
    "h2 { padding-bottom: 0.5em; }\n"
    "section { padding: 2em; }\n"
    "section h2 { "
        "width: 80%;"
        "display: block;"
        "border-bottom: 1pxsolid #aaa;"
        "padding: 0;"
        "margin-bottom: 1.5em;"
        "font-size: 1em;"
    "}\n"
    "h3 { font-weight: normal; }\n"
    ".functionPredicate { display: block; font-size: 0.8em; margin-left: 0.7em; }\n"
    ".keyword { "
        "font-weight: bold;"
        "width: 5em;"
        "display: inline-block;"
        "text-align: right;"
        "padding-right: 0.5em;"
        "color: #522;"
        "line-height: 1.4em;"
    "}\n"
    ".identifier { height: 1.4em; line-height: 1.4em; display: inline-block }\n"
    ".definition , .overload { margin-top: 0.5em; }\n"
    ".inlinedoc { padding-bottom: 1em; padding-left: 1em }\n"
    "a.reference { text-decoration: none; color: #000 }\n"
    "a.reference:hover { text-decoration: underline; color: #00f; }\n"
    ".brokenreference { background: url(underline.gif) bottom repeat-x; }\n"
    "#moduleIndexOuter {}\n"
    "#moduleIndexOuter li { list-style: none; }\n"
    ",moduleIndexSectionHeader { margin-bottom: 1em; padding-top: 2em; clear: both; }\n"
    ".moduleIndexItem { display: block; float: left; width: 49%; }\n"
    ".moduleIndexItem a { text-decoration: none; color: #000; }\n"
    ".moduleIndexItem a:hover { text-decoration: underline; color: #00f; }\n"
    ;
}

static void htmlEncode(std::string& data)
{
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(1, data[pos]);  break;
        }
    }
    data.swap(buffer);
}

static void emitInlineDoc(std::ostream &o, const std::string &str)
{
    if (str.empty())
        return;
    std::istringstream lines(str);
    std::string line;
    o << "<div class='inlinedoc'>\n";
    while (getline(lines, line )) {
        o << "<p class='inlinedocLine'>" << line << "</p>\n";
    }
    o << "</div>\n";
}

static void emitHtmlOverload(std::ostream &o, DocState *state, DocObject *item)
{
    clay::OverloadPtr overload = (clay::Overload *)item->item.ptr();
    std::string htmlName(item->name);
    htmlEncode(htmlName);
    o << "<div class='overload'> <h3> ";
    clay::CodePtr code = overload->code;

    if (!!code->predicate) {
        std::string pred = code->predicate->asString();
        htmlEncode(pred);
        o << "<span class='functionPredicate'>[ " << pred<< " ]</span> ";
    }


    if (state->references[item->name])
        o << "<a class='reference' href='" << state->references[item->name]->fqn << ".html#" << htmlName << "'> ";
    else
        o << "<span class='brokenreference'>";

    o << "<span class='keyword'> overload </span>";

    o << "<span class='identifier'>";
    o << htmlName;
    o << "</span>";

    if (state->references[item->name])
         o << "</a>";
    else
        o << "</span>";


    o << " ( ";
    o << "<span class='functionSignature'>\n";
    for (vector<FormalArgPtr>::iterator it = code->formalArgs.begin();
            it != code->formalArgs.end(); it++){
        FormalArgPtr arg = *it;
        if (!!arg->name) {
            std::string name = identifierString(arg->name);
            htmlEncode(name);
            o << "    <span class='functionArgumentName' > " << name << "</span>";
        }
        if (!!arg->type) {
            std::string type = arg->type->asString();
            htmlEncode(type);
            o << " : ";
            o << "<span class='functionArgumentType' > " << type << "</span>\n";
        }
        if (it + 1 != code->formalArgs.end())
            o << " , ";
    }

    o << " </span> ) </span> </h3>";

    emitInlineDoc(o, item->description);
    o << "</div>" << endl;
}

static void emitHtmlProcedure(std::ostream &o, DocState *state, DocObject *item)
{
    std::string htmlName(item->name);
    htmlEncode(htmlName);
    o << "<div class='definition'> "
      << "<h3>"
      << "<a name='" << htmlName << "'>"
      << "<span class='keyword'> public </span>"
      << "<span class='identifier'>" << htmlName << "</span>  </a> </h3>";
    emitInlineDoc(o, item->description);
    o << "</div>" << endl;
}

static void emitHtmlRecord(std::ostream &o, DocState *state, DocObject *item)
{
    std::string htmlName(item->name);
    htmlEncode(htmlName);
    o << "<div class='record'> "
      << "<h3>"
      << "<a name='" << htmlName << "'>"
      << "<span class='keyword'> record </span>"
      << "<span class='identifier'>" << htmlName << "</span>  </a> </h3>";
    emitInlineDoc(o, item->description);
    o << "</div>" << endl;
}


static void emitHtmlHeader(std::ostream &o, std::string title)
{
    o << "<!doctype html>\n"
      << "<html lang='en'>\n"
      << "<head>\n"
      << "<title>" << title << "</title>\n"
      << "<style>\n" << getCssStyle() << "</style>\n"
      << "</head><body>"
      << "<div id='navigation'><ul>\n"
      << "  <li><a href='index.html'> Module Index </a> </li>\n"
      << "</ul><div class='post-ul'></div></div> \n"
      << "<div id='main'> \n"
      ;
}
static void emitHtmlFooter(std::ostream &o)
{
    o << "\n</div>\n"
      << "\n</body>\n</html>\n";
}

void emitHtmlModule(std::string outpath, DocState *state, DocModule *mod)
{
    ofstream o;
    std::string outFileName = outpath + "/" + string(mod->fqn) + ".html";
    o.open (outFileName.c_str(), ios::trunc);
    if (!o.is_open()) {
        llvm::errs() << "failed to open " << outFileName << "\n";
        exit(errno);
    }
    emitHtmlHeader(o, mod->fqn);
    o << "<div id='mainContentOuter'> <div id='mainContentHeader'>   \n<h1>" << mod->fqn << "</h1> \n";
    emitInlineDoc(o, mod->description);
    o << "</div><div id='mainContentInner'>";

    for (std::vector<DocSection*>::iterator it = mod->sections.begin();
            it != mod->sections.end(); it++) {
        o << "<section>" << endl;
        if (!(*it)->name.empty())
            o << "<h2> " << (*it)->name << "</h2>" << endl;
        emitInlineDoc(o, (*it)->description);
        for (std::vector<DocObject *>::iterator i2 = (*it)->objects.begin();
                    i2 != (*it)->objects.end(); i2++) {
            switch ((*i2)->item->objKind) {
                case clay::PROCEDURE:
                    emitHtmlProcedure(o, state, *i2);
                    break;
                case clay::RECORD_DECL:
                    emitHtmlRecord(o, state, *i2);
                    break;
                case clay::OVERLOAD:
                    emitHtmlOverload(o, state, *i2);
                    break;
                default: {} // make compiler happy
            }

        }
        o << "</section>" << endl;
    }

    o << "</div></div>" << endl;

    emitHtmlFooter(o);
    o.close();
}



void emitHtmlIndex(std::string outpath, DocState *state)
{
    ofstream index;
    index.open ((outpath + "/index.html").c_str(), ios::trunc);
    if (!index.is_open()) {
        llvm::errs() << "failed to open " << (outpath + "/index.html")<< "\n";
        exit(errno);
    }
    emitHtmlHeader(index, state->name);
    index << "<div id='mainContentOuter'>\n<h1> " << state->name << "</h1> \n";
    index << "<div id='mainContentInner'>\n";

    char a = 0;

    for (std::map<std::string, DocModule*>::iterator j = state->modules.begin();
            j != state->modules.end(); j++) {

        if (j->first.empty())
            continue;

        if (j->first.at(0) != a ) {
            if (a != 0)
                index << "</ul></section>";
            a = j->first.at(0);
            index << "<section class='moduleIndexSection' ><h2 class='moduleIndexSectionHeader'>" << a << "</h2> <ul>";
        }

        index << "<li class='moduleIndexItem'> <a href='" << string(j->first) + ".html" << "'> " << string(j->first) << " </a> </li>\n";
        emitHtmlModule(outpath, state, j->second);
    }

    index << "</ul></section>";
    index << "</div></div>" << endl;


    index.close();
}
