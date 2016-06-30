/* Copyright 1990-2006, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Words: Control Words                                                    */

#include "j.h"
#include "w.h"

// CTESTB         (CIF+CELSEIF+CSELECT+CSELECTN+CWHILE+CWHILST+CFOR+CCASE+CFCASE)
#define CWCASE(x,y)    (x+256*y)
#define CWASSERT(b)    if(!(b))R i


static A jtcongotoblk(J jt,I n,CW*con){A z;CW*d=con;I i,j,k,*u,*v;
 GA(z,INT,2*n,2,0); v=AS(z); v[0]=n; v[1]=2;
 u=v=AV(z);
 for(i=j=0;i<n;++i,++d){
  *u++=-1; *u++=-1; 
  switch(d->type){
   case CEND: 
    v[k]=i; while(0<k&&0<v[k])k-=2; break;
   case CCASE: case CCATCH: case CDO: case CELSE: case CELSEIF: case CFCASE:
    v[k]=i;  /* fall thru */
   case CFOR: case CIF: case CSELECT: case CTRY: case CWHILE: case CWHILST:
    v[j]=i; k=1+j; j+=2;
 }}
 R z;
}    /* compute blocks for goto checking */

static I jtcongotochk(J jt,I i,I j,A x){I k,n,*v;
 n=*AS(x); v=AV(x);
 for(k=0;k<n;++k,v+=2)if(v[0]<=j&&j<=v[1]&&!(v[0]<=i&&i<=v[1]))R i;
 R -1;
}    /* i: goto; j: label; return -1 if ok or i if bad */

#define LABELEQU(m,s,e)    (CLABEL==e->type&&(x=lv[e->i],!memcmp(s,6+CAV(x),m)))

static I jtcongoto(J jt,I n,CW*con,A*lv){A x,z;C*s;CW*d=con,*e;I i,j,k,m;
 RZ(z=congotoblk(n,con));
 for(i=0;i<n;++i,++d)
  if(CGOTO==d->type){
   x=lv[d->i]; s=5+CAV(x); m=0; while('.'!=s[m])++m; ++m;
   e=con-1; j=-1;
   DO(n, ++e; if(LABELEQU(m,s,e)){j=1+i; d->go=(US)j; break;});
   CWASSERT(0<=j);
   e=con+j-1;
   for(k=j;k<n;++k){++e; if(LABELEQU(m,s,e)){i=k; CWASSERT(0);}}
   if(0<=congotochk(i,j-1,z))R i;
  }
 R -1;
}    /* same result as conall */


/* conend - end encountered (but select end handled separately) */
/* same result as conall                                        */
/* b i p  end.                                                  */
/* c j q  1st word before end.                                  */
/* d k r  2nd word before end.                                  */
/* each triplet (b,i,p) is ptr, stack index, code (ptr->type)   */

// Process end. not associated with select. or try.
static I conend(I i,I j,I k,CW*b,CW*c,CW*d,I p,I q,I r){I e,m,t;
 e=1+i;    // set e to Next Sequential Instruction
 CWASSERT(c); c->go=(US)e;   // Set previous control to come here.  This could be the do. if if./do. or loop/do., or the else. of else./do.
 switch(CWCASE(r,q)){    // look at the stack-types of the antepenultimate and penultimate entries
  default:                   CWASSERT(0);  // if not known combination, give control error
  case CWCASE(CDO,CELSE):                  // if. ...do. ... end.  or do. ... else. ... end. - leave as is
  case CWCASE(CIF,CDO):      break;
  case CWCASE(CELSEIF,CDO):  CWASSERT(d); d->go=(US)e; break;  // if elseif./do./end., set elseif. to come to end.
  case CWCASE(CWHILST,CDO):  CWASSERT(d); d->go=(US)(1+j);   // set whilst. to start at end, and fall through to...
  case CWCASE(CWHILE,CDO):                                    // ...while. ... do.
   // Set the end. to point back to the post-while., and then scan the loop looking for break./continue.
   // that has not been processed before (we detect these by d->go==SMAX; so when there are nested loops
   // we will skip over break. for inner loops, which have already been processed). 
   CWASSERT(b&&d); b->go=(US)(1+k); m=i-k-1;   // get # cws between (after while.) and (before end.)
   // fill in break. to go after end., or continue. to go after while.; leave others unchanged
   DO(m, ++d; t=d->type; if(SMAX==d->go)d->go=t==CBREAK?                 (US)e :t==CCONT?(US)(1+k):(US)SMAX;);
   break;
  case CWCASE(CFOR,CDOF):
   // for. is like while., but end. and continue. go back to the do., and break. is marked as BREAKF
   // to indicate that the for. must be popped off the execution stack
   CWASSERT(b&&d); b->go=(US)j;   m=i-k-1;
   DO(m, ++d; t=d->type; if(SMAX==d->go)d->go=t==CBREAK?(d->type=CBREAKF,(US)e):t==CCONT?(US)j    :(US)SMAX;);
 }
 R -1;
}

