%%
%% This is file `jresnistdoi01.bst',
%% generated with the docstrip utility.
%%
%% The original source files were:
%%
%% merlin.mbs  (with options: `seq-no,vonx,nm-rvx,ed-rev,jnrlst,mcite,mct-1,mct-x5,dt-beg,yr-par,xmth,yrp-x,jxper,pp-last,num-xser,numser,ser-vol,ser-ed,jnm-x,pub-par,pre-pub,doi,edparxc,edby,edbyx,blk-com,com-blank,blknt,in-x,pp,ed,ord,jabr,xand,etal-xc,revdata,eprint,url,url-blk,nfss,')
%% ----------------------------------------
%% *** BibTeX style for J Res NIST, with DOI entries ***
%% 
%% Copyright 1994-2011 Patrick W Daly
% ===============================================================
% IMPORTANT NOTICE:
% This bibliographic style (bst) file has been generated from one or
% more master bibliographic style (mbs) files, listed above.
%
% This generated file can be redistributed and/or modified under the terms
% of the LaTeX Project Public License Distributed from CTAN
% archives in directory macros/latex/base/lppl.txt; either
% version 1 of the License, or any later version.
% ===============================================================
% Name and version information of the main mbs file:
% \ProvidesFile{merlin.mbs}[2011/11/18 4.33 (PWD, AO, DPC)]
%   For use with BibTeX version 0.99a or later
%-------------------------------------------------------------------
% This bibliography style file is intended for texts in ENGLISH
% This is a numerical citation style, and as such is standard LaTeX.
% It requires no extra package to interface to the main text.
% The form of the \bibitem entries is
%   \bibitem{key}...
% Usage of \cite is as follows:
%   \cite{key} ==>>          [#]
%   \cite[chap. 2]{key} ==>> [#, chap. 2]
% where # is a number determined by the ordering in the reference list.
% The order in the reference list is that by which the works were originally
%   cited in the text, or that in the database.
%---------------------------------------------------------------------

ENTRY
{ address
archive
author
booktitle
chapter
collaboration
doi
edition
editor
eid
eprint
howpublished
institution
journal
key
month
note
number
numpages
organization
pages
publisher
school
series
title
type
url
volume
year
}
{}
{ label }
INTEGERS { output.state before.all mid.sentence after.sentence after.block }
FUNCTION {init.state.consts}
{ #0 'before.all :=
#1 'mid.sentence :=
#2 'after.sentence :=
#3 'after.block :=
}
STRINGS { s t}
FUNCTION {output.nonnull}
{ 's :=
output.state mid.sentence =
{ " " * write$ }
{ output.state after.block =
{ add.period$ write$
newline$
"\newblock " write$
}
{ output.state before.all =
'write$
{ add.period$ " " * write$ }
if$
}
if$
mid.sentence 'output.state :=
}
if$
s
}
FUNCTION {output}
{ duplicate$ empty$
'pop$
'output.nonnull
if$
}
FUNCTION {output.check}
{ 't :=
duplicate$ empty$
{ pop$ "empty " t * " in " * cite$ * warning$ }
'output.nonnull
if$
}
FUNCTION {fin.entry}
{ doi empty$
{ add.period$ }
{ }
if$
write$
newline$
}

FUNCTION {new.block}
{ output.state before.all =
'skip$
{ after.block 'output.state := }
if$
}
FUNCTION {new.sentence}
{ output.state after.block =
'skip$
{ output.state before.all =
'skip$
{ after.sentence 'output.state := }
if$
}
if$
}
FUNCTION {add.blank}
{  " " * before.all 'output.state :=
}

FUNCTION {date.block}
{
add.blank
}

FUNCTION {not}
{   { #0 }
{ #1 }
if$
}
FUNCTION {and}
{   'skip$
{ pop$ #0 }
if$
}
FUNCTION {or}
{   { pop$ #1 }
'skip$
if$
}
STRINGS {z}

FUNCTION {remove.dots}
{ 'z :=
""
{ z empty$ not }
{ z #1 #2 substring$
duplicate$ "\." =
{ z #3 global.max$ substring$ 'z :=  * }
{ pop$
z #1 #1 substring$
z #2 global.max$ substring$ 'z :=
duplicate$ "." = 'pop$
{ * }
if$
}
if$
}
while$
}
FUNCTION {new.block.checka}
{ empty$
'skip$
'new.block
if$
}
FUNCTION {new.block.checkb}
{ empty$
swap$ empty$
and
'skip$
'new.block
if$
}
FUNCTION {new.sentence.checka}
{ empty$
'skip$
'new.sentence
if$
}
FUNCTION {new.sentence.checkb}
{ empty$
swap$ empty$
and
'skip$
'new.sentence
if$
}
FUNCTION {field.or.null}
{ duplicate$ empty$
{ pop$ "" }
'skip$
if$
}
FUNCTION {emphasize}
{ duplicate$ empty$
{ pop$ "" }
{ "\emph{" swap$ * "}" * }
if$
}
FUNCTION {tie.or.space.prefix}
{ duplicate$ text.length$ #3 <
{ "~" }
{ " " }
if$
swap$
}

FUNCTION {capitalize}
{ "u" change.case$ "t" change.case$ }

FUNCTION {space.word}
{ " " swap$ * " " * }
% Here are the language-specific definitions for explicit words.
% Each function has a name bbl.xxx where xxx is the English word.
% The language selected here is ENGLISH
FUNCTION {bbl.and}
{ "and"}

FUNCTION {bbl.etal}
{ "et~al." }

FUNCTION {bbl.editors}
{ "eds" }

FUNCTION {bbl.editor}
{ "ed" }

FUNCTION {bbl.edby}
{ "edited by" }

FUNCTION {bbl.edition}
{ "Ed." }

FUNCTION {bbl.volume}
{ "Vol." }

FUNCTION {bbl.of}
{ "of" }

FUNCTION {bbl.number}
{ "number" }

FUNCTION {bbl.nr}
{ "no." }

FUNCTION {bbl.in}
{ "in" }

FUNCTION {bbl.pages}
{ "pp" }

FUNCTION {bbl.page}
{ "p" }

FUNCTION {bbl.eidpp}
{ "pages" }

FUNCTION {bbl.chapter}
{ "Chapter" }

FUNCTION {bbl.techrep}
{ "" }

FUNCTION {bbl.patent}
{ "Patent" }

FUNCTION {bbl.mthesis}
{ "Master's thesis" }

FUNCTION {bbl.phdthesis}
{ "Ph.D. thesis." }

FUNCTION {bbl.first}
{ "1st" }

FUNCTION {bbl.second}
{ "2nd" }

FUNCTION {bbl.third}
{ "3rd" }

FUNCTION {bbl.fourth}
{ "4th" }

FUNCTION {bbl.fifth}
{ "5th" }

FUNCTION {bbl.st}
{ "st" }

FUNCTION {bbl.nd}
{ "nd" }

FUNCTION {bbl.rd}
{ "rd" }

FUNCTION {bbl.th}
{ "th" }

MACRO {jan} {"January"}

MACRO {feb} {"February"}

MACRO {mar} {"March"}

MACRO {apr} {"April"}

MACRO {may} {"May"}

MACRO {jun} {"June"}

MACRO {jul} {"July"}

MACRO {aug} {"August"}

MACRO {sep} {"September"}

MACRO {oct} {"October"}

MACRO {nov} {"November"}

MACRO {dec} {"December"}

FUNCTION {eng.ord}
{ duplicate$ "1" swap$ *
#-2 #1 substring$ "1" =
{ bbl.th * }
{ duplicate$ #-1 #1 substring$
duplicate$ "1" =
{ pop$ bbl.st * }
{ duplicate$ "2" =
{ pop$ bbl.nd * }
{ "3" =
{ bbl.rd * }
{ bbl.th * }
if$
}
if$
}
if$
}
if$
}

MACRO {acmcs} {"ACM Comput. Surv."}

MACRO {acta} {"Acta Inf."}

MACRO {cacm} {"Commun. ACM"}

MACRO {ibmjrd} {"IBM J. Res. Dev."}

MACRO {ibmsj} {"IBM Syst.~J."}

MACRO {ieeese} {"IEEE Trans. Software Eng."}

MACRO {ieeetc} {"IEEE Trans. Comput."}

MACRO {ieeetcad}
{"IEEE Trans. Comput. Aid. Des."}

MACRO {ipl} {"Inf. Process. Lett."}

MACRO {jacm} {"J.~ACM"}

MACRO {jcss} {"J.~Comput. Syst. Sci."}

MACRO {scp} {"Sci. Comput. Program."}

MACRO {sicomp} {"SIAM J. Comput."}

MACRO {tocs} {"ACM Trans. Comput. Syst."}

MACRO {tods} {"ACM Trans. Database Syst."}

MACRO {tog} {"ACM Trans. Graphic."}

MACRO {toms} {"ACM Trans. Math. Software"}

MACRO {toois} {"ACM Trans. Office Inf. Syst."}

MACRO {toplas} {"ACM Trans. Progr. Lang. Syst."}

MACRO {tcs} {"Theor. Comput. Sci."}

FUNCTION {bibinfo.check}
{ swap$
duplicate$ missing$
{
pop$ pop$
""
}
{ duplicate$ empty$
{
swap$ pop$
}
{ swap$
pop$
}
if$
}
if$
}
FUNCTION {bibinfo.warn}
{ swap$
duplicate$ missing$
{
swap$ "missing " swap$ * " in " * cite$ * warning$ pop$
""
}
{ duplicate$ empty$
{
swap$ "empty " swap$ * " in " * cite$ * warning$
}
{ swap$
pop$
}
if$
}
if$
}
FUNCTION {format.eprint}
{ eprint duplicate$ empty$
'skip$
{ "\eprint"
archive empty$
'skip$
{ "[" * archive * "]" * }
if$
"{" * swap$ * "}" *
}
if$
}
FUNCTION {format.url}
{
url
duplicate$ empty$
{ pop$ "" }
{ "\urlprefix\url{" swap$ * "}" * }
if$
}

INTEGERS { nameptr namesleft numnames }


STRINGS  { bibinfo}

FUNCTION {format.names}
{ 'bibinfo :=
duplicate$ empty$ 'skip$ {
's :=
"" 't :=
#1 'nameptr :=
s num.names$ 'numnames :=
numnames 'namesleft :=
{ namesleft #0 > }
{ s nameptr
"{vv~}{ll}{ f{}}{ jj}"
format.name$
remove.dots
bibinfo bibinfo.check
't :=
nameptr #1 >
{
nameptr #1
#1 + =
numnames #99
> and
{ "others" 't :=
#1 'namesleft := }
'skip$
if$
namesleft #1 >
{ ", " * t * }
{
s nameptr "{ll}" format.name$ duplicate$ "others" =
{ 't := }
{ pop$ }
if$
"," *
t "others" =
{
" " * bbl.etal *
}
{ " " * t * }
if$
}
if$
}
't
if$
nameptr #1 + 'nameptr :=
namesleft #1 - 'namesleft :=
}
while$
} if$
}
FUNCTION {format.names.ed}
{
format.names
}
FUNCTION {format.authors}
{ author "author" format.names
duplicate$ empty$ 'skip$
{ collaboration "collaboration" bibinfo.check
duplicate$ empty$ 'skip$
{ " (" swap$ * ")" * }
if$
*
}
if$
}
FUNCTION {get.bbl.editor}
{ editor num.names$ #1 > 'bbl.editors 'bbl.editor if$ }

FUNCTION {format.editors}
{ editor "editor" format.names duplicate$ empty$ 'skip$
{
" " *
get.bbl.editor
"(" swap$ * ")" *
*
}
if$
}
FUNCTION {doilink}
{ duplicate$ empty$
{ pop$ "" }
{doi empty$
{ skip$ }
{ "\href{https://doi.org/" doi * "}{" * swap$ * "}" * }
if$
}
if$
}
FUNCTION {format.doi}
{ doi empty$
{ "" }
{
"\doi{" doi * "}" *
}
if$
doilink
}
FUNCTION {format.note}
{
note empty$
{ "" }
{ note #1 #1 substring$
duplicate$ "{" =
'skip$
{ output.state mid.sentence =
{ "l" }
{ "u" }
if$
change.case$
}
if$
note #2 global.max$ substring$ * "note" bibinfo.check
}
if$
}

FUNCTION {format.title}
{ title
duplicate$ empty$ 'skip$
{ "t" change.case$ }
if$
"title" bibinfo.check
}
FUNCTION {output.bibitem}
{ newline$
"\bibitem{" write$
cite$ write$
"}" write$
newline$
""
before.all 'output.state :=
}

FUNCTION {n.dashify}
{
't :=
""
{ t empty$ not }
{ t #1 #1 substring$ "-" =
{ t #1 #2 substring$ "--" = not
{ "--" *
t #2 global.max$ substring$ 't :=
}
{   { t #1 #1 substring$ "-" = }
{ "-" *
t #2 global.max$ substring$ 't :=
}
while$
}
if$
}
{ t #1 #1 substring$ *
t #2 global.max$ substring$ 't :=
}
if$
}
while$
}

FUNCTION {word.in}
{ "" }

FUNCTION {format.date}
{
""
duplicate$ empty$
year  "year"  bibinfo.check duplicate$ empty$
{ swap$ 'skip$
{ "there's a month but no year in " cite$ * warning$ }
if$
*
}
{ swap$ 'skip$
{
swap$
" " * swap$
}
if$
*
}
if$
duplicate$ empty$
'skip$
{
before.all 'output.state :=
" (" swap$ * ")" *
}
if$
}
FUNCTION {format.btitle}
{ title "title" bibinfo.check
duplicate$ empty$ 'skip$
{
emphasize
}
if$
}
FUNCTION {either.or.check}
{ empty$
'pop$
{ "can't use both " swap$ * " fields in " * cite$ * warning$ }
if$
}
FUNCTION {format.bvolume}
{ volume empty$
{ "" }
{ bbl.volume volume tie.or.space.prefix
"volume" bibinfo.check * *
series "series" bibinfo.check
duplicate$ empty$ 'pop$
{ emphasize ", " * swap$ * }
if$
"volume and number" number either.or.check
}
if$
}
FUNCTION {format.number.series}
{ volume empty$
{ number empty$
{ series field.or.null }
{ series empty$
{ number "number" bibinfo.check }
{ output.state mid.sentence =
{ bbl.number }
{ bbl.number capitalize }
if$
number tie.or.space.prefix "number" bibinfo.check * *
bbl.in space.word *
series "series" bibinfo.check *
}
if$
}
if$
}
{ "" }
if$
}
FUNCTION {is.num}
{ chr.to.int$
duplicate$ "0" chr.to.int$ < not
swap$ "9" chr.to.int$ > not and
}

FUNCTION {extract.num}
{ duplicate$ 't :=
"" 's :=
{ t empty$ not }
{ t #1 #1 substring$
t #2 global.max$ substring$ 't :=
duplicate$ is.num
{ s swap$ * 's := }
{ pop$ "" 't := }
if$
}
while$
s empty$
'skip$
{ pop$ s }
if$
}

FUNCTION {convert.edition}
{ extract.num "l" change.case$ 's :=
s "first" = s "1" = or
{ bbl.first 't := }
{ s "second" = s "2" = or
{ bbl.second 't := }
{ s "third" = s "3" = or
{ bbl.third 't := }
{ s "fourth" = s "4" = or
{ bbl.fourth 't := }
{ s "fifth" = s "5" = or
{ bbl.fifth 't := }
{ s #1 #1 substring$ is.num
{ s eng.ord 't := }
{ edition 't := }
if$
}
if$
}
if$
}
if$
}
if$
}
if$
t
}

FUNCTION {format.edition}
{ edition duplicate$ empty$ 'skip$
{
convert.edition
output.state mid.sentence =
{ "l" }
{ "t" }
if$ change.case$
"edition" bibinfo.check
" " * bbl.edition *
}
if$
}
INTEGERS { multiresult }
FUNCTION {multi.page.check}
{ 't :=
#0 'multiresult :=
{ multiresult not
t empty$ not
and
}
{ t #1 #1 substring$
duplicate$ "-" =
swap$ duplicate$ "," =
swap$ "+" =
or or
{ #1 'multiresult := }
{ t #2 global.max$ substring$ 't := }
if$
}
while$
multiresult
}
FUNCTION {format.pages}
{ pages duplicate$ empty$ 'skip$
{ duplicate$ multi.page.check
{
bbl.pages swap$
n.dashify
}
{
bbl.page swap$
}
if$
tie.or.space.prefix
"pages" bibinfo.check
* *
}
if$
}
FUNCTION {format.journal.pages}
{ pages duplicate$ empty$ 'pop$
{ swap$ duplicate$ empty$
{ pop$ pop$ format.pages }
{
":" *
swap$
n.dashify
"pages" bibinfo.check
*
}
if$
}
if$
}
FUNCTION {format.journal.eid}
{ eid "eid" bibinfo.check
duplicate$ empty$ 'pop$
{ swap$ duplicate$ empty$ 'skip$
{
":" *
}
if$
swap$ *
numpages empty$ 'skip$
{ bbl.eidpp numpages tie.or.space.prefix
"numpages" bibinfo.check * *
" (" swap$ * ")" * *
}
if$
}
if$
}
FUNCTION {format.vol.num.pages}
{ volume field.or.null
duplicate$ empty$ 'skip$
{
"volume" bibinfo.check
}
if$
number "number" bibinfo.check duplicate$ empty$ 'skip$
{
swap$ duplicate$ empty$
{ "there's a number but no volume in " cite$ * warning$ }
'skip$
if$
swap$
"(" swap$ * ")" *
}
if$ *
}

FUNCTION {format.chapter.pages}
{ chapter empty$
{ "" }
{ type empty$
{ bbl.chapter }
{ type "l" change.case$
"type" bibinfo.check
}
if$
chapter tie.or.space.prefix
"chapter" bibinfo.check
* *
}
if$
}

FUNCTION {format.booktitle}
{
booktitle "booktitle" bibinfo.check
emphasize
}
FUNCTION {format.in.ed.booktitle}
{ format.booktitle duplicate$ empty$ 'skip$
    {
    editor "editor" format.names.ed duplicate$ empty$ 'pop$
        {
        get.bbl.editor
        " " * swap$ *
        swap$
        "," *
        " " * swap$
        * }
    if$
word.in swap$ *
}
if$
}

FUNCTION {empty.misc.check}
{ author empty$ title empty$ howpublished empty$
month empty$ year empty$ note empty$
and and and and and
{ "all relevant fields are empty in " cite$ * warning$ }
'skip$
if$
}
FUNCTION {format.thesis.type}
{ type duplicate$ empty$
'pop$
{ swap$ pop$
"t" change.case$ "type" bibinfo.check
}
if$
}
FUNCTION {format.tr.number}
{ number "number" bibinfo.check
type duplicate$ empty$
{ pop$ bbl.techrep }
'skip$
if$
"type" bibinfo.check
swap$ duplicate$ empty$
{ pop$ "t" change.case$ }
{ tie.or.space.prefix * * }
if$
}
FUNCTION {format.pat.number}
{ number "number" bibinfo.check
type duplicate$ empty$
{ pop$ bbl.patent }
'skip$
if$
"type" bibinfo.check
swap$ duplicate$ empty$
{ pop$ "t" change.case$ }
{ tie.or.space.prefix * * }
if$
}
FUNCTION {format.article.crossref}
{
key duplicate$ empty$
{ pop$
journal duplicate$ empty$
{ "need key or journal for " cite$ * " to crossref " * crossref * warning$ }
{ "journal" bibinfo.check emphasize word.in swap$ * }
if$
}
{ word.in swap$ * " " *}
if$
" \cite{" * crossref * "}" *
}
FUNCTION {format.crossref.editor}
{ editor #1 "{vv~}{ll}" format.name$
"editor" bibinfo.check
editor num.names$ duplicate$
#2 >
{ pop$
"editor" bibinfo.check
" " * bbl.etal
*
}
{ #2 <
'skip$
{ editor #2 "{ff }{vv }{ll}{ jj}" format.name$ "others" =
{
"editor" bibinfo.check
" " * bbl.etal
*
}
{
bbl.and space.word
* editor #2 "{vv~}{ll}" format.name$
"editor" bibinfo.check
*
}
if$
}
if$
}
if$
}
FUNCTION {format.book.crossref}
{ volume duplicate$ empty$
{ "empty volume in " cite$ * "'s crossref of " * crossref * warning$
pop$ word.in
}
{ bbl.volume
swap$ tie.or.space.prefix "volume" bibinfo.check * * bbl.of space.word *
}
if$
editor empty$
editor field.or.null author field.or.null =
or
{ key empty$
{ series empty$
{ "need editor, key, or series for " cite$ * " to crossref " *
crossref * warning$
"" *
}
{ series emphasize * }
if$
}
{ key * }
if$
}
{ format.crossref.editor * }
if$
" \cite{" * crossref * "}" *
}
FUNCTION {format.incoll.inproc.crossref}
{
editor empty$
editor field.or.null author field.or.null =
or
{ key empty$
{ format.booktitle duplicate$ empty$
{ "need editor, key, or booktitle for " cite$ * " to crossref " *
crossref * warning$
}
{ word.in swap$ * }
if$
}
{ word.in key * " " *}
if$
}
{ word.in format.crossref.editor * " " *}
if$
" \cite{" * crossref * "}" *
}
FUNCTION {format.org.or.pub}
{ 't :=
""
address empty$ t empty$ and
'skip$
{
add.blank "(" *
t empty$
{ address "address" bibinfo.check *
}
{ t *
address empty$
'skip$
{ ", " * address "address" bibinfo.check * }
if$
}
if$
")" *
}
if$
}
FUNCTION {format.publisher.address}
{ publisher "publisher" bibinfo.warn format.org.or.pub
}

FUNCTION {format.organization.address}
{ organization "organization" bibinfo.check format.org.or.pub
}

FUNCTION {format.institution.address}
{ institution "institution" bibinfo.check format.org.or.pub
}

FUNCTION {article}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.title "title" output.check
new.sentence
crossref missing$
{
journal
remove.dots
"journal" bibinfo.check
emphasize
"journal" output.check
add.blank
format.vol.num.pages output
}
{ format.article.crossref output.nonnull
}
if$
eid empty$
{ format.journal.pages }
{ format.journal.eid }
if$
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}
FUNCTION {book}
{ output.bibitem
author empty$
{ format.editors "author and editor" output.check
add.blank
}
{ format.authors output.nonnull
crossref missing$
{ "author and editor" editor either.or.check }
'skip$
if$
}
if$
format.date "year" output.check
date.block
format.btitle "title" output.check
new.sentence
crossref missing$
{ format.bvolume output
format.number.series output
format.publisher.address output
}
{
format.book.crossref output.nonnull
}
if$
", " * format.edition output
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}
FUNCTION {booklet}
{ output.bibitem
format.authors output
format.date output
date.block
format.title "title" output.check
new.sentence
howpublished "howpublished" bibinfo.check output
", " * address "address" bibinfo.check output
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {inbook}
{ output.bibitem
author empty$
{ format.editors "author and editor" output.check
}
{ format.authors output.nonnull
crossref missing$
{ "author and editor" editor either.or.check }
'skip$
if$
}
if$
format.date "year" output.check
date.block
format.btitle "title" output.check
new.sentence
crossref missing$
{
format.number.series output
format.publisher.address output
", " * format.bvolume output
", " * format.chapter.pages "chapter and pages" output.check
}
{
format.chapter.pages "chapter and pages" output.check
format.book.crossref output.nonnull
}
if$
", " * format.edition output
", " * format.pages "pages" output.check
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {incollection}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.title "title" output.check
new.sentence
crossref missing$
{ format.in.ed.booktitle "booktitle" output.check
format.number.series output
publisher empty$
{ format.organization.address output }
{ organization "organization" bibinfo.check output
format.publisher.address output
}
if$
", " * format.bvolume output
}
{ format.incoll.inproc.crossref output.nonnull
}
if$
", " * format.pages "pages" output.check
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}
FUNCTION {inproceedings}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.title "title" output.check
new.sentence
crossref missing$
{ format.in.ed.booktitle "booktitle" output.check
format.number.series output
publisher empty$
{ format.organization.address output }
{ organization "organization" bibinfo.check output
format.publisher.address output
}
if$
", " * format.bvolume output
}
{ format.incoll.inproc.crossref output.nonnull
}
if$
", " * format.pages "pages" output.check
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}
FUNCTION {conference} { inproceedings }
FUNCTION {manual}
{ output.bibitem
author empty$
{ organization "organization" bibinfo.check
duplicate$ empty$ 'pop$
{ output
address "address" bibinfo.check output
}
if$
}
{ format.authors output.nonnull }
if$
format.date output
date.block
format.btitle "title" output.check
author empty$
{ organization empty$
{
address "address" bibinfo.check output
}
'skip$
if$
}
{
organization "organization" bibinfo.check output
address "address" bibinfo.check output
}
if$
", " * format.edition output
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {mastersthesis}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.btitle
"title" output.check
bbl.mthesis format.thesis.type output.nonnull
school "school" bibinfo.warn output
address "address" bibinfo.check output
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {misc}
{ output.bibitem
format.authors output
format.date output
format.title output
howpublished empty$
'skip$
{", " * howpublished "howpublished" bibinfo.check output}
if$
new.sentence
format.doi output
"." * 
format.note output
format.eprint output
format.url output
fin.entry
empty.misc.check
}
FUNCTION {phdthesis}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.btitle
"title" output.check
new.sentence
bbl.phdthesis format.thesis.type output.nonnull
school "school" bibinfo.warn output
", " * address "address" bibinfo.check output
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {proceedings}
{ output.bibitem
editor empty$
{ organization "organization" bibinfo.check output
}
{ format.editors output.nonnull }
if$
format.date "year" output.check
date.block
format.btitle "title" output.check
new.sentence
", " 
editor empty$
{ publisher empty$
{  format.number.series output }
{

format.publisher.address output
}
if$
}
{ publisher empty$
{
format.number.series output
format.organization.address output }
{
format.number.series output
organization "organization" bibinfo.check output
format.publisher.address output
}
if$
}
if$
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {techreport}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.title
"title" output.check
new.sentence
format.institution.address output
", " * format.tr.number output.nonnull
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {patent}
{ output.bibitem
format.authors "author" output.check
format.date "year" output.check
date.block
format.title
"title" output.check
new.sentence
institution "institution" bibinfo.warn output
address "address" bibinfo.check output
" " * format.pat.number output.nonnull
new.sentence
format.doi output
new.sentence
format.note output
format.eprint output
format.url output
fin.entry
}

FUNCTION {unpublished}
{ output.bibitem
format.authors "author" output.check
format.date output
date.block
format.title "title" output.check
new.sentence
format.doi output
new.sentence
format.note "note" output.check
format.eprint output
format.url output
fin.entry
}



FUNCTION {default.type} { misc }
READ
STRINGS { longest.label }
INTEGERS { number.label longest.label.width }
FUNCTION {initialize.longest.label}
{ "" 'longest.label :=
#1 'number.label :=
#0 'longest.label.width :=
}
FUNCTION {longest.label.pass}
{ number.label int.to.str$ 'label :=
number.label #1 + 'number.label :=
label width$ longest.label.width >
{ label 'longest.label :=
label width$ 'longest.label.width :=
}
'skip$
if$
}
EXECUTE {initialize.longest.label}
ITERATE {longest.label.pass}
FUNCTION {begin.bib}
{ preamble$ empty$
'skip$
{ preamble$ write$ newline$ }
if$
"\begin{thebibliography}{"  longest.label  * "}" *
write$ newline$
"\providecommand{\url}[1]{\texttt{#1}}"
write$ newline$
"\providecommand{\urlprefix}{Available at }"
write$ newline$
"\expandafter\ifx\csname urlstyle\endcsname\relax"
write$ newline$
"  \providecommand{\doi}[1]{https://doi.org/\discretionary{}{}{}#1}\else"
write$ newline$
"  \providecommand{\doi}{https://doi.org/\discretionary{}{}{}\begingroup \urlstyle{rm}\Url}\fi"
write$ newline$
"\providecommand{\eprint}[2][]{\url{#2}}"
write$ newline$
}
EXECUTE {begin.bib}
EXECUTE {init.state.consts}
ITERATE {call.type$}
FUNCTION {end.bib}
{ newline$
"\end{thebibliography}" write$ newline$
}
EXECUTE {end.bib}
%% End of customized bst file
%%
%% End of file `jresnistdoi01.bst'.