// Fix up the stack after encountering the end. for a try.  e=address of end.
static I conendtry(I e,I top,I stack[],CW*con){CW*v;I c[3],d[4],i=-1,j,k=0,m,t=0;US ii;
 c[0]=c[1]=c[2]=-1; d[k++]=e;
 // fill d[] with end. line# followed by catchx. numbers, in descending order of line #s.  Error if repeated catch.
 while(top&&t!=CTRY){
  j=stack[--top];
  switch(t=(j+con)->type){
   case CTRY:    break;
   case CCATCH:  CWASSERT(0>c[0]); c[0]=d[k++]=j; break;
   case CCATCHD: CWASSERT(0>c[1]); c[1]=d[k++]=j; break;
   case CCATCHT: CWASSERT(0>c[2]); c[2]=d[k++]=j; break;
   default:      CWASSERT(0);
 }}
 CWASSERT(t==CTRY&&1<k);   // verify at least one catchx.  j now points to try.
 (j+con)->go=(US)d[k-1];                         // point the try. to the first catchx.
 m=k; DO(k-1, --m; (d[m]+con)->go=(US)d[m-1];);  // point each catchx. to the next catchx., and the last one to the end.
 (e+con)->go=(US)(1+e);                          // point the end. to NSI
 m=d[k-1];                                    // line# of first catchx.
 // if there is a catch./catchd., scan the lines from after-try. to the first catchx., and point
 // any hitherto not-processed throw. to go to the catch. (if it exists) or catchd. (if no catch.).
 // That way, unfielded throw. counts as an error that can be picked up in catch.
 if     (0<=c[0]){ii=(US)(1+c[0]); v=j+con; DO(m-j-1, ++v; if(SMAX==v->go&&CBREAK!=v->type&&CCONT!=v->type)v->go=ii;);}
 else if(0<=c[1]){ii=(US)(1+c[1]); v=j+con; DO(m-j-1, ++v; if(SMAX==v->go&&CBREAK!=v->type&&CCONT!=v->type)v->go=ii;);}
 R top;  //return stack pointer with the try. ... end. removed
}    /* result is new value of top */

// Fix up the stack after encountering the end, for a select.  i=address of end.
static I conendsel(I i,I top,I stack[],CW*con){I c=i-1,d=0,j,ot=top,t;
 // Go through the stack in reverse order till we hit the select.
 // c will hold the cw one before the one to go to if the previous test fails (init to one before end.)
 while(1){
  j=stack[--top]; t=(j+con)->type;    // back up to next cw
  if(t==CSELECT||t==CSELECTN)break;                //when we hit select., we're done
  if(t==CDOSEL){d=j; (j+con)->go=(US)(1+c);}  // on do., remember line# of do. in d; point that do. to the failure position
  else{                            // must be case./fcase.
   c=j; (j+con)->go=(US)i;          // set failed-compare point to be the case. test; point case. to the end. (end-of-case goes to end.)
   if(d==1+j)(d+con)->go=(US)(1+d);    // if empty case., set do. to fall through to next inst
   if(t==CFCASE&&top<ot-2)(stack[2+top]+con)->go=(US)(1+stack[3+top]);  // if fcase. (and not last case), point case. AFTER fcase. to go to the do. for that following case.
 }}
 (c+con)->go=(US)(1+c);  // set first case. to fall through to the first test
 R top;     // return stack with select. ... end. removed
}    /* result is new value of top */

// audit the control sequence in con, making replacement in places
// n is number of control words, con points to them
static I jtconall(J jt,I n,CW*con){A y;CW*b=0,*c=0,*d=0;I e,i,j,k,p=0,q,r,*stack,tb=0,top=0,wb=0;
 // We process the words through a stack to match up control sequences.  Initialize to empty
 GA(y,INT,n,1,0); stack=AV(y);
 for(i=0;i<n;++i){
  // top is the top of the stack (i. e. the place to add the next entry)
  // b, c, d -> con entries, p, q, r are cw-types   for current cw, previous cw on stack, 2d-previous cw on stack
  // e is the next sequential instruction (following instruction i, the current line)
  // tb counts the number of unclosed if. statements.  If this is nonzero, every sentence becomes a T-block sentence
  q=r=0; e=1+i;             b=i+con; p=b->type;
  if(0<top){j=stack[top-1]; c=j+con; q=c->type;}
  if(1<top){k=stack[top-2]; d=k+con; r=d->type;}
  switch(p){
   case CBBLOCK: if(tb)b->type=CTBLOCK; break;   // In case of  if. if. x do. y end. do. end., y is NOT eligible to be the result, because
     // it is executed  in a T block; so we switch B to T inside ANY T block
   case CLABEL:  b->go=(US)e;           break;  // label goes to NSI
   case CTRY:
   case CCATCH:
   case CCATCHD:
   case CCATCHT: stack[top++]=i;        break;  // try./catch.: push address of control word
   case CCONT:
   case CBREAK:  CWASSERT(wb);          break;  // continue/break: verify in a loop
   case CFOR:
   case CWHILE:  
   case CWHILST: ++wb;                          // for./while./whilst.: increment nested-loop count, fall through to...
   case CIF:     stack[top++]=i; ++tb;  break;   // (if. too) ...push addr of cw, increment t-block count
   case CCASE:
   case CFCASE:                                 // case./fcase.
    CWASSERT(q==CSELECT||q==CSELECTN||q==CDOSEL);           // verify in select.
    stack[top++]=i; if(q==CDOSEL)++tb;        // push address of cw; if not first case, increment T-block count (for the first,
                                              // we inherit the T-block status from the select.)
    break;
   case CSELECT:                              // select. - start T-block and check select. nesting
    // The execution-time code creates a stack-frame for select. that must be popped if break. or continue. is
    // encountered.  If select. blocks are nested, all nested blocks back to the looping structure must be
    // popped.  But the execution-time stack does not create a stack-frame for while., so looking at the
    // run-time stack cannot indicate where the top-level select. is.  Instead, we must analyze it here.
    // we change a nested select. to SELECTN.  This is needed only if we are in a loop.
    if(wb) {   // analyze only if in loop
     // Go back through the stack till we hit a loop.  If we find a SELECT along the way, change this to a SELECTN
     I s;  // stack pointer
     for(s=top-1;con[stack[s]].type!=CFOR && con[stack[s]].type!=CWHILE && con[stack[s]].type!=CWHILST;--s) {
      if(con[stack[s]].type==CSELECT){b->type=CSELECTN;break;}
     } 
    }
    stack[top++]=i; ++tb;  break;            // push addr of cw, increment t-block count
   case CDO:                                   // do. - classify according to what structure we are in
    CWASSERT(testb[q]);                       // first, make sure do. is allowed here
     // classify the do. based on struct: if for. do., call it DOF; if [f]case. do, call it DOSEL;
     // otherwise (if./elseif., while./whilst.) leave it as DO
    b->type=q==CFOR?CDOF:q==CCASE||q==CFCASE?CDOSEL:CDO;
    stack[top++]=i; --tb;                    // push the line# of the DO line; and note that this ends a T block
    break;
   case CELSEIF:                             // elseif. - like end. + if.
    CWASSERT(q==CDO);                        // verify in if.  (but it could be while. ??)
    c->go=(US)e;                             // set previous do. go to instruction after elseif.
    if(r==CELSEIF)d->go=(US)i;               // if struct is elseif. .. do.  elseif., point the previous elseif. to this one
    top-=2; stack[top++]=i; ++tb;            // remove the previous if. ... do. or elseif. ... do., replace with this elseif.; and
                                             // note that the previous block was a B and we have now moved to a T
    break;
   case CELSE:                               // else.
    CWASSERT(r==CIF&&q==CDO);                // verify part of if. ... do. ... else.
    c->go=(US)e;                             // set if. to jump to NSI
    stack[top-2]=stack[top-1]; stack[top-1]=i;  // replace if. ... do. on stack with do. ... else.
    break;
   case CEND:                                // end. run a conend... routine to update pointers in the cws. q->the do., r->the starting cw of the structure
    switch(q){
     case CDOSEL:                            // if [f]case. ... do. ... end. 
      top=conendsel(i,top,stack,con); CWASSERT(0<=top); b->type=CENDSEL; break;  // end the select., and change the cw to ENDSEL
     case CCATCH: case CCATCHD: case CCATCHT:  // if a catch?.
      CWASSERT(1<=top);
      top=conendtry(i,top,stack,con); CWASSERT(0<=top);                  break;  // end the catch, verify validity
     default:                                // if if./elseif./else. or while./whilst./for. ...
      top-=2;                                // pop the starting cw and the do.
      if(r==CWHILE||r==CWHILST||r==CFOR)--wb;  // if this ends a loop, decrement the nested-loop count
      CWASSERT(0>conend(i,j,k,b,c,d,p,q,r));   // update the controls matching this end.
 }}}
 // when it's over, the stack should be empty.  If not, return the index of the top control on the stack
 R top?stack[top-1]:-1;
}    /* modifies con; return -1 if OK or index of bad con entry */

A jtspellcon(J jt,I c){
 switch(c){
  default:      ASSERTSYS(0,"spellcon");
  case CASSERT: R cstr("assert.");
  case CBBLOCK: R cstr("bblock.");
  case CBREAK:  
  case CBREAKF: R cstr("break.");
  case CCASE:   R cstr("case.");
  case CCATCH:  R cstr("catch.");
  case CCATCHD: R cstr("catchd.");
  case CCATCHT: R cstr("catcht.");
  case CCONT:   R cstr("continue.");
  case CDO:
  case CDOF:    
  case CDOSEL:  R cstr("do.");
  case CELSE:   R cstr("else.");
  case CELSEIF: R cstr("elseif.");
  case CEND:    
  case CENDSEL: R cstr("end.");
  case CFCASE:  R cstr("fcase.");
  case CFOR:    R cstr("for.");
  case CGOTO:   R cstr("goto_.");
  case CIF:     R cstr("if.");
  case CLABEL:  R cstr("label_.");
  case CRETURN: R cstr("return.");
  case CSELECTN:
  case CSELECT: R cstr("select.");
  case CTBLOCK: R cstr("tblock.");
  case CTHROW:  R cstr("throw.");
  case CTRY:    R cstr("try.");
  case CWHILE:  R cstr("while.");
  case CWHILST: R cstr("whilst.");
}}

static I jtconword(J jt,I n,C*s){
 if(2<n&&'.'==*(s+n-1))switch(*s){
  case 'a': if(!strncmp(s,"assert.",  n))R CASSERT;  break;
  case 'b': if(!strncmp(s,"break.",   n))R CBREAK;   break;
  case 'c': if(!strncmp(s,"case.",    n))R CCASE;    
            if(!strncmp(s,"continue.",n))R CCONT;
            if(!strncmp(s,"catch.",   n))R CCATCH;
            if(!strncmp(s,"catchd.",  n))R CCATCHD;
            if(!strncmp(s,"catcht.",  n))R CCATCHT;  break;
  case 'd': if(!strncmp(s,"do.",      n))R CDO;      break;
  case 'e': if(!strncmp(s,"end.",     n))R CEND;
            if(!strncmp(s,"else.",    n))R CELSE;
            if(!strncmp(s,"elseif.",  n))R CELSEIF;  break;
  case 'f': if(!strncmp(s,"for.",     n))R CFOR;
            if(!strncmp(s,"for_",    4L)){ASSERTN(vnm(n-5,4+s),EVILNAME,nfs(n-5,4+s)); R CFOR;}
            if(!strncmp(s,"fcase.",   n))R CFCASE;   break;
  case 'g': if(!strncmp(s,"goto_",   5L))R CGOTO;    break;
  case 'i': if(!strncmp(s,"if.",      n))R CIF;      break;
  case 'l': if(!strncmp(s,"label_",  6L))R CLABEL;   break;
  case 'r': if(!strncmp(s,"return.",  n))R CRETURN;  break;
  case 's': if(!strncmp(s,"select.",  n))R CSELECT;  break;
  case 't': if(!strncmp(s,"throw.",   n))R CTHROW;
            if(!strncmp(s,"try.",     n))R CTRY;     break;
  case 'w': if(!strncmp(s,"while.",   n))R CWHILE;
            if(!strncmp(s,"whilst.",  n))R CWHILST;  break;
 }
 R 0;
}

static F1(jtgetsen){A y,z,*z0,*zv;C*s;I i,j,k=-1,m,n,*v;
 RZ(y=wordil(w));
 v=AV(y);                          /* pairs, (index, len)        */
 n=2**v++;                         /* count of pair element      */
 n=0>n?-(2+n):n;                   /* remove NB. pair            */
 GA(z,BOX,n/2,1,0); z0=zv=AAV(z);  /* list of ctrls & sentences  */ 
 s=CAV(w);                         /* text of entire string      */
 for(i=0;i<n;i+=2){
  j=v[i]; m=v[1+i];                /* index & length of this word*/
  if(0>k)k=j;                      /* index of sentence          */
  if(conword(m,j+s)){
   if(k<j)RZ(*zv++=str(j-k,k+s));  /* emit sentence in progress  */ 
   RZ(*zv++=str(m,j+s));           /* emit ctrl                  */
   k=-1;
 }}
 if(0<=k)RZ(*zv++=str(j+m-k,k+s)); /* emit sentence if any       */
 R vec(BOX,zv-z0,z0);
}    /* partition by controls */

/* preparse - return tokenized lines and control information     */
/* argument is a list of boxed explicit defn lines               */
/* result is 1 iff try is seen                                   */
/* zl - list of lines of tokens                                  */
/* zc - corresp list of control info                             */
// zs - local symbol table, to use and copy
/* control info has 3 I values for each line                     */
/* control info values - type, goto linenum, source linenum      */

#define ASSERTCW(b,j)  {if(!(b)){I jj=(j); jsignal3(EVCTRL,wv[jj],jj); R 0;}}

B jtpreparse(J jt,A w,A*zl,A*zc){PROLOG;A c,l,*lv,*v,w0,w1,*wv,x,y;B b=0,try=0;
     C*s;CW*d,*cv;I as=0,i,j,k,m,n,p,q,yn;
 RZ(w);
 p=AN(w); wv=AAV(w);
 ASSERT(p<SMAX,EVLIMIT);
 RZ(c=exta(CONW,1L,1L,3*p)); cv=(CW*)AV(c); n=0;  /* ctrl info             */
 RZ(l=exta(BOX, 1L,1L,5*p)); lv=    AAV(l); m=0;  /* tokens                */
 for(i=0;i<p;++i){
  RZ(y=getsen(wv[i])); yn=AN(y); v=AAV(y);
  for(j=0;j<yn;++j){
   if(n==AN(c)){RZ(c=ext(0,c)); cv=(CW*)AV(c);}
   w0=v[j];                             /* sentence text         */
   RZ(w1=wordil(w0));                   /* get wordlen again     */
   s=CAV(w0);
   k=conword(*(2+AV(w1)),s);            /* what kind of word?    */
   if(k==CTRY)try=1;                    /* try is seen           */
   if(k==CASSERT){ASSERTCW(!as,i  ); as=1;} 
   else if(1==as){ASSERTCW(!k, i); as=2; --n;}
   d=n+cv;                              /* address control info  */
   d->type=k?(C)k:2==as?CASSERT:CBBLOCK;/* control type          */
   d->source=(US)i;                     /* source line number    */
   d->go= !k||k==CCONT||k==CBREAK||k==CTHROW ? (US)SMAX : k==CRETURN ? (US)SMAX-1 : (US)(1+n);
   b|=k==CGOTO;                         /* goto seen?            */
   if(!k)RZ(x=enqueue(w1,w0)) else x=k==CLABEL||k==CGOTO||k==CFOR&&4<AN(w0)?w0:0L;
   q=k?1&&x:AN(x);
   ASSERT(q<SMAX,EVLIMIT);
   if(x){                               /* tokens of the line    */
    while(AN(l)<m+q){RZ(l=ext(0,l)); lv=AAV(l);}
    if(k)lv[m]=x; else ICPY(m+lv,AAV(x),q);
   }
   d->i=m; d->n=(US)q; m+=q;
   if(2==as)as=0;
   ++n;
 }}
 RE(0);
 ASSERTCW(!as,p-1);
 ASSERTCW(!b||0>(i=congoto(n,cv,lv)),(i+cv)->source);
 ASSERTCW(    0>(i= conall(n,cv   )),(i+cv)->source);
 AN(l)=*AS(l)=m; *zl=l;
 AN(c)=*AS(c)=n; *zc=c;
 R try;
}
